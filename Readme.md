Raspberry Pi Pico EEPROM Programmer
======================
Code that allows pico to get files through usb via Xmodem/Ymodem and write recieved data to EEPROM ( AT28C64B )

Warning ! 3.3v vs 5v
---------------------
Raspberry Pi Pico is a 3.3v device while AT28C64B is 5V device and it means that to read from EEPROM you have to use logic level converter. Logic level converter should also be use to write to EEPROM but from what I have seen AT28C64B can be written to by 3.3V device

To do
-------
- Add Zmodem
- Add data polling, right now I'm waiting max time that it takes write cycle to complete
- Add proper reading from EEPROM (right now I don't have enough logic level converter)
- Figure out why suddenly log messages are not visible on terminal
- Clean and refactor code
- Add support for other memory types ( flash, sram )

Pins
----
GPIOs 0-12 are connected to A0-A12
GPIOs 13-20 are connected to I/O0 - I/O1
GPIO 21 is connected to WE
VSYS is connected to VCC and OE

Usage
-----
1. Build and flash program to pico
2. Connect pico through usb to the computer
3. Launch terminal emulator connect it to your pico ( screen /dev/<pico device name> )
4. Choose protocol
5. Send file through selected protocol
