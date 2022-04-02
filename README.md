# MMPSU_Control_App
This is an application to control the MMPSU over I2C from an embedded linux system (like a Raspbery Pi).

## Build the mmpsu daemon
Simply run `$ make` from the command line while in the top level directory. An executable called `mmpsu` will be created in the `bin/` directory.

## Install the daemon
`cd` into the `tools/` directory and run the `install.sh` script. If it doesn't work properly, contact the authors of this code.