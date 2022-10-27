Arduino Due 
-----------
Arduino Due is based on Atmel SAM3X8E Cortex M3 CPU. It has 96KB of SRAM and 512KB of flash memory of code.


## What works?
 * Pin
 * ADC
 * DAC
 * PWM
 * I2C
 * LED

## Building


#### 1) Requirements:

-	a) Cygwin (If you're compiling the code on Windows)
-	b) Install arm-non-eabi toolchain or use the one comes with Arduino IDE
-	c) Bossac program utility for flashing binary (also comes with the IDE)
-	d) Any Serial console client program: PuTTY, screen, minicom.
-	e) Windows also requires .inf driver. (provided)


#### 2) compiling & flashing:

* Clone the git repo

* ``cd`` into ``atmel-sam3x/``

To create a binary file, type
```
make
```

Once done, connect Arduino Due's Native port to computer and press **erase** button on the board. This will put the board into bootloader mode.

Note the COM port number on which the Due is connected and then type :
```
make upload port=PORT
```
for example, on Windows(Cygwin):
```
make upload port=COM30
```
and on Linux:
```
make upload port=ACM0
```


**NOTE:** On Windows you need to install drivers manually in order to get serial USB working. Reconnect the board, Open Device Manager and look for ``CDC Virtual com`` device,  right click and select ``Update driver``,
point to folder containing sam3x.inf file. This should install the driver.


Arduino Due is configured to give REPL on serial USB at baudrate of 115200. On Windows you can use PuTTY and screen on Linux. 

With PuTTY, click on “Session” in the left-hand panel, then click the “Serial” radio button on the right, then enter you COM port (eg COM30) in the “Serial Line” box. Finally, click the “Open” button.

On Linux type:
```
screen /dev/ttyACM0
```

To get the REPL and board info hit ``Ctrl+D``.
