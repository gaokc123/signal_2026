#ifndef FREQ_CONTROL_H
#define FREQ_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

// 频率控制常量定义
#define FREQ_MIN_HZ    1000        // 最小频率 1000Hz
#define FREQ_MAX_HZ    3000000    // 最大频率 3MHz
#define FREQ_STEP_HZ   1000        // 频率步进 1000Hz
#define FREQ_DEFAULT   1000 // 默认频率

// 频率控制函数声明

/**
 * @brief 初始化频率控制模块
 */
void FreqControl_Init(void);

/**
 * @brief 增加频率
 */
void FreqControl_IncreaseFreq(void);

/**
 * @brief 减少频率
 */
void FreqControl_DecreaseFreq(void);

/**
 * @brief 获取当前频率
 * @return 当前频率值(Hz)
 */
uint32_t FreqControl_GetCurrentFreq(void);

/**
 * @brief 设置指定频率
 * @param freq 要设置的频率值(Hz)
 */
void FreqControl_SetFreq(uint32_t freq);

#endif // FREQ_CONTROL_H