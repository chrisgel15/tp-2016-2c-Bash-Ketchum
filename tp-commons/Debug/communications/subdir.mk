################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../communications/checkReads.c \
../communications/createClientSocket.c \
../communications/createFdSets.c \
../communications/createSocket.c \
../communications/doAccept.c \
../communications/doBind.c \
../communications/doConnect.c \
../communications/doListen.c \
../communications/ltnCommons.c 

OBJS += \
./communications/checkReads.o \
./communications/createClientSocket.o \
./communications/createFdSets.o \
./communications/createSocket.o \
./communications/doAccept.o \
./communications/doBind.o \
./communications/doConnect.o \
./communications/doListen.o \
./communications/ltnCommons.o 

C_DEPS += \
./communications/checkReads.d \
./communications/createClientSocket.d \
./communications/createFdSets.d \
./communications/createSocket.d \
./communications/doAccept.d \
./communications/doBind.d \
./communications/doConnect.d \
./communications/doListen.d \
./communications/ltnCommons.d 


# Each subdirectory must supply rules for building sources it contributes
communications/%.o: ../communications/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


