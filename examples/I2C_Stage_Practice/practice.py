# Educational event model: no I2C, no hardware timing, no actual sensor.
class ReadModel:
    def __init__(self):
        self.stage = "prepare"
        self.pending = []
        self.result = None
        self.error = None

    def feed(self, event, byte=None):
        if self.stage in ("done", "failed"):
            return
        if event in ("nack", "timeout"):
            self.error = (self.stage, event)
            self.pending = []
            self.stage = "failed"
            return
        if self.stage == "prepare" and event == "ready":
            self.stage = "select"
        elif self.stage == "select" and event == "selected":
            self.stage = "request"
        elif self.stage == "request" and event == "address_ack":
            self.stage = "receive"
        elif self.stage == "receive" and event == "byte" and type(byte) is int and 0 <= byte <= 255:
            self.pending.append(byte)
            if len(self.pending) == 2:
                self.stage = "finish"
        elif self.stage == "finish" and event == "end_confirmed":
            raw = ((self.pending[0] << 8) | self.pending[1]) >> 4
            steps = raw - 4096 if raw >= 2048 else raw
            self.result = steps * 0.0625
            self.stage = "done"
        else:
            self.error = (self.stage, "unexpected_event")
            self.pending = []
            self.stage = "failed"


def run(events):
    model = ReadModel()
    for event, byte in events:
        model.feed(event, byte)
    return model

prefix = [("ready", None), ("selected", None), ("address_ack", None)]
cases = [
    ("zero", prefix + [("byte", 0), ("byte", 0), ("end_confirmed", None)],
     ("done", None, 0.0)),
    ("25C", prefix + [("byte", 0x19), ("byte", 0), ("end_confirmed", None)],
     ("done", None, 25.0)),
    ("address_nack", [("ready", None), ("nack", None)],
     ("failed", ("select", "nack"), None)),
    ("partial_timeout", prefix + [("byte", 0x19), ("timeout", None)],
     ("failed", ("receive", "timeout"), None)),
    ("finish_timeout", prefix + [("byte", 0x19), ("byte", 0), ("timeout", None)],
     ("failed", ("finish", "timeout"), None)),
    ("early_end", prefix + [("byte", 0x19), ("end_confirmed", None)],
     ("failed", ("receive", "unexpected_event"), None)),
    ("late_end", prefix + [("byte", 0x19), ("byte", 0), ("timeout", None), ("end_confirmed", None)],
     ("failed", ("finish", "timeout"), None)),
]
failures = 0
for name, events, expected in cases:
    model = run(events)
    actual = (model.stage, model.error, model.result)
    passed = actual == expected
    failures += not passed
    print(f"{name}: {'PASS' if passed else 'FAIL'} {actual}")
print(f"Result: {len(cases) - failures}/{len(cases)} passed")
raise SystemExit(1 if failures else 0)
