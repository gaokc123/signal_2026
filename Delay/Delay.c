#include "ti_msp_dl_config.h"
#include "Delay.h"

// 延时ms毫秒
void Delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        delay_cycles(CPUCLK_FREQ / 1000); // 每次延迟1ms
    }
}

// 延时us微秒
void Delay_us(uint32_t us)
{
    for (uint32_t i = 0; i < us; i++) {
        delay_cycles(CPUCLK_FREQ / 1000000); // 每次延迟1us
    }
}