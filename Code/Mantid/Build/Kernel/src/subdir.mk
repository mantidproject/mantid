################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Kernel/src/Algorithm.cpp \
../Kernel/src/AnalysisDataService.cpp \
../Kernel/src/Workspace.cpp \
../Kernel/src/WorkspaceFactory.cpp 

OBJS += \
./Kernel/src/Algorithm.o \
./Kernel/src/AnalysisDataService.o \
./Kernel/src/Workspace.o \
./Kernel/src/WorkspaceFactory.o 

CPP_DEPS += \
./Kernel/src/Algorithm.d \
./Kernel/src/AnalysisDataService.d \
./Kernel/src/Workspace.d \
./Kernel/src/WorkspaceFactory.d 


# Each subdirectory must supply rules for building sources it contributes
Kernel/src/%.o: ../Kernel/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


