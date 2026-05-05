#include "FreqControl.h"
#include "../AD9834/AD9834.h"
#include "../Debug/ti_msp_dl_config.h"


// ======================== Frequency Control Module ========================

// 静态变量存储当前频率
static uint32_t currentFreqHz = FREQ_MIN_HZ;

// 内联函数：更新频率并设置硬件
static inline void update_frequency_and_hardware(uint32_t new_freq)
{
    currentFreqHz = new_freq;
    AD9834_SetOutput(currentFreqHz, SINE_WAVE);  // 设置频率和波形
    // LED指示频率变化
    DL_GPIO_togglePins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
}

/**
 * @brief 初始化频率控制模块
 */
void FreqControl_Init(void) {
    currentFreqHz = FREQ_DEFAULT;
    AD9834_Init();
    // 设置AD9834的初始频率和波形
    AD9834_SetOutput(currentFreqHz, SINE_WAVE);
}

/**
 * @brief 增加频率
 */
void FreqControl_IncreaseFreq(void) {
    uint32_t new_freq = currentFreqHz + FREQ_STEP_HZ;
    if (new_freq <= FREQ_MAX_HZ) {
        update_frequency_and_hardware(new_freq);
       // UART_SetValue("n1.val", new_freq);
    }
}

/**
 * @brief 减少频率
 */
void FreqControl_DecreaseFreq(void) {
    if (currentFreqHz > FREQ_MIN_HZ) {
        uint32_t new_freq = currentFreqHz - FREQ_STEP_HZ;
        if (new_freq >= FREQ_MIN_HZ) {
            update_frequency_and_hardware(new_freq);
           // UART_SetValue("n1.val", new_freq);
        }
    }
}

/**
 * @brief 获取当前频率
 * @return 当前频率值(Hz)
 */
uint32_t FreqControl_GetCurrentFreq(void) {
    return currentFreqHz;
}

/**
 * @brief 设置指定频率
 * @param freq 要设置的频率值(Hz)
 */
void FreqControl_SetFreq(uint32_t freq) {
    if (freq >= FREQ_MIN_HZ && freq <= FREQ_MAX_HZ) {
        currentFreqHz = freq;
        AD9834_SetOutput(currentFreqHz, SINE_WAVE);  // 设置频率和波形
    }
}
