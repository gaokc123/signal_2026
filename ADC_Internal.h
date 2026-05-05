#ifndef __ADC_INTERNAL_H__
#define __ADC_INTERNAL_H__

#include "ti_msp_dl_config.h"

void ADC_Internal_Init(void);
uint16_t ADC_Internal_Read(void);
float ADC_Internal_GetVoltage(void);

#endif
