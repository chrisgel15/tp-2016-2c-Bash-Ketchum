#!/bin/bash

cd /home/utnso/so-commons-library

sudo make uninstall

cd /home/utnso

sudo rm -rf so-commons-library/

sudo rm -rf /home/utnso/tp-2016-2c-Bash-Ketchum

cd /home/utnso/massive-file-creator

make clean

cd /home/utnso

sudo rm -rf massive-file-creator/

sudo rm -rf osada-utils/

export LD_LIBRARY_PATH = ""
