# Educational toy model only: not a motor driver or tuning guidance.
KP = 1.0
KI_PER_STEP = 0.2
RESPONSE = 0.6
STEPS = 50

def run(use_conditional_integration):
    speed = 0.0
    integral = 0.0
    label = "conditional" if use_conditional_integration else "unlimited"
    print(f"\nmethod={label}")
    print("step target duty speed_after integral")
    for step in range(1, STEPS + 1):
        target = 140.0 if step <= 30 else 30.0
        error = target - speed
        candidate = integral + KI_PER_STEP * error
        candidate_output = KP * error + candidate
        pushes_high = candidate_output > 100.0 and error > 0.0
        pushes_low = candidate_output < 0.0 and error < 0.0
        if not use_conditional_integration or not (pushes_high or pushes_low):
            integral = candidate
        duty = max(0.0, min(100.0, KP * error + integral))
        speed += RESPONSE * (duty - speed)
        if step in (1, 10, 20, 30, 31, 32, 35, 40, 50):
            print(f"{step:4d} {target:6.1f} {duty:5.1f} {speed:11.1f} {integral:8.1f}")

if __name__ == "__main__":
    run(False)
    run(True)
