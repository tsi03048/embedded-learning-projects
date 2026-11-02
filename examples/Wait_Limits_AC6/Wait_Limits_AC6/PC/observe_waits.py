"""Injected rule model. Does NOT execute ARM C or emulate ADC/NVIC/UART/baud."""
from pathlib import Path
import csv,unittest
MASK=0xffffffff
IDLE,WAIT_ADC,SEND,WAIT_TC,ERROR=range(5)
def decision(ready,age,limit):
    if ready and age<=limit:return 'accept'
    if age>=limit:return 'timeout'
    return 'wait'
class Model:
    def __init__(self,uart=True):
        self.uart=uart;self.phase=IDLE;self.error=0;self.error_tick=0;self.since=0;self.last_slot=0
        self.raw=65535;self.valid=0;self.alarm=0;self.samples=0;self.submitted=0;self.confirmed=0;self.skipped=0;self.frame=b'';self.prefix=b'';self.dr_reads=0
    def step(self,tick,eoc=0,txe=0,tc=0,raw=0):
        if self.phase==IDLE:
            age=(tick-self.last_slot)&MASK
            if age>=100:
                periods=age//100;self.last_slot=(self.last_slot+periods*100)&MASK;self.skipped+=periods-1
                self.valid=0;self.since=tick;self.phase=WAIT_ADC
        elif self.phase==WAIT_ADC:
            d=decision(eoc,(tick-self.since)&MASK,10)
            if d=='accept':
                self.raw=raw;self.dr_reads+=1;self.samples+=1;self.valid=1
                if raw>=2600:self.alarm=1
                elif raw<=2400:self.alarm=0
                if self.uart:
                    self.frame=f'raw={raw:04d},state={"W" if self.alarm else "N"}\r\n'.encode();self.submitted=0;self.prefix=b'';self.since=tick;self.phase=SEND
                else:self.phase=IDLE
            elif d=='timeout':self.error=3;self.error_tick=tick;self.phase=ERROR
        elif self.phase==SEND:
            d=decision(txe,(tick-self.since)&MASK,50)
            if d=='accept':
                self.prefix+=self.frame[self.submitted:self.submitted+1];self.submitted+=1;self.since=tick
                if self.submitted==18:self.phase=WAIT_TC
            elif d=='timeout':self.error=4;self.error_tick=tick;self.phase=ERROR
        elif self.phase==WAIT_TC:
            d=decision(tc,(tick-self.since)&MASK,50)
            if d=='accept':self.confirmed+=1;self.phase=IDLE
            elif d=='timeout':self.error=5;self.error_tick=tick;self.phase=ERROR
        return dict(tick=tick,eoc=eoc,txe=txe,tc=tc,phase=self.phase,error=self.error,raw=self.raw,valid=self.valid,submitted=self.submitted,confirmed=self.confirmed,samples=self.samples,skipped=self.skipped)
def ready_sender():
    m=Model();m.step(100);m.step(101,eoc=1,raw=2600);return m
class WaitTests(unittest.TestCase):
    def test_adc_before_deadline(self):
        m=Model(False);m.step(100);m.step(109,eoc=1,raw=2600);self.assertEqual((m.valid,m.raw,m.dr_reads,m.error),(1,2600,1,0))
    def test_adc_ready_at_boundary_wins(self):
        m=Model(False);m.step(100);m.step(110,eoc=1,raw=0);self.assertEqual((m.valid,m.raw,m.error),(1,0,0))
    def test_adc_absent_at_boundary(self):
        m=Model();m.step(100);m.step(110);self.assertEqual((m.error,m.valid,m.dr_reads),(3,0,0))
    def test_late_first_ready_is_rejected(self):
        m=Model();m.step(100);m.step(111,eoc=1,raw=2000);self.assertEqual((m.error,m.dr_reads),(3,0))
    def test_old_result_not_new_acquisition(self):
        m=Model();m.raw=3000;m.valid=1;m.alarm=1;m.step(100);self.assertEqual((m.raw,m.valid,m.alarm),(3000,0,1));m.step(110);self.assertEqual((m.raw,m.valid,m.alarm),(3000,0,1))
    def test_zero_is_valid_data(self):
        m=Model(False);m.step(100);m.step(101,eoc=1,raw=0);self.assertEqual((m.valid,m.raw,m.samples),(1,0,1))
    def test_late_eoc_after_error_does_not_read(self):
        m=Model();m.step(100);m.step(110);m.step(111,eoc=1,raw=3000);self.assertEqual((m.phase,m.dr_reads,m.error),(ERROR,0,3))
    def test_unsigned_wrap(self):
        m=Model();m.phase=WAIT_ADC;m.since=0xfffffffc;m.step(6,eoc=1,raw=0);self.assertEqual(m.error,0);self.assertEqual(m.valid,1)
    def test_frozen_tick_does_not_timeout(self):
        m=Model();m.step(100)
        for _ in range(1000):m.step(100)
        self.assertEqual((m.phase,m.error),(WAIT_ADC,0))
    def test_txe_ready_at50_accepts_one(self):
        m=ready_sender();m.step(151,txe=1);self.assertEqual((m.submitted,m.error),(1,0))
    def test_txe_missing_at50(self):
        m=ready_sender();m.step(151);self.assertEqual((m.error,m.submitted),(4,0))
    def test_txe_ready_after50_rejected(self):
        m=ready_sender();m.step(152,txe=1);self.assertEqual((m.error,m.submitted),(4,0))
    def test_partial_prefix_four_submissions(self):
        m=ready_sender()
        for t in [102,104,106,108]:m.step(t,txe=1)
        m.step(158);self.assertEqual((m.prefix,m.submitted,m.confirmed,m.error),(b'raw=',4,0,4))
    def test_all_submitted_not_confirmed(self):
        m=ready_sender()
        for t in range(102,138,2):m.step(t,txe=1)
        self.assertEqual((m.submitted,m.phase,m.confirmed),(18,WAIT_TC,0))
    def test_tc_ready_at50(self):
        m=ready_sender()
        for t in range(102,138,2):m.step(t,txe=1)
        m.step(186,tc=1);self.assertEqual((m.confirmed,m.error),(1,0))
    def test_tc_missing_at50(self):
        m=ready_sender()
        for t in range(102,138,2):m.step(t,txe=1)
        m.step(186);self.assertEqual((m.submitted,m.confirmed,m.error),(18,0,5))
    def test_uart_error_keeps_accepted_measurement(self):
        m=ready_sender();m.step(151);self.assertEqual((m.valid,m.raw,m.samples,m.error),(1,2600,1,4))
    def test_inactivity_budget_is_not_total_report_budget(self):
        m=ready_sender()
        for n in range(18):m.step(101+49*(n+1),txe=1)
        m.step(101+49*18+1,tc=1);self.assertEqual((m.confirmed,m.error),(1,0))
    def test_calibration_observation_policy(self):
        self.assertEqual([decision(r,a,10) for r,a in [(True,9),(True,10),(False,10),(True,11)]],['accept','accept','timeout','timeout'])
    def test_timer_boot_poll_bound_not_time(self):
        def boot(ready_at):
            for n in range(100000):
                if n==ready_at:return True,n+1
            return False,100000
        self.assertEqual(boot(0),(True,1));self.assertEqual(boot(None),(False,100000))
def scenarios():
    normal=[(100,0,0,0,0),(101,1,0,0,2600)]+[(t,0,1,0,0) for t in range(102,138,2)]+[(138,0,0,1,0)]
    adc=[(100,0,0,0,0),(109,0,0,0,0),(110,0,0,0,0),(111,1,0,0,3000)]
    txe=[(100,0,0,0,0),(101,1,0,0,2600)]+[(t,0,1,0,0) for t in [102,104,106,108]]+[(157,0,0,0,0),(158,0,0,0,0)]
    tc=normal[:-1]+[(185,0,0,0,0),(186,0,0,0,0),(187,0,0,1,0)]
    return {'normal':normal,'adc_missing':adc,'txe_after4':txe,'tc_missing':tc}
def write_results():
    folder=Path(__file__).resolve().parent
    if not (folder/'input.csv').exists():
        with (folder/'input.csv').open('w',encoding='utf8',newline='') as f:
            w=csv.writer(f);w.writerow(['scenario','tick','eoc','txe','tc','raw'])
            for name,rows in scenarios().items():
                for row in rows:w.writerow([name,*row])
    with (folder/'input.csv').open(encoding='utf8',newline='') as f:inputs=list(csv.DictReader(f))
    models={};trace=[]
    for r in inputs:
        name=r.pop('scenario');m=models.setdefault(name,Model());row=m.step(**{k:int(v) for k,v in r.items()});trace.append({'scenario':name,**row})
    with (folder/'trace.csv').open('w',encoding='utf8',newline='') as f:
        w=csv.DictWriter(f,fieldnames=trace[0].keys());w.writeheader();w.writerows(trace)
    summary=[]
    for name,m in models.items():
        summary.append(f'{name}: error={m.error}, raw={m.raw}, valid={m.valid}, DR submitted={m.submitted}, finalTC confirmations={m.confirmed}, prefix={m.prefix!r}')
    (folder/'EXPECTED.txt').write_text('\n'.join(summary)+'\nInjected status observations, NOT physical waveforms or ARM execution.\n',encoding='utf8');print('\n'.join(summary))
if __name__=='__main__':
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(WaitTests))
    if not result.wasSuccessful():raise SystemExit(1)
    write_results()
