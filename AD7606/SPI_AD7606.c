#include "SPI_AD7606.h"
#include "ti_msp_dl_config.h"
#include "Delay/Delay.h"
#include <stdio.h>


// 全局缓冲：最小化，只留 DMA 需要的
uint16_t   AD7606_TX[1];
int16_t    AD7606_RX[AD7606_CH * SAMPLES];
int16_t    AD7606_CH1[SAMPLES];
__IO uint8_t AD7606_RX_flag = 0;


// FIR/FFT 复数输入缓冲
q15_t      adc_cplx[2 * SAMPLES];

// void AD7606_Init(void) {
//     // 填充读取命令
//     AD7606_TX[0] = 0xFFFF;

//     // 硬件复位序列
//     DL_GPIO_clearPins(AD7606_PORT, AD7606_REST_PIN);
//     Delay_ms(1);
//     DL_GPIO_setPins  (AD7606_PORT, AD7606_REST_PIN);
//     Delay_ms(1);
//     DL_GPIO_clearPins(AD7606_PORT, AD7606_REST_PIN);
//     Delay_ms(1);

//     // 配置 DMA 发送（CH1）
//     DL_DMA_setSrcAddr (DMA, DMA_CH1_CHAN_ID, (uint32_t)AD7606_TX);
//     DL_DMA_setDestAddr(DMA, DMA_CH1_CHAN_ID, (uint32_t)&SPI_0_INST->TXDATA);
//     DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, 1);

//     // 配置 DMA 接收（CH0）
//     DL_DMA_setSrcAddr (DMA, DMA_CH0_CHAN_ID, (uint32_t)&SPI_0_INST->RXDATA);
//     DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t)AD7606_RX);
//     DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, SAMPLES);

//     // 清空 RX FIFO
//     while (!DL_SPI_isRXFIFOEmpty(SPI_0_INST)) {
//         DL_SPI_receiveData16(SPI_0_INST);
//     }

//     // 启用 DMA 接收 + 中断 + 定时器
//     DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
//     NVIC_EnableIRQ(SPI_0_INST_INT_IRQN);
//     DL_Interrupt_clearGroup(AD7606_INT_IIDX, AD7606_Busy_IIDX);
//     NVIC_EnableIRQ(AD7606_INT_IRQN);
//     DL_TimerG_startCounter(PWM_0_INST);
// }

uint8_t AD7606_DataReady(void) {
    if (!AD7606_RX_flag) 
        return 0;

   

    // 假设定义：
    // AD7606_CH = 通道数
    // SAMPLES = 采样点数
    // AD7606_RX[AD7606_CH * SAMPLES] 存储所有通道数据
    // AD7606_CH1[SAMPLES] 存储拆分出来的单通道数据

    // 只提取第1通道（索引0）数据
    for (int i = 0; i < SAMPLES; i++) {
        AD7606_CH1[i] = AD7606_RX[i * AD7606_CH + 0];
    }
    
     // 清标志，表示已处理
    AD7606_RX_flag = 0;
    return 1;
}


// BUSY 中断：触发下一次 DMA
// void GROUP1_IRQHandler(void) {
//     if (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1) == AD7606_INT_IIDX) {
//         DL_GPIO_clearPins(AD7606_PORT, AD7606_CS_PIN);
//         while (!DL_SPI_isRXFIFOEmpty(SPI_0_INST))
//             DL_SPI_receiveData16(SPI_0_INST);
//         DL_DMA_setTransferSize(DMA, DMA_CH1_CHAN_ID, 1);
//         DL_DMA_enableChannel(DMA, DMA_CH1_CHAN_ID);
//         DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, SAMPLES);
//         DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
//     }
// }

// SPI DMA 完成中断：拉高 CS 并置标志
void SPI_0_INST_IRQHandler(void) {
    if (DL_SPI_getPendingInterrupt(SPI_0_INST) == DL_SPI_IIDX_DMA_DONE_RX) {
        DL_GPIO_setPins(AD7606_PORT, AD7606_CS_PIN);
        AD7606_RX_flag = 1;
    }
}


void Print_ADC_Raw(void) {
    //printf("=== ADC Raw Data (%d samples) ===\r\n", SAMPLES);
    for (int i = 0; i < SAMPLES; i++) {
        // 下标 4 位对齐，数值 6 位对齐
        printf("RAW[%4d] = %6d\r\n", i, AD7606_RX[i]);
    }
    printf("=== End ===\r\n");
}