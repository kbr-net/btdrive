# btdrive - Atari 8-bit floppy emulator
This is an esp32 based Atari 8-bit floppy emulator, where disk images could be easily uploaded via standard bluetooth filetransfer, stored on an internal flash filesystem.

It is in early development state and supports read only now.

## Hardware
Any of ESP32-Wroom boards with at least 2MB flash should work. 4MB is recommended to have more space on the flash filesystem.
ESP32-C* or -S* variants do not work, because they have only BLE(bluetooth low energy).

## Warning
Do not directly connect the Atari SIO port(5V) to the esp32, because of different logic levels. Esp is at 3,3V and may be damaged! Use a level shifter between.

## Make
This project has to be built with the **ESP-IDF**.
