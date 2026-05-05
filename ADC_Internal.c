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

void ADC_Internal_Init(void) {
    DL_GPIO_initPeripheralAnalogFunction(IOMUX_PINCM60);
}

uint16_t ADC_Internal_Read(void) {
    DL_ADC12_setStartAddress(ADC12_0_INST, DL_ADC12_SEQ_START_ADDR_00);
    DL_ADC12_setEndAddress(ADC12_0_INST, DL_ADC12_SEQ_END_ADDR_00);

    if ((DL_ADC12_getStatus(ADC12_0_INST) & DL_ADC12_STATUS_CONVERSION_ACTIVE) == 0u) {
        DL_ADC12_startConversion(ADC12_0_INST);
    }

    uint32_t guard = 0;
    while ((DL_ADC12_getStatus(ADC12_0_INST) & DL_ADC12_STATUS_CONVERSION_ACTIVE) != 0u) {
        guard++;
        if (guard >= ADC_WAIT_GUARD) {
            break;
        }
    }

    return DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_0);
}

float ADC_Internal_GetVoltage(void) {
    uint16_t raw = ADC_Internal_Read();
    // 计算电压: (Raw / 4095) * VREF
    return ((float)raw / 4095.0f) * ADC_VREF_VOLTAGE;
}
