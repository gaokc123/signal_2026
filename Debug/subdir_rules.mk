################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/Program/TI/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"F:/Gao/Desktop/Hzz/signal/AD7606" -I"F:/Gao/Desktop/Hzz/signal/FIR" -I"F:/Gao/Desktop/Hzz/signal" -I"F:/Gao/Desktop/Hzz/signal/OLED" -I"F:/Gao/Desktop/Hzz/signal/FFT" -I"F:/Gao/Desktop/Hzz/signal/Key" -I"F:/Gao/Desktop/Hzz/signal/Debug" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/third_party/CMSIS/Core/Include" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/third_party/CMSIS/DSP/Include" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-475031599: ../main.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"E:/Program/TI/ccs/utils/sysconfig_1.24.0/sysconfig_cli.bat" --script "F:/Gao/Desktop/Hzz/signal/main.syscfg" -o "." -s "E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/.metadata/product.json" -s "E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/.metadata/product.json" --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-475031599 ../main.syscfg
device.opt: build-475031599
device.cmd.genlibs: build-475031599
ti_msp_dl_config.c: build-475031599
ti_msp_dl_config.h: build-475031599
Event.dot: build-475031599

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/Program/TI/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"F:/Gao/Desktop/Hzz/signal/AD7606" -I"F:/Gao/Desktop/Hzz/signal/FIR" -I"F:/Gao/Desktop/Hzz/signal" -I"F:/Gao/Desktop/Hzz/signal/OLED" -I"F:/Gao/Desktop/Hzz/signal/FFT" -I"F:/Gao/Desktop/Hzz/signal/Key" -I"F:/Gao/Desktop/Hzz/signal/Debug" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/third_party/CMSIS/Core/Include" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/third_party/CMSIS/DSP/Include" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/Program/TI/ccs/tools/compiler/ti-cgt-armllvm_4.0.3.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"F:/Gao/Desktop/Hzz/signal/AD7606" -I"F:/Gao/Desktop/Hzz/signal/FIR" -I"F:/Gao/Desktop/Hzz/signal" -I"F:/Gao/Desktop/Hzz/signal/OLED" -I"F:/Gao/Desktop/Hzz/signal/FFT" -I"F:/Gao/Desktop/Hzz/signal/Key" -I"F:/Gao/Desktop/Hzz/signal/Debug" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/third_party/CMSIS/Core/Include" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source" -I"E:/Program/TI/SDK/mspm0_sdk_2_04_00_06/source/third_party/CMSIS/DSP/Include" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


