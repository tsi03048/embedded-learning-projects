EXTI button example for STM32F103VET6 / Arm Compiler 6.22
Open EXTI_AC6.uvproj and select 01_EXTI_Button. No extra main.c needed.
PC0: button to GND, internal pull-up; external 10k pull-up may be added.
Configure board, SWD probe, flash algorithm and shared pins before powering.
Observe exti0_services in debugger. Counts handler services, not button presses.
Validation: C sources compiled with AC6.22. In the author environment armasm
failed to launch due to a Windows DLL error; link and hardware unverified.
