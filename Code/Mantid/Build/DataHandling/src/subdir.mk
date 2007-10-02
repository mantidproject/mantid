################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../DataHandling/src/LoadRaw.cpp 

OBJS += \
./DataHandling/src/LoadRaw.o 

CPP_DEPS += \
./DataHandling/src/LoadRaw.d 


# Each subdirectory must supply rules for building sources it contributes
DataHandling/src/%.o: ../DataHandling/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


