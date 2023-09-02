################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/logconfig.c \
../src/operaciones.c \
../src/socket.c 

C_DEPS += \
./src/logconfig.d \
./src/operaciones.d \
./src/socket.d 

OBJS += \
./src/logconfig.o \
./src/operaciones.o \
./src/socket.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-2c-KernelTitans/shares/include" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/logconfig.d ./src/logconfig.o ./src/operaciones.d ./src/operaciones.o ./src/socket.d ./src/socket.o

.PHONY: clean-src

