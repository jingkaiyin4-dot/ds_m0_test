################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
app/%.o: ../app/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/TI/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/BNO08X_UART_RVC" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/Ultrasonic_GPIO" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/Ultrasonic_Capture" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/OLED_Hardware_I2C" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/OLED_Hardware_SPI" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/OLED_Software_I2C" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/OLED_Software_SPI" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/MPU6050" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/PID" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/UART" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/motor" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/encoder" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Debug" -I"C:/TI/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"C:/TI/mspm0_sdk_2_05_01_00/source" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/MSPM0" -I"D:/TI-DS-M0/mpu6050-oled-hardware-i2c-20260412-192940/Drivers/WIT" -DMOTION_DRIVER_TARGET_MSPM0 -DMPU6050 -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"app/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


