"""Separate queue/recovery policy model. No ARM/peripheral/baud execution."""
from pathlib import Path
import json,re,unittest,io
MASK=0xffffffff
SYNC=b'\r\n!SYNC\r\n'
def age(now,then):return (now-then)&MASK
def accepted(ready,elapsed,limit):return bool(ready and elapsed<=limit)
def frame(gen,raw,state):return f'D{gen},raw={raw:04d},state={"W" if state else "N"}\r\n'.encode('ascii')
class Lines:
    """Bounded exact CRLF syntax; no CRC, semantic checks or host acknowledgment."""
    def __init__(self):self.buf=bytearray();self.dropping=False;self.records=[];self.syncs=0;self.rejected=0
    def feed(self,data):
        for b in data:
            if self.dropping:
                if b==10:self.dropping=False
                continue
            self.buf.append(b)
            if len(self.buf)>48:self.buf.clear();self.dropping=True;self.rejected+=1;continue
            if b!=10:continue
            line=bytes(self.buf);self.buf.clear()
            if line==b'\r\n':continue
            if line==b'!SYNC\r\n':self.syncs+=1;continue
            m=re.fullmatch(rb'D([01]),raw=([0-9]{4}),state=([NW])\r\n',line)
            if m and int(m[2])<=4095:
                self.records.append(dict(generation=int(m[1]),raw=int(m[2]),state=m[3].decode()))
            else:self.rejected+=1

class Model:
    """Starts just after initial calibration; later cleanup/calibration use injected flags."""
    def __init__(self,gap=0):
        self.run='ACTIVE';self.tx='IDLE';self.adc_wait=False;self.last_slot=0;self.adc_since=0
        self.ready=1;self.valid=0;self.raw=65535;self.alarm=0;self.samples=0;self.skipped=0
        self.gen=0;self.sample_gen=MASK;self.q=[];self.admitted=0;self.full_dropped=0
        self.pending_discarded=0;self.stale=0;self.abandoned=0;self.last_discarded=0;self.abandon_info=None
        self.active=None;self.report=b'';self.index=0;self.confirmed=[];self.wire=bytearray()
        self.gap=gap;self.gate=False;self.pace=0;self.tx_since=0
        self.error=0;self.error_tick=0;self.last_error=0;self.last_error_tick=0
        self.attempts=0;self.denied=0;self.recovering=0;self.reinit=0
        self.since=0;self.sync_index=0;self.resets=0;self.commands=[];self.trace=[]
    def halt(self,code,now):
        self.error=code;self.error_tick=now;self.run='HALTED';self.valid=0;self.recovering=0
    def offer(self,raw):
        assert 0<=raw<=4095
        self.raw=raw;self.sample_gen=self.gen;self.samples+=1;self.valid=1
        if raw>=2600:self.alarm=1
        elif raw<=2400:self.alarm=0
        if len(self.q)<2:
            self.q.append((self.samples,raw,self.alarm,self.sample_gen));self.admitted+=1
        else:self.full_dropped+=1
    def invariant(self):
        assert 0<=len(self.q)<=2
        assert self.samples==self.admitted+self.full_dropped
        assert self.admitted==len(self.q)+(self.active is not None)+len(self.confirmed)+self.pending_discarded+self.stale+self.abandoned
    def step(self,now,retry=False,eoc=True,txe=True,tc=True,rst_ready=True,cal_ready=True,raw=3000):
        now&=MASK
        if retry:
            if self.run=='HALTED' and self.attempts==0 and self.error in (3,4,5):
                self.last_error=self.error;self.last_error_tick=self.error_tick
                self.attempts+=1;self.recovering=1;self.valid=0;self.ready=0
                self.last_discarded=len(self.q);self.pending_discarded+=len(self.q);self.q.clear()
                if self.active is not None:
                    self.abandoned+=1;self.abandon_info=(self.active,self.index);self.active=None
                self.adc_wait=False;self.tx='IDLE';self.since=now;self.run='DRAIN'
            else:self.denied+=1
        if self.run!='ACTIVE':
            elapsed=age(now,self.since)
            if self.run=='DRAIN':
                if accepted(tc,elapsed,50):self.sync_index=0;self.since=now;self.run='SYNC_SEND'
                elif elapsed>=50:self.halt(7,now)
            elif self.run=='SYNC_SEND':
                if accepted(txe,elapsed,50):
                    self.wire.append(SYNC[self.sync_index]);self.sync_index+=1;self.since=now
                    if self.sync_index==9:self.run='SYNC_TC'
                elif elapsed>=50:self.halt(8,now)
            elif self.run=='SYNC_TC':
                if accepted(tc,elapsed,50):self.run='CONFIG'
                elif elapsed>=50:self.halt(9,now)
            elif self.run=='CONFIG':
                self.valid=0;self.ready=0;self.resets+=1
                self.commands+=['ADON0','ADC1RST1_0','CONFIGURE','STABILIZE','RSTCAL']
                self.since=now;self.run='RSTCAL'
            elif self.run=='RSTCAL':
                if accepted(rst_ready,elapsed,10):self.commands+=['CAL'];self.since=now;self.run='CAL'
                elif elapsed>=10:self.halt(1,now)
            elif self.run=='CAL':
                if accepted(cal_ready,elapsed,10):
                    self.commands+=['DISCARD_CAL_DR','CLEAR_SR'];self.ready=1
                    if self.recovering:self.gen+=1;self.reinit+=1;self.recovering=0
                    self.error=0;self.adc_wait=False;self.tx='IDLE';self.last_slot=now;self.run='ACTIVE'
                elif elapsed>=10:self.halt(2,now)
            return
        if not self.adc_wait:
            elapsed=age(now,self.last_slot)
            if elapsed>=100:
                periods=elapsed//100;self.last_slot=(self.last_slot+periods*100)&MASK
                self.skipped+=periods-1;self.valid=0;self.adc_since=now;self.adc_wait=True;self.commands+=['SWSTART']
        else:
            elapsed=age(now,self.adc_since)
            if accepted(eoc,elapsed,10):self.offer(raw);self.adc_wait=False
            elif elapsed>=10:self.halt(3,now)
        if self.run!='ACTIVE':return
        if self.tx=='IDLE':
            if self.q:
                p=self.q.pop(0)
                if p[3]!=self.gen:self.stale+=1;return
                self.active=p;self.report=frame(p[3],p[1],p[2]);self.index=0;self.pace=now;self.gate=False;self.tx='SEND'
        elif self.tx=='SEND':
            if not self.gate:
                if age(now,self.pace)<(self.gap if self.attempts==0 else 0):return
                self.tx_since=now;self.gate=True
            elapsed=age(now,self.tx_since)
            if accepted(txe,elapsed,50):
                self.wire.append(self.report[self.index]);self.index+=1;self.pace=now;self.gate=False
                if self.index==21:self.tx_since=now;self.tx='TC'
            elif elapsed>=50:self.halt(4,now)
        elif self.tx=='TC':
            elapsed=age(now,self.tx_since)
            if accepted(tc,elapsed,50):self.confirmed.append(self.active);self.active=None;self.tx='IDLE'
            elif elapsed>=50:self.halt(5,now)

def seeded():
    m=Model();m.offer(3000);m.step(0);m.offer(2000);m.offer(2100)
    for t in range(1,5):m.step(t)
    m.halt(4,5);return m
def complete_cleanup(m,start=10):
    m.step(start,retry=True)
    for t in range(start+1,start+14):m.step(t)
    return start+13
def scenario(kind):
    m=Model(80)
    for t in range(1001):
        before=(m.run,m.tx,m.samples,len(m.q),m.pending_discarded,m.gen,m.valid,len(m.confirmed))
        hide=m.attempts==0 and m.index>=4 and m.tx=='SEND'
        m.step(t,retry=t==600,txe=not hide,tc=not(kind=='drain_refused' and (m.run=='DRAIN' or t==600)),
               cal_ready=not(kind=='cal_refused' and m.attempts==1),raw=3000 if m.attempts==0 else 1500)
        m.invariant()
        after=(m.run,m.tx,m.samples,len(m.q),m.pending_discarded,m.gen,m.valid,len(m.confirmed))
        if before!=after:m.trace.append(dict(tick=t,run=m.run,tx=m.tx,samples=m.samples,pending=len(m.q),
            full_dropped=m.full_dropped,pending_discarded=m.pending_discarded,abandoned=m.abandoned,
            generation=m.gen,sample_generation=m.sample_gen,valid=m.valid,ready=m.ready,
            error=m.error,confirmed=len(m.confirmed)))
    parser=Lines();parser.feed(m.wire)
    return dict(trace=m.trace,submitted_ascii=m.wire.decode(),records=parser.records,syncs=parser.syncs,
        rejected=parser.rejected,error=m.error,last_error=m.last_error,attempts=m.attempts,
        generation=m.gen,reinit=m.reinit,samples=m.samples,admitted=m.admitted,full_dropped=m.full_dropped,
        pending_discarded=m.pending_discarded,abandoned=m.abandoned,confirmed=len(m.confirmed),
        pending=len(m.q),active=m.active)

class Tests(unittest.TestCase):
    def test_pending_and_active_separate_counts(self):
        m=seeded();self.assertEqual(len(m.q),2);m.step(10,retry=True)
        self.assertEqual((m.pending_discarded,m.abandoned,len(m.q)),(2,1,0));m.invariant()
    def test_no_request_preserves_frozen_pending(self):
        m=seeded();q=list(m.q);m.step(1000)
        self.assertEqual(m.q,q);self.assertEqual(m.pending_discarded,0)
    def test_running_request_consumed_and_denied(self):
        m=Model();m.step(0,retry=True);self.assertEqual((m.attempts,m.denied,m.run),(0,1,'ACTIVE'))
    def test_unsupported_boot_failure(self):
        for code in [1,2,6,7,8,9]:
            m=Model();m.halt(code,0);m.step(1,retry=True);self.assertEqual((m.attempts,m.denied),(0,1))
    def test_second_retry_denied_after_success(self):
        m=seeded();complete_cleanup(m);m.halt(4,30);m.step(31,retry=True)
        self.assertEqual((m.attempts,m.denied,m.gen),(1,1,1))
    def test_old_error_preserved(self):
        m=seeded();m.step(10,retry=True,tc=False);self.assertEqual((m.error,m.last_error,m.last_error_tick),(4,4,5))
    def test_drain_writes_no_more_old_bytes(self):
        m=seeded();n=len(m.wire);m.step(10,retry=True,tc=False);m.step(20,tc=False)
        self.assertEqual(len(m.wire),n);self.assertEqual(m.run,'DRAIN')
    def test_drain_failure_no_marker_or_reset(self):
        m=seeded();m.step(10,retry=True,tc=False);m.step(60,tc=False)
        self.assertEqual((m.error,m.gen,m.resets,m.recovering),(7,0,0,0));self.assertEqual(bytes(m.wire),b'D0,r')
    def test_sync_txe_failure(self):
        m=seeded();m.step(10,retry=True);m.step(60,txe=False)
        self.assertEqual((m.error,m.resets,m.gen),(8,0,0))
    def test_sync_final_tc_required(self):
        m=seeded();m.step(10,retry=True)
        for t in range(11,20):m.step(t)
        self.assertEqual(m.run,'SYNC_TC');m.step(69,tc=False)
        self.assertEqual((m.error,m.resets,m.gen),(9,0,0))
    def test_marker_then_stop_reset_calibration(self):
        m=seeded();complete_cleanup(m)
        self.assertEqual(bytes(m.wire),b'D0,r'+SYNC)
        self.assertEqual(m.commands[-8:],['ADON0','ADC1RST1_0','CONFIGURE','STABILIZE','RSTCAL','CAL','DISCARD_CAL_DR','CLEAR_SR'])
    def test_ready_not_fresh(self):
        m=seeded();end=complete_cleanup(m)
        self.assertEqual((m.gen,m.ready,m.valid,m.raw,m.sample_gen),(1,1,0,2100,0))
        self.assertEqual(len(m.q),0);self.assertIsNone(m.active)
    def test_old_eoc_during_cleanup_not_published(self):
        m=seeded();samples=m.samples;complete_cleanup(m)
        self.assertEqual(m.samples,samples)
    def test_new_conversion_required(self):
        m=seeded();end=complete_cleanup(m);m.step(end+100)
        self.assertEqual(m.valid,0);m.step(end+101,raw=1500)
        self.assertEqual((m.valid,m.sample_gen,m.active[3]),(1,1,1));self.assertTrue(m.report.startswith(b'D1,'))
    def test_stale_slot_rejected_not_relabelled(self):
        m=Model();m.offer(3000);m.gen=1;m.step(0)
        self.assertEqual(m.stale,1);self.assertIsNone(m.active);self.assertEqual(m.report,b'');m.invariant()
    def test_current_line_uses_snapshot_generation(self):
        m=Model();m.offer(3000);m.step(0);saved=m.report;m.offer(1500)
        self.assertEqual(m.report,saved);self.assertEqual(m.active[1:],(3000,1,0))
    def test_full_drop_is_distinct_from_recovery_discard(self):
        m=seeded();m.offer(3000);self.assertEqual(m.full_dropped,1)
        m.step(10,retry=True);self.assertEqual((m.full_dropped,m.pending_discarded),(1,2));m.invariant()
    def test_cal_failure_does_not_advance_epoch(self):
        m=seeded();m.step(10,retry=True)
        for t in range(11,23):m.step(t)
        m.step(32,cal_ready=False);self.assertEqual((m.error,m.gen,m.reinit,m.valid,m.recovering),(2,0,0,0,0))
    def test_rstcal_failure(self):
        m=seeded();m.step(10,retry=True)
        for t in range(11,22):m.step(t)
        m.step(31,rst_ready=False);self.assertEqual((m.error,m.gen,m.valid),(1,0,0))
    def test_old_complete_record_cannot_be_retracted(self):
        p=Lines();p.feed(frame(0,3000,1)+SYNC+frame(1,1500,0))
        self.assertEqual([r['generation'] for r in p.records],[0,1]);self.assertEqual(p.syncs,1)
    def test_partial_closed_before_new_record(self):
        p=Lines();p.feed(b'D0,r'+SYNC+frame(1,1500,0));self.assertEqual(p.rejected,1)
        self.assertEqual(p.records,[dict(generation=1,raw=1500,state='N')])
    def test_chunk_boundaries(self):
        p=Lines()
        for b in b'D0,r'+SYNC+frame(1,1500,0):p.feed(bytes([b]))
        self.assertEqual((len(p.records),p.syncs,p.rejected),(1,1,1))
    def test_full_wire_and_refusal_scenarios(self):
        a=scenario('success');b=scenario('drain_refused');c=scenario('cal_refused')
        self.assertEqual((a['pending_discarded'],a['abandoned'],a['generation']),(2,1,1))
        self.assertTrue(a['records']);self.assertTrue(all(r['generation']==1 for r in a['records']))
        self.assertEqual((b['error'],b['generation'],b['records']),(7,0,[]))
        self.assertEqual((c['error'],c['generation'],c['records']),(2,0,[]))
    def test_tick_wrap_and_ready_boundary(self):
        self.assertEqual(age(5,0xfffffff0),21)
        self.assertTrue(accepted(True,50,50));self.assertFalse(accepted(True,51,50))
    def test_frozen_tick_does_not_expire(self):
        m=seeded();m.step(10,retry=True,tc=False)
        for _ in range(99):m.step(10,tc=False)
        self.assertEqual(m.error,4);self.assertEqual(m.run,'DRAIN')
    def test_late_eoc_no_new_queue_entry(self):
        m=Model();m.step(100);m.step(111,eoc=True)
        self.assertEqual((m.error,m.samples,len(m.q)),(3,0,0))
    def test_whole_line_sent_not_pc_ack(self):
        m=Model();m.offer(1000);m.step(0)
        for t in range(1,22):m.step(t)
        self.assertEqual((m.tx,len(m.confirmed)),('TC',0));m.step(22)
        self.assertEqual(len(m.confirmed),1)

if __name__=='__main__':
    out=Path(__file__).resolve().parent
    results={k:scenario(k) for k in ['success','drain_refused','cal_refused']}
    (out/'comparison.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf8')
    log=io.StringIO();test=unittest.TextTestRunner(stream=log,verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Tests))
    (out/'TEST_RESULT.txt').write_text(log.getvalue(),encoding='utf8');print(log.getvalue())
    print({k:{n:r[n] for n in ['error','generation','samples','pending_discarded','abandoned','confirmed']} for k,r in results.items()})
    raise SystemExit(not test.wasSuccessful())
