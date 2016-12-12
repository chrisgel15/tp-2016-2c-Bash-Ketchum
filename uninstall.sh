#!/bin/bash

cd /home/utnso/so-commons-library

sudo make uninstall

cd /home/utnso

sudo rm -rf so-commons-library/

cd /home/utnso

sudo rm -rf massive-file-creator/

sudo rm -rf osada-utils/

sudo rm -rf osadaDisks/

sudo rm -rf /home/utnso/tp-2016-2c-Bash-Ketchum

export LD_LIBRARY_PATH=""
