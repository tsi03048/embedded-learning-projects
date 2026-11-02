"""Independent rule model, NOT execution of ARM C, peripheral or electrical hardware."""
from pathlib import Path
import csv,unittest
MASK=0xffffffff
def threshold(raw):return int(raw>=2600)
def hysteresis(raw,previous):
    if raw>=2600:return 1
    if raw<=2400:return 0
    return previous
def slot(now,last):
    elapsed=(now-last)&MASK
    if elapsed<100:return last,0,False
    periods=elapsed//100
    return (last+periods*100)&MASK,periods-1,True
def frame(raw,state):return f'raw={raw:04d},state={"W" if state else "N"}\r\n'.encode('ascii')
class RulesTest(unittest.TestCase):
    def test_single_boundary(self):self.assertEqual([threshold(x) for x in [2599,2600,2601]],[0,1,1])
    def test_hysteresis_enter(self):self.assertEqual(hysteresis(2600,0),1)
    def test_hysteresis_exit(self):self.assertEqual(hysteresis(2400,1),0)
    def test_band_remembers_both_histories(self):
        for previous in [0,1]:
            for raw in [2401,2500,2599]:self.assertEqual(hysteresis(raw,previous),previous)
    def test_chatter_comparison(self):
        history=[2599,2600,2599,2601,2598,2500,2400];state=0;states=[]
        for x in history:state=hysteresis(x,state);states.append(state)
        self.assertEqual(states,[0,1,1,1,1,1,0]);self.assertEqual([threshold(x) for x in history],[0,1,0,1,0,0,0])
    def test_boot_band_uses_explicit_normal_policy(self):self.assertEqual(hysteresis(2500,0),0)
    def test_full_scale_and_zero(self):self.assertEqual([hysteresis(x,0) for x in [0,4095]],[0,1])
    def test_estimated_mv_not_4095_divisor(self):self.assertEqual([x*3300//4096 for x in [0,2048,4095]],[0,1650,3299])
    def test_slot_boundary(self):
        self.assertEqual(slot(99,0),(0,0,False));self.assertEqual(slot(100,0),(100,0,True));self.assertEqual(slot(199,100),(100,0,False))
    def test_late_slot_uses_one_present_sample(self):
        self.assertEqual(slot(350,0),(300,2,True));self.assertEqual(slot(351,300),(300,0,False));self.assertEqual(slot(400,300),(400,0,True))
    def test_tick_wrap(self):
        last=0xffffffc0;now=(last+100)&MASK;self.assertEqual(slot(now,last),(now,0,True))
    def test_ascii_length_values_and_capacity(self):
        for raw in [0,1,2400,2600,4095]:
            for state in [0,1]:
                b=frame(raw,state);self.assertEqual(len(b),18);self.assertEqual(int(b[4:8]),raw);self.assertEqual(b[-2:],b'\r\n');self.assertNotIn(b'\0',b)
        self.assertAlmostEqual(18*10/(8000000/833)*1000,18.7425);self.assertLess(18*10/(8000000/833),0.1)
def write_trace():
    folder=Path(__file__).resolve().parent
    with (folder/'input.csv').open(encoding='utf-8-sig',newline='') as f:rows=list(csv.DictReader(f))
    state=0;trace=[];lines=[]
    for r in rows:
        tick=int(r['tick']);raw=int(r['raw']);assert 0<=raw<=4095
        state=hysteresis(raw,state)
        trace.append(dict(tick=tick,raw=raw,threshold_led=threshold(raw),hysteresis_led=state,estimated_mv=raw*3300//4096))
        lines.append(frame(raw,state))
    with (folder/'trace.csv').open('w',encoding='utf8',newline='') as f:
        w=csv.DictWriter(f,fieldnames=trace[0].keys());w.writeheader();w.writerows(trace)
    (folder/'expected_uart.txt').write_bytes(b''.join(lines))
    print('Rows:',len(trace),'UART:',[x.decode().strip() for x in lines[:4]])
    print('Saved trace.csv + expected_uart.txt; independent model, NOT ARM or real-board execution.')
if __name__=='__main__':
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(RulesTest))
    if not result.wasSuccessful():raise SystemExit(1)
    write_trace()
