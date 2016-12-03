#!/bin/bash
sudo apt-get update
sudo apt-get install openssl
sudo apt-get install libssl-dev

# Instalacion de las commons

cd /home/utnso

git clone https://github.com/sisoputnfrba/so-commons-library

cd so-commons-library

sudo make install

#Preparo archivo del disco

cd /home/utnso

mkdir osadaDisks

cd osadaDisks

truncate -s 5MB test1.bin

# Instalacion de osada-utils

cd /home/utnso

git clone https://github.com/sisoputnfrba/osada-utils

cd osada-utils

sudo ./osada-format /home/utnso/osadaDisks/test1.bin

# Instalacion de massive-file-decriptor

cd /home/utnso

git clone https://github.com/sisoputnfrba/massive-file-creator

cd massive-file-creator

make

# Instalacion del TP

cd /home/utnso

git clone https://github.com/sisoputnfrba/tp-2016-2c-Bash-Ketchum

git checkout desarrollo

cd /home/utnso/tp-2016-2c-Bash-Ketchum/tp-commons/Debug

make all

echo "Instaladas tp-commons"

#cd /home/utnso/tp-2016-2c-Bash-Ketchum/Entrenador/Debug

#make all

#echo "Instalado el Entrenador"

#cd /home/utnso/tp-2016-2c-Bash-Ketchum/Mapa/Debug

#make all

#echo "Instalado el Mapa"

cd /home/utnso/tp-2016-2c-Bash-Ketchum/PokedexCliente/Debug

make all

echo "Instalada el PokedexCliente"

cd /home/utnso/tp-2016-2c-Bash-Ketchum/PokedexServidor/Debug

make all

echo "Instalada la PokedexServidor"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utnso/tp-2016-2c-Bash-Ketchum/tp-commons/Debug

echo "Modificada la variable LD_LIBRARY_PATH"
