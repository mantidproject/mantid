################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Algorithms/src/SimpleIntegration.cpp 

OBJS += \
./Algorithms/src/SimpleIntegration.o 

CPP_DEPS += \
./Algorithms/src/SimpleIntegration.d 


# Each subdirectory must supply rules for building sources it contributes
Algorithms/src/%.o: ../Algorithms/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


