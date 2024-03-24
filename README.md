# btdrive - Atari 8-bit floppy emulator
This is an **ESP32** based Atari 8-bit floppy emulator, where disk images could be easily uploaded via standard bluetooth filetransfer, stored on an internal flash filesystem.

It is in early development state, and supports up to 4 drives now due to serial console control.
Write support may be dangerous!

## Console commands
```
f - format flash filesystem
l - list filesystem
d - list drives
t - task list
u - increase highspeed pokeydiv
i - decrease highspeed pokeydiv
n[x] [y] - insert image[x] to drive[y](1-4).
    Without drive defaults to 1(D1:). Without image defaults to 0
    (first image on filesystem, which is also inserted after boot).
```

## Hardware
Any of ESP32-Wroom boards with at least 2MB flash should work. 4MB is recommended to have more space on the flash filesystem.
ESP32-C* or -S* variants do not work, because they have only BLE(bluetooth low energy).

### Default pins
(for Atari SIO connection)
```
16 - RXD
17 - TXD
21 - Command
```
But could be changed via menuconfig

### Warning
Do not directly connect the Atari SIO port(5V) to the esp32, because of different logic levels. Esp is at 3,3V and may be damaged! Use a level shifter between.

## Make
This project has to be built with the **ESP-IDF**.
