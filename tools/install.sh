#! /bin/bash

echo "switching to correct node version..."
# nvm use 16 > /dev/null

echo "Checking user groups..."

REQ_GROUPS=("gpio" "kmem" "i2c" "dialout")

for GRP in ${REQ_GROUPS[@]}; do
    if ! [ $(getent group $GRP | grep $USER) ]
    then
        echo "Adding $USER to group $GRP..."
        sudo usermod -aG $GRP $USER > /dev/null
    fi
done

APP_DIR=$(pwd)

if [[ "$APP_DIR" == *"tools"* ]]; then
    APP_DIR=$(readlink -f $APP_DIR/..)
fi

I2C_PATH=/dev/i2c-1

DEBUG_PATH=/dev/ttyAMA0

echo "\
[Unit]
Description=Repowered Robotics MMPSU daemon

[Service]
Type=simple
User=mmpsu
ExecStart=$APP_DIR/bin/mmpsu $I2C_PATH $DEBUG_PATH
Restart=always
RestartSec=1

[Install]
WantedBy=multi-user.target\
" | sudo tee /etc/systemd/system/mmpsud.service