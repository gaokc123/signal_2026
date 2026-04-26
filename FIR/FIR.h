#ifndef FIR_H
#define FIR_H

#include "ti_msp_dl_config.h"

/*
 * FIR滤波器输出数组声明
 * 外部定义于 FIR.c，存储滤波后的数据
*/


extern int16_t fir_output[];

/*
 * FIR_Init
 *  初始化FIR滤波器相关资源
 *  - 配置硬件乘累加模块（MathACL）参数
 *  - 初始化延迟线状态
 */
void FIR_Init(void);

/*
 * FIR_Process_MathACL
 *  执行整块输入数据的FIR滤波
 *  参数:
 *    input  - 指向输入数据数组的指针，长度为samples，通常是ADC采样值
 *    output - 指向输出数据数组的指针，长度为samples，存放滤波后的结果
 *    samples- 输入数据样本数
 *  说明:
 *    本函数利用硬件乘累加器（MathACL）对输入信号做FIR滤波，
 *    其中滤波系数和延迟线由FIR模块内部维护。
 */
void FIR_Process_MathACL(int16_t* input, int16_t* output, int samples);
void FIR_Process(void);

#endif

// 使用示例：
// FIR_Init();                            // 初始化FIR滤波器
// FIR_Process_MathACL(AD7606_RX, fir_output, SAMPLES);  // 执行滤波
