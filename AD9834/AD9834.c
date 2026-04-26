/*
 * AD9834波形发生器驱动
 * 提供AD9834芯片的初始化、频率设置和波形控制功能
 */
#include "AD9834.h"
#include "ti_msp_dl_config.h"
#include "Delay/Delay.h"  // 通用延时函数
#include "USART/USART.h"

// 同步信号低电平（拉低以开始SPI传输）
#define AD9834_SYNC_L()   DL_GPIO_clearPins(AD9834_SYNC_PORT, AD9834_SYNC_PIN)
// 同步信号高电平（拉高以结束SPI传输）
#define AD9834_SYNC_H()   DL_GPIO_setPins(AD9834_SYNC_PORT, AD9834_SYNC_PIN)
// 复位信号低电平（拉低复位AD9834）
#define AD9834_RST_L()    DL_GPIO_clearPins(AD9834_RST_PORT , AD9834_RST_PIN)
// 复位信号高电平（释放复位）
#define AD9834_RST_H()    DL_GPIO_setPins(AD9834_RST_PORT , AD9834_RST_PIN)

/**
 * @brief 向AD9834写入16位寄存器值
 * @param reg 要写入的16位寄存器值
 * @note 时序要求：
 *   - 先拉低SYNC信号开始传输
 *   - 通过SPI发送16位数据
 *   - 等待SPI传输完成
 *   - 拉高SYNC信号结束传输
 *   此操作确保AD9834正确接收控制命令
 */
void AD9834_Set_Register(uint16_t reg)
{
    AD9834_SYNC_L();  // 开始传输：拉低SYNC信号
    DL_SPI_transmitData16(SPI_1_INST, reg);  // 通过SPI1发送16位寄存器值
    while (DL_SPI_isBusy(SPI_1_INST));  // 等待SPI传输完成，确保数据发送完毕
    AD9834_SYNC_H();  // 结束传输：拉高SYNC信号
}

/**
 * @brief 设置输出频率
 * @param fre_value 28位频率控制字
 * @note 频率控制字写入规则：
 *   - 28位频率值分为低14位和高14位两次写入
 *   - 控制位设置：0x4000表示写入频率寄存器
 *   - 写入顺序：先低14位，再高14位
 */
void AD9834_Set_Frequency(uint32_t fre_value)
{
    // 提取低14位并设置控制位(0x4000表示写入频率寄存器)
    uint16_t fre_l = fre_value & 0x3FFF;  // 取低14位
    fre_l |= 0x4000;  // 设置控制位：01（选择频率寄存器）
    
    // 提取高14位并设置控制位
    uint16_t fre_h = (fre_value >> 14) & 0x3FFF;  // 取高14位
    fre_h |= 0x4000;  // 设置控制位
    
    // 依次写入频率寄存器
    AD9834_Set_Register(fre_l);  // 写入低字
    AD9834_Set_Register(fre_h);  // 写入高字
}

/**
 * @brief 设置输出波形类型
 * @param type 波形选择：SINE_WAVE(正弦波)/TRIANGLE_WAVE(三角波)/SQUARE_WAVE(方波)
 */
void AD9834_Set_Wave(uint16_t type)
{
    AD9834_Set_Register(type);  // 写入波形控制字
}

/**
 * @brief AD9834初始化
 * @note 执行复位序列并设置默认波形和相位
 * 复位时序说明：
 *   - 复位拉低1ms：确保芯片完全复位
 *   - 释放复位1ms：等待芯片稳定
 *   - 回到低电平100ms：确保芯片完全初始化
 *   此延时序列符合AD9834数据手册的复位要求
 */
void AD9834_Init(void)
{
    // 复位序列：低电平->高电平->低电平
    AD9834_RST_L();    // 复位拉低
    Delay_ms(1);       // 保持1ms确保复位有效
    
    AD9834_RST_H();    // 释放复位
    Delay_ms(1);       // 稳定时间
    AD9834_RST_L();    // 回到低电平
    Delay_ms(100);     // 等待芯片完全初始化(100ms)
    
    // 设置默认输出
    AD9834_Set_Wave(SINE_WAVE);      // 默认正弦波输出
    AD9834_Set_Register(0xC000);     // 设置初始相位为0度
}

/**
 * @brief 设置波形输出参数
 * @param wave_freq 输出频率(Hz)
 * @param wave_type 波形类型
 * @note 频率控制字计算公式：
 *   FREQ = (f_out * 2^28) / MCLK
 *   其中：
 *     - f_out: 目标输出频率
 *     - MCLK: 主时钟频率(75MHz)
 *     - 2^28: AD9834频率寄存器的分辨率
 *   计算结果为28位整数，用于设置输出频率
 */
void AD9834_SetOutput(uint32_t wave_freq, uint16_t wave_type)
{
    // 计算频率控制字（MCLK = 75MHz）
    uint32_t fre_code = (uint32_t)((double)wave_freq * (1UL << 28) / 75000000.0);
    
    // 设置输出参数
    AD9834_Set_Frequency(fre_code);  // 设置频率
    AD9834_Set_Wave(wave_type);      // 设置波形
}