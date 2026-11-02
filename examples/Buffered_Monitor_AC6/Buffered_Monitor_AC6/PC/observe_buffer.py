"""Separate policy model. Injected ADC/TXE/TC observations; no ARM/baud simulation."""
from pathlib import Path
import json, unittest, io

MASK=0xffffffff
def age(now,then): return (now-then)&MASK
def frame(raw,alarm): return f'D0,raw={raw:04d},state={"W" if alarm else "N"}\r\n'.encode('ascii')

class Model:
    def __init__(self,gap=0):
        self.gap=gap;self.last_slot=0;self.adc_since=0;self.adc_wait=False
        self.raw=None;self.alarm=0;self.valid=False;self.samples=0;self.skipped=0
        self.q=[None,None];self.rd=0;self.wr=0;self.count=0;self.peak=0
        self.admitted=0;self.dropped=0;self.active=None;self.tx='IDLE';self.index=0
        self.report=b'';self.gate=False;self.pace=0;self.tx_since=0
        self.confirmed=[];self.wire=bytearray();self.error=0;self.trace=[]
    def offer(self,raw):
        self.raw=raw
        if raw>=2600:self.alarm=1
        elif raw<=2400:self.alarm=0
        self.samples+=1;self.valid=True
        snapshot=(self.samples,raw,self.alarm)
        if self.count<2:
            self.q[self.wr]=snapshot;self.wr+=1
            if self.wr==2:self.wr=0
            self.count+=1;self.admitted+=1;self.peak=max(self.peak,self.count)
        else:self.dropped+=1
    def pop(self):
        if not self.count:return None
        result=self.q[self.rd];self.rd+=1
        if self.rd==2:self.rd=0
        self.count-=1;return result
    def step(self,now,raw=2000,eoc=True,txe=True,tc=True):
        now&=MASK
        if self.error:return
        if not self.adc_wait:
            elapsed=age(now,self.last_slot)
            if elapsed>=100:
                periods=elapsed//100;self.last_slot=(self.last_slot+periods*100)&MASK
                self.skipped+=periods-1;self.adc_since=now;self.adc_wait=True;self.valid=False
        else:
            elapsed=age(now,self.adc_since)
            if eoc and elapsed<=10:self.offer(raw);self.adc_wait=False
            elif elapsed>=10:self.error=3
        if self.error:return
        if self.tx=='IDLE':
            if self.count:
                self.active=self.pop();self.report=frame(self.active[1],self.active[2])
                self.index=0;self.pace=now;self.gate=False;self.tx='SEND'
        elif self.tx=='SEND':
            if not self.gate:
                if age(now,self.pace)<self.gap:return
                self.tx_since=now;self.gate=True
            elapsed=age(now,self.tx_since)
            if txe and elapsed<=50:
                self.wire.append(self.report[self.index]);self.index+=1
                self.pace=now;self.gate=False
                if self.index==21:self.tx_since=now;self.tx='TC'
            elif elapsed>=50:self.error=4
        elif self.tx=='TC':
            elapsed=age(now,self.tx_since)
            if tc and elapsed<=50:self.confirmed.append(self.active);self.active=None;self.tx='IDLE'
            elif elapsed>=50:self.error=5
    def invariant(self):
        assert 0<=self.count<=2 and self.rd in (0,1) and self.wr in (0,1)
        assert self.samples==self.admitted+self.dropped
        assert self.admitted==len(self.confirmed)+self.count+(self.active is not None)

def run(gap):
    m=Model(gap)
    for t in range(1001):
        before=(m.samples,m.dropped,len(m.confirmed),m.tx)
        m.step(t,raw=3000 if (t//100)%2 else 1500);m.invariant()
        if before!=(m.samples,m.dropped,len(m.confirmed),m.tx):
            m.trace.append(dict(tick=t,samples=m.samples,raw=m.raw,alarm=m.alarm,
                pending=m.count,dropped=m.dropped,tx=m.tx,
                active_id=m.active[0] if m.active else None,
                confirmed=len(m.confirmed)))
    return m

class Tests(unittest.TestCase):
    def test_fifo_wrap(self):
        m=Model();m.offer(100);m.offer(200)
        self.assertEqual(m.pop()[0],1);m.offer(300)
        self.assertEqual([m.pop()[0],m.pop()[0]],[2,3]);self.assertEqual(m.count,0)
    def test_equal_indices_distinguished_by_count(self):
        m=Model();self.assertEqual(m.rd,m.wr);self.assertEqual(m.count,0)
        m.offer(1);m.offer(2);self.assertEqual(m.rd,m.wr);self.assertEqual(m.count,2)
    def test_full_keeps_pending(self):
        m=Model();m.offer(100);m.offer(200);before=list(m.q);m.offer(3000)
        self.assertEqual(m.q,before);self.assertEqual(m.dropped,1)
        self.assertEqual((m.samples,m.raw,m.alarm,m.valid),(3,3000,1,True))
    def test_snapshot_is_not_live_raw(self):
        m=Model();m.offer(3000);m.step(0);report=m.report;m.offer(1500)
        self.assertEqual(m.report,report);self.assertEqual(m.active,(1,3000,1))
    def test_state_is_copied_with_raw(self):
        m=Model();m.offer(3000);m.offer(2500)
        self.assertEqual(m.pop()[1:],(3000,1));self.assertEqual(m.pop()[1:],(2500,1))
    def test_hysteresis(self):
        m=Model()
        for x,want in [(2500,0),(2600,1),(2500,1),(2400,0)]:
            m.offer(x);self.assertEqual(m.alarm,want)
    def test_two_pending_plus_one_active(self):
        m=Model();m.offer(1);m.step(0);m.offer(2);m.offer(3)
        self.assertEqual((m.count,m.active[0]),(2,1));m.offer(4);self.assertEqual(m.dropped,1)
    def test_normal_no_drop(self):
        m=run(0);self.assertEqual(m.dropped,0);self.assertEqual(len(m.confirmed),9)
        self.assertEqual([x[0] for x in m.confirmed],list(range(1,10)))
    def test_slow_still_acquires(self):
        fast=run(0);slow=run(20)
        self.assertEqual(slow.samples,fast.samples);self.assertEqual(slow.samples,9)
        self.assertGreater(slow.dropped,0);self.assertEqual(slow.peak,2)
        self.assertEqual(slow.error,0)
    def test_active_line_not_mixed(self):
        m=run(20);self.assertTrue(bytes(m.wire).startswith(frame(3000,1)))
        self.assertEqual(m.confirmed[0],(1,3000,1))
    def test_at_most_one_dr_submission_per_pass(self):
        m=Model();m.offer(1234)
        for t in range(30):
            n=len(m.wire);m.step(t);self.assertLessEqual(len(m.wire)-n,1)
    def test_final_byte_not_confirmation(self):
        m=Model();m.offer(0);m.step(0)
        for t in range(1,22):m.step(t)
        self.assertEqual(m.tx,'TC');self.assertEqual(m.confirmed,[])
        m.step(22,tc=True);self.assertEqual(len(m.confirmed),1)
    def test_gap_is_not_false_txe_timeout(self):
        m=Model(80);m.offer(0);m.step(0);m.step(79,txe=False)
        self.assertFalse(m.gate);self.assertEqual(m.error,0)
        m.step(80,txe=False);m.step(130,txe=True)
        self.assertEqual(m.error,0);self.assertEqual(m.index,1)
    def test_txe_timeout_freezes_both(self):
        m=Model();m.offer(1);m.step(0);m.step(1,txe=False);m.step(51,txe=False)
        self.assertEqual(m.error,4);before=(m.samples,bytes(m.wire),list(m.q))
        m.step(1000);self.assertEqual((m.samples,bytes(m.wire),m.q),before)
    def test_adc_error_prevents_same_pass_write(self):
        m=Model();m.offer(1000);m.step(0);m.step(100,eoc=False,txe=False)
        n=len(m.wire);m.step(110,eoc=False,txe=True)
        self.assertEqual(m.error,3);self.assertEqual(len(m.wire),n)
    def test_tc_timeout(self):
        m=Model();m.offer(0);m.step(0)
        for t in range(1,22):m.step(t)
        m.step(71,tc=False);self.assertEqual(m.error,5);self.assertEqual(len(m.confirmed),0)
    def test_late_eoc_rejected(self):
        m=Model();m.step(100);m.step(111,eoc=True)
        self.assertEqual(m.error,3);self.assertEqual(m.samples,0)
    def test_late_txe_rejected(self):
        m=Model();m.offer(1);m.step(0);m.step(1,txe=False);m.step(52,txe=True)
        self.assertEqual(m.error,4);self.assertEqual(m.index,0)
    def test_skipped_slots_not_invented(self):
        m=Model();m.step(350);m.step(351)
        self.assertEqual((m.samples,m.skipped,m.last_slot),(1,2,300))
    def test_unsigned_wrap(self):
        self.assertEqual(age(5,0xfffffff0),21)
        m=Model();m.last_slot=0xffffffd0;m.step(0x34)
        self.assertTrue(m.adc_wait);self.assertEqual(m.skipped,0)
    def test_fixed_frame(self):
        self.assertEqual(frame(4095,1),b'D0,raw=4095,state=W\r\n')
        self.assertEqual(len(frame(0,0)),21)
    def test_frozen_tick_no_time_progress(self):
        m=Model();m.step(100)
        for _ in range(30):m.step(100,eoc=False)
        self.assertEqual(m.error,0);self.assertTrue(m.adc_wait)

if __name__=='__main__':
    p=Path(__file__).resolve().parent;result={}
    for name,gap in [('normal',0),('slow20',20)]:
        m=run(gap)
        result[name]=dict(samples=m.samples,admitted=m.admitted,dropped=m.dropped,
            confirmed=len(m.confirmed),pending=m.count,active=m.active,peak=m.peak,
            confirmed_snapshots=m.confirmed,trace=m.trace,
            submitted_bytes_ascii=m.wire.decode('ascii'))
    (p/'comparison.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf8')
    log=io.StringIO();runner=unittest.TextTestRunner(stream=log,verbosity=2)
    tests=runner.run(unittest.defaultTestLoader.loadTestsFromTestCase(Tests))
    (p/'TEST_RESULT.txt').write_text(log.getvalue(),encoding='utf8');print(log.getvalue())
    print({k:{n:v[n] for n in ['samples','admitted','dropped','confirmed','pending','peak']} for k,v in result.items()})
    raise SystemExit(not tests.wasSuccessful())
