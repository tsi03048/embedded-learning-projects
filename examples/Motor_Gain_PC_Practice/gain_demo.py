# Educational toy model: not a motor driver, not a hardware simulation.
TARGET = 60.0          # arbitrary speed points, not RPM
RESPONSE = 0.6         # assumed response per step
STEPS = 10

def run(gain):
    speed = 0.0
    print(f"\ngain={gain:.1f}, target={TARGET:.1f}")
    print("step  duty(%)  speed_after")
    for step in range(1, STEPS + 1):
        error = TARGET - speed
        duty = max(0.0, min(100.0, gain * error))
        speed += RESPONSE * (duty - speed)
        print(f"{step:4d}  {duty:7.1f}  {speed:11.1f}")

if __name__ == "__main__":
    run(1.0)
    run(4.0)
