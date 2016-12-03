################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../metadata/entrenador.c \
../metadata/mapa.c \
../metadata/pokenest.c 

OBJS += \
./metadata/entrenador.o \
./metadata/mapa.o \
./metadata/pokenest.o 

C_DEPS += \
./metadata/entrenador.d \
./metadata/mapa.d \
./metadata/pokenest.d 


# Each subdirectory must supply rules for building sources it contributes
metadata/%.o: ../metadata/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


