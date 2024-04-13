# plcpi-rpi

Command Line, for PLC-Pi08 card



## Setup

Enable Raspberry Pi I2C communication by opening a terminal and typing:
```bash
~$ sudo raspi-config
```
Go to the *Interface Options* menu then *I2C* and enable the port.

## Usage

```bash
~$ git clone https://github.com/SequentMicrosystems/plcpi-rpi.git
~$ cd plcpi-rpi/
~/plcpi-rpi$ sudo make install
```

Now you can access all the functions of the relays board through the command "plcpi". Use -h option for help:
```bash
~$ plcpi -h
```

If you clone the repository any update can be made with the following commands:

```bash
~$ cd plcpi-rpi/  
~/plcpi-rpi$ git pull
~/plcpi-rpi$ sudo make install
``` 
