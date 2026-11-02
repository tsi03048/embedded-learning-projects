# Teaching-only toy model: not motor firmware or a hardware simulator.
TARGET = 60.0        # arbitrary speed points, not RPM
RESPONSE = 0.6      # assumed response per simulated step
KP = 1.0
STEPS = 40

def run(ki):
    speed = 0.0
    integral = 0.0
    print(f"\nKp={KP:.1f}, Ki per step={ki:.1f}, target={TARGET:.1f}")
    print("step  duty(%)  speed_after  integral")
    for step in range(1, STEPS + 1):
        error = TARGET - speed
        integral += ki * error
        duty = max(0.0, min(100.0, KP * error + integral))
        speed += RESPONSE * (duty - speed)
        if step <= 5 or step in (10, 20, 40):
            print(f"{step:4d}  {duty:7.1f}  {speed:11.1f}  {integral:8.1f}")

if __name__ == "__main__":
    run(0.0)
    run(0.2)
