#ifndef SPI_AD7606_H
#define SPI_AD7606_H

#include "ti_msp_dl_config.h"
#include "arm_math.h"      // for q15_t

#define SAMPLES   1024           // 采样点数
#define AD7606_CH 1              // 通道数，后续可调整

// DMA 发送命令缓冲（1 通道）
extern uint16_t   AD7606_TX[1];
// DMA 接收原始 ADC 数据（多通道，连续存放）
extern int16_t    AD7606_RX[AD7606_CH * SAMPLES];
// DMA 完成标志
extern __IO uint8_t AD7606_RX_flag;

// 拆分后单通道数据缓冲
extern int16_t AD7606_CH1[SAMPLES];

// FIR/FFT 复数输入缓冲：实部/虚部交替存放
// 长度 = 2 * SAMPLES
extern q15_t      adc_cplx[2 * SAMPLES];

// 初始化 ADC+DMA+中断+定时器
void    AD7606_Init(void);
// 检查一帧数据是否接收完毕，拆分第1通道数据
uint8_t AD7606_DataReady(void);
// 打印原始 ADC DMA 缓冲
void Print_ADC_Raw(void);

#endif // SPI_AD7606_H
