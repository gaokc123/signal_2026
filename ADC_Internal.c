#include "ADC_Internal.h"
#include "ti_msp_dl_config.h"

// 硬件定义 (根据 main.syscfg)
#define ADC_INST             ADC0
#define ADC_CHANNEL          DL_ADC12_INPUT_CHAN_0  // PA27 对应 ADC0_CH0
#ifdef ADC12_0_ADCMEM_0_REF_VOLTAGE_V
#define ADC_VREF_VOLTAGE     ((float)ADC12_0_ADCMEM_0_REF_VOLTAGE_V)
#else
#define ADC_VREF_VOLTAGE     3.3f
#endif

#define ADC_WAIT_GUARD       100000u

static void adc_internal_reinit(void) {
    DL_ADC12_reset(ADC12_0_INST);
    DL_ADC12_enablePower(ADC12_0_INST);
    delay_cycles(1000);

    SYSCFG_DL_ADC12_0_init();
    
    // 必须先关闭转换才能修改寄存器
    DL_ADC12_disableConversions(ADC12_0_INST);

    DL_ADC12_setSampleTime0(ADC12_0_INST, 32);
    DL_ADC12_setStartAddress(ADC12_0_INST, DL_ADC12_SEQ_START_ADDR_00);
    DL_ADC12_setEndAddress(ADC12_0_INST, DL_ADC12_SEQ_END_ADDR_00);

    DL_ADC12_disableDMATrigger(ADC12_0_INST,
        DL_ADC12_DMA_MEM0_RESULT_LOADED |
        DL_ADC12_DMA_MEM1_RESULT_LOADED |
        DL_ADC12_DMA_MEM2_RESULT_LOADED |
        DL_ADC12_DMA_MEM3_RESULT_LOADED |
        DL_ADC12_DMA_MEM4_RESULT_LOADED |
        DL_ADC12_DMA_MEM5_RESULT_LOADED |
        DL_ADC12_DMA_MEM6_RESULT_LOADED |
        DL_ADC12_DMA_MEM7_RESULT_LOADED |
        DL_ADC12_DMA_MEM8_RESULT_LOADED |
        DL_ADC12_DMA_MEM9_RESULT_LOADED |
        DL_ADC12_DMA_MEM10_RESULT_LOADED |
        DL_ADC12_DMA_MEM11_RESULT_LOADED);
    DL_ADC12_disableDMA(ADC12_0_INST);

    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_enableConversions(ADC12_0_INST);
}

void ADC_Internal_Init(void) {
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM60);
    adc_internal_reinit();
}

uint16_t ADC_Internal_Read(void) {
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    uint32_t guard = 0;
    while (DL_ADC12_getRawInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0u) {
        guard++;
        if (guard >= ADC_WAIT_GUARD) {
            break;
        }
    }
    if (guard >= ADC_WAIT_GUARD) {
        adc_internal_reinit();
        DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
        DL_ADC12_startConversion(ADC12_0_INST);
        guard = 0;
        while (DL_ADC12_getRawInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0u) {
            guard++;
            if (guard >= ADC_WAIT_GUARD) {
                break;
            }
        }
    }
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);

    return DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_0);
}

float ADC_Internal_GetVoltage(void) {
    uint16_t raw = ADC_Internal_Read();
    // 计算电压: (Raw / 4095) * VREF
    return ((float)raw / 4095.0f) * ADC_VREF_VOLTAGE;
}
