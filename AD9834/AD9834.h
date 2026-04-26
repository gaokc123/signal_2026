#ifndef __AD9834_H
#define __AD9834_H

#include <stdint.h>

#define SINE_WAVE       0x2008
#define TRIANGLE_WAVE   0x2002
#define SQUARE_WAVE     0x2028

void AD9834_Init(void);
void AD9834_Set_Register(uint16_t reg);
void AD9834_Set_Frequency(uint32_t fre_value);
void AD9834_Set_Wave(uint16_t type);
void AD9834_SetOutput(uint32_t wave_freq, uint16_t wave_type);
#endif
