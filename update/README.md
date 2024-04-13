# update

This is the Home Automation Card firmware update tool.

## Usage

```bash
git clone https://github.com/SequentMicrosystems/plcpi-rpi.git
cd plcpi-rpi/update/
./update 0
```
For the Rock Pi SBC's runing debian OS replace the last line with:
```bash
~/plcpi-rpi/update$ ./rockupd 0
```

If you clone the repository already, skip the first step. 
The command will download the newest firmware version from our server and write it  to the board.
The stack level of the board must be provided as a parameter. 
During firmware update we strongly recommend to disconnect all outputs from the board since they can change state unpredictably.
