"""Injected-input teaching model, NOT execution/emulation of the ARM C firmware.
Run: python observe_button.py   (Python3 standard library only.)
Reads input.csv, writes trace.csv, and runs boundary/gap/wrap checks.
"""
from pathlib import Path
import csv,unittest

MASK=0xffffffff

class ButtonModel:
    def __init__(self,tick=0,raw=1):
        self.last=tick&MASK
        self.raw=self.candidate=self.stable=raw
        self.since=tick&MASK
        self.presses=self.led=self.gaps=0

    def observe(self,tick,raw):
        now=tick&MASK
        event=0
        if now!=self.last:
            self.raw=raw
            gap=(now-self.last)&MASK
            self.last=now
            if gap!=1 or raw!=self.candidate:
                if gap!=1:self.gaps+=1
                self.candidate=raw
                self.since=now
            elif self.candidate!=self.stable and ((now-self.since)&MASK)>=20:
                self.stable=self.candidate
                if self.stable==0:
                    self.presses+=1
                    self.led=1-self.led
                    event=1
        return {'tick':now,'raw':self.raw,'candidate':self.candidate,
                'candidate_age':(now-self.since)&MASK,'stable':self.stable,
                'press_event':event,'press_count':self.presses,
                'led':self.led,'sample_gaps':self.gaps}

class RulesTest(unittest.TestCase):
    def test_19_vs_20_intervals(self):
        b=ButtonModel()
        for t in range(1,21):b.observe(t,0)
        self.assertEqual((b.stable,b.presses),(1,0))
        b.observe(21,0)
        self.assertEqual((b.stable,b.presses),(0,1))

    def test_bounce_restarts_evidence(self):
        b=ButtonModel()
        for t,r in [(1,0),(2,1),(3,0)]:b.observe(t,r)
        for t in range(4,23):b.observe(t,0)
        self.assertEqual(b.presses,0)
        b.observe(23,0);self.assertEqual(b.presses,1)

    def test_hold_is_one_press(self):
        b=ButtonModel()
        for t in range(1,1001):b.observe(t,0)
        self.assertEqual((b.presses,b.led),(1,1))

    def test_release_then_next_press(self):
        b=ButtonModel()
        for t in range(1,22):b.observe(t,0)
        for t in range(22,43):b.observe(t,1)
        self.assertEqual((b.stable,b.presses),(1,1))
        for t in range(43,64):b.observe(t,0)
        self.assertEqual((b.presses,b.led),(2,0))

    def test_boot_held_is_not_an_event(self):
        b=ButtonModel(raw=0)
        for t in range(1,100):b.observe(t,0)
        self.assertEqual((b.presses,b.led),(0,0))
        for t in range(100,121):b.observe(t,1)
        for t in range(121,142):b.observe(t,0)
        self.assertEqual(b.presses,1)

    def test_short_pulse_is_ignored(self):
        b=ButtonModel()
        for t in range(1,20):b.observe(t,0)
        for t in range(20,60):b.observe(t,1)
        self.assertEqual((b.stable,b.presses),(1,0))

    def test_main_gap_restarts_evidence(self):
        b=ButtonModel()
        for t in range(1,11):b.observe(t,0)
        b.observe(50,0)
        self.assertEqual((b.presses,b.gaps,b.since),(0,1,50))
        for t in range(51,70):b.observe(t,0)
        self.assertEqual(b.presses,0)
        b.observe(70,0);self.assertEqual(b.presses,1)

    def test_unsigned_tick_wrap(self):
        start=0xfffffff0;b=ButtonModel(start)
        for d in range(1,22):b.observe(start+d,0)
        self.assertEqual((b.last,b.presses),(5,1))

    def test_same_tick_does_not_add_samples(self):
        b=ButtonModel();b.observe(1,0)
        for _ in range(1000):b.observe(1,0)
        self.assertEqual(b.presses,0)
        for t in range(2,22):b.observe(t,0)
        self.assertEqual(b.presses,1)

    def test_sampled_history_is_not_all_edges(self):
        b=ButtonModel()
        # An unobserved sub-tick pulse cannot appear in this input history.
        for t in range(1,100):b.observe(t,1)
        self.assertEqual(b.presses,0)

    def test_blink_boundary_wrap_and_late_service(self):
        last=0;led=0;events=[]
        for now in [0,499,500,999,1000,1499,1500,4000]:
            if ((now-last)&MASK)>=500:
                last=now;led=1-led;events.append(now)
        self.assertEqual(events,[500,1000,1500,4000])
        last=0xffffff00
        self.assertEqual((((last+500)&MASK)-last)&MASK,500)

def write_trace():
    base=Path(__file__).resolve().parent
    with (base/'input.csv').open(encoding='utf-8-sig',newline='') as f:
        rows=list(csv.DictReader(f))
    first=rows[0];model=ButtonModel(int(first['tick']),int(first['raw']))
    trace=[model.observe(int(r['tick']),int(r['raw'])) for r in rows]
    with (base/'trace.csv').open('w',encoding='utf8',newline='') as f:
        w=csv.DictWriter(f,fieldnames=trace[0].keys());w.writeheader();w.writerows(trace)
    events=[r['tick'] for r in trace if r['press_event']]
    print('Injected model press events:',events)
    print('Trace saved:',base/'trace.csv')
    print('This is a Python rule model; ARM C / physical switches / IRQ timing were NOT executed.')
    return events

if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(RulesTest)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():raise SystemExit(1)
    write_trace()
