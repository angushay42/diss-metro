# diss-metro
Repository for my undergraduate practical work project.

## Development Notes


 - Use open-ocd to start a server on my machine (mac)
    - `openocd -f interface/stlink.cfg -f target/stm32f4x.cfg`
- this should open a connection to the MCU from my machine via USB
- then, use arm-none-eabi-gdb to load a program and run through it

- gdb will flash the program to the MCU and so any subsequent runs will have the program already loaded 
