# MMPSU_Control_App
This is an application to control the MMPSU over I2C from an embedded linux system (like a Raspbery Pi).

## Build the mmpsu daemon
Simply run `$ make` from the command line while in the top level directory. An executable called `mmpsu` will be created in the `bin/` directory.

## Install the daemon
`cd` into the `tools/` directory and run the `install.sh` script. If it doesn't work properly, contact the authors of this code.

## Node JS
You'll need a Node JS environment installed. For first time setup, run: 
```
$ npm install
```

## Running the whole thing
Now start the mmpsu daemon and the web application:
```
$ sudo service start mmpsud
$ npm start
```
Assuming you're on the same LAN as your RPi, navigate to: your_RPi_hostname.local:8080 to access the web app.