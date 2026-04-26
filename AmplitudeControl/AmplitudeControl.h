#ifndef AMPLITUDECONTROL_H
#define AMPLITUDECONTROL_H

#include <stdint.h>

// MSPM0 DAC 参考电压 (单位: mV)
// 2500 表示使用内部 2.5V 基准，3300 表示使用 VCC (3.3V)
#define DAC12_REF_VOLTAGE_mV    3300 

// 初始化幅度模块
void AmplitudeControl_Init(void);

// 直接设置 DAC 输出电压 (单位: mV)
void AmplitudeControl_SetDirectVoltage(uint32_t voltage_mv);

// 设置指定的电压幅度 (范围 2.0V ~ 4.0V)
void AmplitudeControl_SetVoltage(float targetVpp);

// 以 0.2V 为步进增加幅度
void AmplitudeControl_Increase(void);

// 以 0.2V 为步进减少幅度
void AmplitudeControl_Decrease(void);

// 获取当前设置的电压幅度
float AmplitudeControl_GetCurrentVoltage(void);

#endif // AMPLITUDECONTROL_H