# PC practice model only; no sensor or I2C communication.
def read_temperature(status, msb, lsb, previous):
    if status != "OK":
        return status, previous
    raw12 = ((msb << 8) | lsb) >> 4
    steps = raw12 - 4096 if raw12 >= 2048 else raw12
    return "OK", steps * 0.0625

cases = [
    ("zero", "OK", 0x00, 0x00, "OK", 0.0),
    ("positive", "OK", 0x19, 0x00, "OK", 25.0),
    ("negative", "OK", 0xE7, 0x00, "OK", -25.0),
    ("nack", "NACK", 0x00, 0x00, "NACK", 123.0),
    ("timeout", "TIMEOUT", 0x00, 0x00, "TIMEOUT", 123.0),
]
failed = 0
for name, status, msb, lsb, expected_status, expected_value in cases:
    actual_status, value = read_temperature(status, msb, lsb, 123.0)
    passed = actual_status == expected_status and value == expected_value
    failed += not passed
    print(f"{name}: {'PASS' if passed else 'FAIL'} status={actual_status}, value={value}")
print(f"Result: {len(cases) - failed}/{len(cases)} passed")
raise SystemExit(1 if failed else 0)
