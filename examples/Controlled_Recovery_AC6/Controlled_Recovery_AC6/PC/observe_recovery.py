"""Independent policy model + byte-line parser; NOT ARM/peripheral/baud execution."""
from pathlib import Path
import json,re,unittest
MASK=0xffffffff
IDLE,ADC,SEND,TC,ERROR=0,1,2,3,4
CONFIG,RSTCAL,CAL,DRAIN,SYNC_SEND,SYNC_TC=10,11,12,20,21,22
SYNC=b'\r\n!SYNC\r\n'
def accepted(ready,age,limit):return bool(ready and age<=limit)
class Controller:
    def __init__(self,uart=False):
        self.uart=uart;self.phase=CONFIG;self.error=0;self.error_tick=0;self.last_error=0;self.attempts=0;self.denied=0;self.active=0;self.reinit=0;self.generation=0;self.sample_generation=MASK
        self.raw=65535;self.valid=0;self.ready=0;self.alarm=0;self.samples=0;self.since=0;self.last_slot=0;self.resets=0;self.index=0;self.sync_index=0;self.confirmed=0;self.frame=b'';self.wire=b'';self.commands=[]
    def fault(self,code,tick):self.error=code;self.error_tick=tick;self.active=0;self.phase=ERROR
    def step(self,tick,retry=False,rst_ready=True,cal_ready=True,eoc=False,txe=True,tc=True,raw=2600):
        if retry:
            allowed=self.error in ([3,4,5] if self.uart else [3])
            if self.phase==ERROR and self.attempts==0 and allowed:
                self.last_error=self.error;self.attempts+=1;self.active=1;self.valid=0;self.ready=0;self.since=tick;self.phase=DRAIN if self.uart else CONFIG
            else:self.denied+=1
        age=(tick-self.since)&MASK
        if self.phase==CONFIG:
            self.commands.append('ADC_OFF_RESET_RSTCAL');self.resets+=1;self.ready=0;self.valid=0;self.since=tick;self.phase=RSTCAL
        elif self.phase==RSTCAL:
            if accepted(rst_ready,age,10):self.commands.append('CAL_START');self.since=tick;self.phase=CAL
            elif age>=10:self.fault(1,tick)
        elif self.phase==CAL:
            if accepted(cal_ready,age,10):
                self.commands.append('DISCARD_CAL_DR');self.ready=1
                if self.active:self.generation+=1;self.reinit+=1;self.active=0
                self.error=0;self.last_slot=tick;self.phase=IDLE
            elif age>=10:self.fault(2,tick)
        elif self.phase==IDLE:
            elapsed=(tick-self.last_slot)&MASK
            if elapsed>=100:
                self.last_slot=(self.last_slot+(elapsed//100)*100)&MASK;self.valid=0;self.commands.append('SWSTART');self.since=tick;self.phase=ADC
        elif self.phase==ADC:
            if accepted(eoc,age,10):
                assert 0<=raw<=4095
                self.raw=raw;self.valid=1;self.sample_generation=self.generation;self.samples+=1;self.commands.append('PUBLISH_INPUT_DR')
                if raw>=2600:self.alarm=1
                elif raw<=2400:self.alarm=0
                if self.uart:
                    self.frame=f'D{self.generation},raw={raw:04d},state={"W" if self.alarm else "N"}\r\n'.encode();self.index=0;self.since=tick;self.phase=SEND
                else:self.phase=IDLE
            elif age>=10:self.fault(3,tick)
        elif self.phase==SEND:
            if accepted(txe,age,50):
                self.wire+=self.frame[self.index:self.index+1];self.index+=1;self.since=tick
                if self.index==21:self.phase=TC
            elif age>=50:self.fault(4,tick)
        elif self.phase==TC:
            if accepted(tc,age,50):self.confirmed+=1;self.phase=IDLE
            elif age>=50:self.fault(5,tick)
        elif self.phase==DRAIN:
            if accepted(tc,age,50):self.sync_index=0;self.since=tick;self.phase=SYNC_SEND
            elif age>=50:self.fault(7,tick)
        elif self.phase==SYNC_SEND:
            if accepted(txe,age,50):
                self.wire+=SYNC[self.sync_index:self.sync_index+1];self.sync_index+=1;self.since=tick
                if self.sync_index==9:self.phase=SYNC_TC
            elif age>=50:self.fault(8,tick)
        elif self.phase==SYNC_TC:
            if accepted(tc,age,50):self.phase=CONFIG
            elif age>=50:self.fault(9,tick)
        return dict(tick=tick,phase=self.phase,error=self.error,last_error=self.last_error,attempts=self.attempts,generation=self.generation,valid=self.valid,ready=self.ready,raw=self.raw,samples=self.samples,submitted=self.index,sync_submitted=self.sync_index,confirmed=self.confirmed,resets=self.resets)
class Lines:
    """Bounded CRLF grammar, no CRC/transport error detection/host ACK."""
    def __init__(self):self.buf=bytearray();self.dropping=False;self.records=[];self.syncs=0;self.rejected=0
    def feed(self,data):
        for byte in data:
            if self.dropping:
                if byte==10:self.dropping=False
                continue
            self.buf.append(byte)
            if len(self.buf)>48:
                self.rejected+=1;self.buf.clear();self.dropping=(byte!=10);continue
            if byte!=10:continue
            line=bytes(self.buf);self.buf.clear()
            if line==b'\r\n':continue
            if line==b'!SYNC\r\n':self.syncs+=1;continue
            m=re.fullmatch(rb'D([01]),raw=([0-9]{4}),state=([NW])\r\n',line)
            if m and int(m[2])<=4095:self.records.append(dict(generation=int(m[1]),raw=int(m[2]),state=m[3].decode()))
            else:self.rejected+=1
def warm(uart=False):
    m=Controller(uart)
    for _ in range(3):m.step(0)
    return m
def adc_fault():
    m=warm();m.raw=3000;m.valid=1;m.sample_generation=0;m.samples=1;m.alarm=1;m.step(100);m.step(110);return m
def partial():
    m=warm(True);m.step(100);m.step(101,eoc=True)
    for t in [102,104,106,108]:m.step(t)
    m.step(158,txe=False);return m
def sync_complete(m,start=160):
    m.step(start,retry=True,tc=True)
    for t in range(start+1,start+10):m.step(t)
    m.step(start+10,tc=True)
    return start+11
class RecoveryTests(unittest.TestCase):
    def test_no_automatic_retry(self):
        m=adc_fault();m.step(200,eoc=True);self.assertEqual((m.phase,m.attempts,m.raw),(ERROR,0,3000))
    def test_request_during_active_work_is_denied(self):
        m=warm();m.step(1,retry=True);self.assertEqual((m.denied,m.attempts),(1,0))
    def test_boot_fault_is_not_supported(self):
        m=warm();m.fault(1,10);m.step(11,retry=True);self.assertEqual((m.phase,m.attempts,m.denied),(ERROR,0,1))
    def test_only_one_retry_per_boot(self):
        m=adc_fault();m.step(111,retry=True);m.fault(3,120);m.step(121,retry=True);self.assertEqual((m.attempts,m.denied,m.phase),(1,1,ERROR))
    def test_preserve_original_error(self):
        m=partial();m.step(160,retry=True,tc=False);m.step(210,tc=False);self.assertEqual((m.last_error,m.error),(4,7))
    def test_eoc_during_cleanup_is_not_published(self):
        m=adc_fault()
        for t in [111,112,113]:m.step(t,retry=t==111,eoc=True,raw=3999)
        self.assertEqual((m.raw,m.samples,m.valid),(3000,1,0))
    def test_reinit_is_not_a_fresh_sample(self):
        m=adc_fault()
        for t in [111,112,113]:m.step(t,retry=t==111)
        self.assertEqual((m.ready,m.reinit,m.generation,m.valid,m.sample_generation),(1,1,1,0,0))
    def test_fresh_result_requires_new_start(self):
        m=adc_fault()
        for t in [111,112,113]:m.step(t,retry=t==111)
        m.step(213,eoc=True,raw=0);self.assertEqual(m.valid,0);m.step(214,eoc=True,raw=0);self.assertEqual((m.valid,m.raw,m.sample_generation),(1,0,1))
    def test_drain_makes_no_new_byte_submissions(self):
        m=partial();before=m.wire;m.step(160,retry=True,tc=False);m.step(209,tc=False);self.assertEqual((m.wire,m.sync_index,m.resets),(before,0,1))
    def test_drain_failure_refuses_sync_and_reinit(self):
        m=partial();m.step(160,retry=True,tc=False);m.step(210,tc=False);self.assertEqual((m.error,m.sync_index,m.resets,m.generation),(7,0,1,0))
    def test_sync_txe_failure(self):
        m=partial();m.step(160,retry=True);m.step(210,txe=False);self.assertEqual((m.error,m.resets),(8,1))
    def test_sync_tc_failure(self):
        m=partial();m.step(160,retry=True)
        for t in range(161,170):m.step(t)
        m.step(219,tc=False);self.assertEqual((m.error,m.resets),(9,1))
    def test_sync_bytes_exact_and_no_data_yet(self):
        m=partial();sync_complete(m);self.assertEqual(m.wire,b'D0,r'+SYNC);self.assertEqual((m.resets,m.valid),(1,0))
    def test_reset_only_after_sync_tc(self):
        m=partial();m.step(160,retry=True)
        for t in range(161,170):m.step(t)
        m.step(170,tc=False);self.assertEqual(m.resets,1);m.step(171,tc=True);self.assertEqual(m.resets,1);m.step(172);self.assertEqual(m.resets,2)
    def test_complete_old_line_cannot_be_retracted(self):
        p=Lines();p.feed(b'D0,raw=2600,state=W\r\n'+SYNC+b'D1,raw=2500,state=W\r\n');self.assertEqual([r['generation'] for r in p.records],[0,1])
    def test_partial_without_boundary_is_rejected(self):
        p=Lines();p.feed(b'D0,rD1,raw=2600,state=W\r\n');self.assertEqual((len(p.records),p.rejected),(0,1))
    def test_boundary_allows_fresh_line(self):
        p=Lines();p.feed(b'D0,r'+SYNC+b'D1,raw=2600,state=W\r\n');self.assertEqual((len(p.records),p.syncs,p.rejected),(1,1,1));self.assertEqual(p.records[0]['generation'],1)
    def test_arbitrary_chunk_sizes(self):
        data=b'D0,r'+SYNC+b'D1,raw=0000,state=N\r\n'
        for size in [1,2,7,50]:
            p=Lines()
            for j in range(0,len(data),size):p.feed(data[j:j+size])
            self.assertEqual(p.records,[dict(generation=1,raw=0,state='N')])
    def test_overlong_line_then_new_line(self):
        p=Lines();p.feed(b'x'*100+b'\r\nD1,raw=4095,state=W\r\n');self.assertEqual((p.rejected,len(p.records)),(1,1));self.assertLessEqual(len(p.buf),48)
    def test_raw_range_and_grammar(self):
        p=Lines();p.feed(b'D1,raw=9999,state=W\r\nD1,raw=0123,state=X\r\nD2,raw=0123,state=N\r\n');self.assertEqual((p.rejected,p.records),(3,[]))
    def test_sync_is_control_not_measurement(self):
        p=Lines();p.feed(SYNC);self.assertEqual((p.syncs,p.records),(1,[]))
    def test_frozen_tick_during_drain(self):
        m=partial();m.step(160,retry=True,tc=False)
        for _ in range(1000):m.step(160,tc=False)
        self.assertEqual(m.phase,DRAIN)
    def test_drain_ready_at_boundary_and_wrap(self):
        m=partial();m.step(160,retry=True,tc=False);m.since=0xfffffffc;m.step(46,tc=True);self.assertEqual(m.phase,SYNC_SEND)
    def test_calibration_fails_after_retry(self):
        m=adc_fault();m.step(111,retry=True);m.step(112);m.step(122,cal_ready=False);self.assertEqual((m.error,m.generation,m.valid,m.reinit,m.active),(2,0,0,0,0))
def write_results():
    folder=Path(__file__).resolve().parent
    streams={
      'without_boundary':['D0,r','D1,raw=2600,state=W\r\n'],
      'with_boundary':['D0,r','\r\n!SYNC\r\n','D1,raw=2600,state=W\r\n'],
      'old_complete_then_new':['D0,raw=2600,state=W\r\n','\r\n!SYNC\r\n','D1,raw=2500,state=W\r\n'],
      'split_chunks':['D','1,ra','w=0','000,state=N\r','\n']}
    source=folder/'streams.json'
    if not source.exists():source.write_text(json.dumps(streams,indent=2),encoding='utf8')
    results={}
    for name,chunks in json.loads(source.read_text(encoding='utf8')).items():
        p=Lines()
        for chunk in chunks:p.feed(chunk.encode('ascii'))
        results[name]=dict(records=p.records,syncs=p.syncs,rejected=p.rejected,unfinished=bytes(p.buf).decode('ascii',errors='replace'))
    (folder/'line_results.json').write_text(json.dumps(results,indent=2),encoding='utf8')
    trace=[]
    m=adc_fault();trace.append({'scenario':'adc_retry',**m.step(110)})
    for t in [111,112,113,213,214]:trace.append({'scenario':'adc_retry',**m.step(t,retry=t==111,eoc=t==214,raw=1500)})
    m=partial();trace.append({'scenario':'uart_retry',**m.step(158)})
    trace.append({'scenario':'uart_retry',**m.step(160,retry=True)})
    for t in range(161,174):trace.append({'scenario':'uart_retry',**m.step(t)})
    trace.append({'scenario':'uart_retry',**m.step(273)});trace.append({'scenario':'uart_retry',**m.step(274,eoc=True,raw=2500)})
    m=partial();trace.append({'scenario':'drain_fail',**m.step(160,retry=True,tc=False)});trace.append({'scenario':'drain_fail',**m.step(210,tc=False)})
    (folder/'recovery_trace.json').write_text(json.dumps(trace,indent=2),encoding='utf8')
    print(json.dumps(results));print('Saved injected policy trace and byte parser results. NOT real UART/ARM execution.')
if __name__=='__main__':
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(RecoveryTests))
    if not result.wasSuccessful():raise SystemExit(1)
    write_results()
