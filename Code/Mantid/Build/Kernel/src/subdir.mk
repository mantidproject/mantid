################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Kernel/src/Algorithm.cpp 

OBJS += \
./Kernel/src/Algorithm.o 

CPP_DEPS += \
./Kernel/src/Algorithm.d 


# Each subdirectory must supply rules for building sources it contributes
Kernel/src/%.o: ../Kernel/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


