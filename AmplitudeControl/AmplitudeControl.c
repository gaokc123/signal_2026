#include "AmplitudeControl.h"
#include <math.h>
#include "ti_msp_dl_config.h"

// ======================== Amplitude Control Module ========================

// --- 硬件与系统参数配置 ---
#define AD9834_VIN_VPP      0.107f      // 差分放大器后的实际正弦波输入幅度: 107mV
#define VOUT_MIN            0.02f       // 【修改】最小输出电压极限: 20mV (0.02V)
#define VOUT_MAX            7.0f        // 【修改】最大输出电压极限: 7.0V
#define VOUT_STEP           0.2f        // 步进值: 0.2V

// 静态变量存储当前目标幅度，建议初始值依然保持在安全的中间地带
static float currentVoutVpp = 2.0f;     // 默认初始目标依然为2.0V

/**
 * @brief 直接设置 DAC 输出电压
 * @param voltage_mv 目标电压 (单位: mV)
 */
void AmplitudeControl_SetDirectVoltage(uint32_t voltage_mv) {
    uint32_t dac_value;

    /* Set output voltage: 
     * DAC value (12-bits) = DesiredOutputVoltage x 4095 
     * ----------------------- 
     * ReferenceVoltage 
     */ 
    dac_value = (voltage_mv * 4095) / DAC12_REF_VOLTAGE_mV; 

    DL_DAC12_output12(DAC0, dac_value); 
    DL_DAC12_enable(DAC0); 
}

/**
 * @brief 更新DAC以控制AD603输出指定的峰峰值电压
 * @param targetVpp 期望的输出峰峰值 (1.0V ~ 3.0V)
 */
void AmplitudeControl_SetVoltage(float targetVpp) {
    // 1. 范围限制与防呆保护
    if (targetVpp < VOUT_MIN) targetVpp = VOUT_MIN;
    if (targetVpp > VOUT_MAX) targetVpp = VOUT_MAX;
    
    currentVoutVpp = targetVpp; // 更新当前状态

    // 2. 计算所需增益 (dB)
    // 公式: Gain(dB) = 20 * log10(Vout / Vin)
    float gain_db = 20.0f * log10f(currentVoutVpp / AD9834_VIN_VPP);

    // 3. 根据模块手册公式反推 VDA (mV)
    // 模块实测线性公式: G = -0.0402 * VDA + 64.539
    float vda_mv = (64.539f - gain_db) / 0.0402f;

    // 2. 使用补偿后的常数 (70.56) 反推 VDA
    // 这里的 70.56 是为了补偿 50欧姆阻抗失配带来的 6dB 压降
    //float vda_mv = (70.56f - gain_db) / 0.0402f;

    // 4. 硬件边界检查 (手册实测建议范围 126mV ~ 2121mV)
    if (vda_mv < 126.0f)  vda_mv = 126.0f;
    if (vda_mv > 2121.0f) vda_mv = 2121.0f;

    // 5. 转换为 12位 DAC 寄存器值 (0-4095)
    uint32_t dacCode = (uint32_t)((vda_mv / (float)DAC12_REF_VOLTAGE_mV) * 4095.0f);

    // 6. 将数字量写入 MSPM0 的 DAC0 模块
    DL_DAC12_output12(DAC0, dacCode);
}

/**
 * @brief 初始化幅度控制模块 (默认输出 2.0V)
 */
void AmplitudeControl_Init(void) {
    // 【修改】初始化时输出2.0V
    AmplitudeControl_SetVoltage(2.0f); 
}

/**
 * @brief 增加幅度 (步进 0.2V)
 */
void AmplitudeControl_Increase(void) {
    // 加上 0.05f 容差是为了防止浮点数精度误差导致加不到 3.0V
    if (currentVoutVpp + VOUT_STEP <= VOUT_MAX + 0.05f) { 
        AmplitudeControl_SetVoltage(currentVoutVpp + VOUT_STEP);
    }
}

/**
 * @brief 减少幅度 (步进 0.2V)
 */
void AmplitudeControl_Decrease(void) {
    if (currentVoutVpp - VOUT_STEP >= VOUT_MIN - 0.05f) {
        AmplitudeControl_SetVoltage(currentVoutVpp - VOUT_STEP);
    }
}

/**
 * @brief 获取当前设置的幅度值
 * @return 当前目标电压 Vpp
 */
float AmplitudeControl_GetCurrentVoltage(void) {
    return currentVoutVpp;
}