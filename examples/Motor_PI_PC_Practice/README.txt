Motor PI teaching practice / PC Python 3.
Run in PowerShell: python pi_demo.py

Simplified simulated process, not a physical motor, RPM calibration, or firmware.
Target and gains are arbitrary. Time per step is unspecified and Ki is per step.
Output is clamped to 0..100; integral is NOT limited or protected against windup.
Not suitable for use on real motor hardware or for controller tuning.
