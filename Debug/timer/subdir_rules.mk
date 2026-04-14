################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
timer/%.o: ../timer/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"C:/TI/ccs2020/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/BNO08X_UART_RVC" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/Ultrasonic_GPIO" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/Ultrasonic_Capture" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/OLED_Hardware_I2C" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/OLED_Hardware_SPI" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/OLED_Software_I2C" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/OLED_Software_SPI" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/MPU6050" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/motor" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/encoder" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Debug" -I"C:/TI/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"C:/TI/mspm0_sdk_2_05_01_00/source" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/MSPM0" -I"C:/TI/mspm0_sdk_2_05_01_00/examples/nortos/LP_MSPM0G3507/driverlib/gpio_input_capture/mpu6050-oled-hardware-i2c/Drivers/WIT" -DMOTION_DRIVER_TARGET_MSPM0 -DMPU6050 -D__MSPM0G3507__ -gdwarf-3 -MMD -MP -MF"timer/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


