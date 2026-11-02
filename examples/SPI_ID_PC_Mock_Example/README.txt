SPI ID state machine PC mock tests
Two C files: spi_id_state.c and test_spi_id.c. test_spi_id.c includes the exact
implementation; compile test_spi_id.c only, not both files.
Example if GCC is installed: gcc -std=c99 -Wall -Wextra test_spi_id.c -o spi_test
Then run spi_test (Windows: .\spi_test.exe). Run from the extracted directory.
Tests: normal transfer, mismatch, timeout and CS low, injected OVR, clock wrap.
Mock test is not a hardware timing or SPI behavior validation.
Arm Compiler 6.22 individually compiled both implementation and test source;
the authoring environment has no working native C compiler: tests not executed.
Do not use the state machine for hardware without implementing verified ports
and a device-specific fault recovery policy. FAULT can leave CS low.
