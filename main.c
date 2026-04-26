#include <stdio.h>
#include "ti_msp_dl_config.h"
#include "Delay/Delay.h"
#include "AD9834/AD9834.h"
#include "FreqControl/FreqControl.h"
#include "AmplitudeControl/AmplitudeControl.h"

// 🔧 声明提前
void Clock_Set_80MHz(void);

int main(void) {
    // 1. 系统初始化
    SYSCFG_DL_init();
    delay_cycles(100);
    
    // 2. 初始化相关模块
    AD9834_Init();
    AD9834_SetOutput(5000, SINE_WAVE); 
    AmplitudeControl_Init();

    // 3. 设置 DAC 输出电压 (例如 1000mV)
    AmplitudeControl_SetDirectVoltage(1605);

    // 4. 使能 GPIO 中断 (用于频率与幅度调节)
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    NVIC_EnableIRQ(GPIOB_INT_IRQn);

    while (1) {
        // 主循环：等待中断触发频率或幅度改变
    }
}

// ======================================================
// GPIO 中断服务程序：处理频率和幅度调节
// ======================================================
void GROUP1_IRQHandler(void) {
    bool anyPressed = false;

    // --- 处理 GPIOA 中断 ---
    uint32_t gpioA_idx = DL_GPIO_getEnabledInterruptStatus(GPIOA,
                        GPIO_SWITCHES_Amplitude_decrease_PIN |
                        GPIO_SWITCHES_Relay_Key_2_PIN);

    // 幅度减少 (PA28)
    if (gpioA_idx & GPIO_SWITCHES_Amplitude_decrease_PIN) {
        if (DL_GPIO_readPins(GPIOA, GPIO_SWITCHES_Amplitude_decrease_PIN) == 0) {
            AmplitudeControl_Decrease();
        }
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_SWITCHES_Amplitude_decrease_PIN);
    }

    // 继电器按键 2 (PA12)
    if (gpioA_idx & GPIO_SWITCHES_Relay_Key_2_PIN) {
        if (DL_GPIO_readPins(GPIOA, GPIO_SWITCHES_Relay_Key_2_PIN) == 0) {
            // TODO: 处理继电器 2 逻辑
        }
        DL_GPIO_clearInterruptStatus(GPIOA, GPIO_SWITCHES_Relay_Key_2_PIN);
    }

    // --- 处理 GPIOB 中断 ---
    uint32_t gpioB_idx = DL_GPIO_getEnabledInterruptStatus(GPIOB,
                        GPIO_SWITCHES_frequency_increase_PIN | 
                        GPIO_SWITCHES_frequency_decrease_PIN | 
                        GPIO_SWITCHES_Amplitude_increase_PIN |
                        GPIO_SWITCHES_Relay_Key_1_PIN);

    // 频率增加 (PB6)
    if (gpioB_idx & GPIO_SWITCHES_frequency_increase_PIN) {
        if (DL_GPIO_readPins(GPIOB, GPIO_SWITCHES_frequency_increase_PIN) == 0) {
            FreqControl_IncreaseFreq();
        }
        DL_GPIO_clearInterruptStatus(GPIOB, GPIO_SWITCHES_frequency_increase_PIN);
    }
    
    // 频率减少 (PB14)
    if (gpioB_idx & GPIO_SWITCHES_frequency_decrease_PIN) {
        if (DL_GPIO_readPins(GPIOB, GPIO_SWITCHES_frequency_decrease_PIN) == 0) {
            FreqControl_DecreaseFreq();
        }
        DL_GPIO_clearInterruptStatus(GPIOB, GPIO_SWITCHES_frequency_decrease_PIN);
    }

    // 幅度增加 (PB12)
    if (gpioB_idx & GPIO_SWITCHES_Amplitude_increase_PIN) {
        if (DL_GPIO_readPins(GPIOB, GPIO_SWITCHES_Amplitude_increase_PIN) == 0) {
            AmplitudeControl_Increase();
        }
        DL_GPIO_clearInterruptStatus(GPIOB, GPIO_SWITCHES_Amplitude_increase_PIN);
    }

    // 继电器按键 1 (PB13)
    if (gpioB_idx & GPIO_SWITCHES_Relay_Key_1_PIN) {
        if (DL_GPIO_readPins(GPIOB, GPIO_SWITCHES_Relay_Key_1_PIN) == 0) {
            // TODO: 处理继电器 1 逻辑
        }
        DL_GPIO_clearInterruptStatus(GPIOB, GPIO_SWITCHES_Relay_Key_1_PIN);
    }

    // --- LED 反馈逻辑 ---
    // 检查是否有任何按键被按下 (低电平)
    if ((DL_GPIO_readPins(GPIOA, GPIO_SWITCHES_Amplitude_decrease_PIN | GPIO_SWITCHES_Relay_Key_2_PIN) != 
         (GPIO_SWITCHES_Amplitude_decrease_PIN | GPIO_SWITCHES_Relay_Key_2_PIN)) ||
        (DL_GPIO_readPins(GPIOB, GPIO_SWITCHES_frequency_increase_PIN | GPIO_SWITCHES_frequency_decrease_PIN | GPIO_SWITCHES_Amplitude_increase_PIN | GPIO_SWITCHES_Relay_Key_1_PIN) != 
         (GPIO_SWITCHES_frequency_increase_PIN | GPIO_SWITCHES_frequency_decrease_PIN | GPIO_SWITCHES_Amplitude_increase_PIN | GPIO_SWITCHES_Relay_Key_1_PIN))) {
        anyPressed = true;
    }

    if (anyPressed) {
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN); // 点亮 LED
    } else {
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN); // 熄灭 LED
    }
}