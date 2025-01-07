################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/Siul2_Port_Ip_Cfg.c \
../board/dcd_config.c \
../board/ivt_config.c \
../board/quadspi_config.c 

OBJS += \
./board/Siul2_Port_Ip_Cfg.o \
./board/dcd_config.o \
./board/ivt_config.o \
./board/quadspi_config.o 

C_DEPS += \
./board/Siul2_Port_Ip_Cfg.d \
./board/dcd_config.d \
./board/ivt_config.d \
./board/quadspi_config.d 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Standard S32DS C Compiler'
	arm-none-eabi-gcc "@board/Siul2_Port_Ip_Cfg.args" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


