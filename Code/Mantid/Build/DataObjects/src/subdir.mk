################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../DataObjects/src/Histogram1D.cpp \
../DataObjects/src/Workspace1D.cpp 

OBJS += \
./DataObjects/src/Histogram1D.o \
./DataObjects/src/Workspace1D.o 

CPP_DEPS += \
./DataObjects/src/Histogram1D.d \
./DataObjects/src/Workspace1D.d 


# Each subdirectory must supply rules for building sources it contributes
DataObjects/src/%.o: ../DataObjects/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


