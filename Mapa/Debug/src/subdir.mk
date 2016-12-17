################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Interbloqueo.c \
../src/Interfaz_Grafica.c \
../src/Mapa.c \
../src/Mensajes.c \
../src/Pokenest.c \
../src/battle.c \
../src/factory.c 

OBJS += \
./src/Interbloqueo.o \
./src/Interfaz_Grafica.o \
./src/Mapa.o \
./src/Mensajes.o \
./src/Pokenest.o \
./src/battle.o \
./src/factory.o 

C_DEPS += \
./src/Interbloqueo.d \
./src/Interfaz_Grafica.d \
./src/Mapa.d \
./src/Mensajes.d \
./src/Pokenest.d \
./src/battle.d \
./src/factory.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2016-2c-Bash-Ketchum/tp-commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


