#include <stdio.h>
#include "ti_msp_dl_config.h"
#include "Delay/Delay.h"
#include "AD9834/AD9834.h"
#include "FreqControl/FreqControl.h"
#include "AmplitudeControl/AmplitudeControl.h"

// 🔧 声明提前
void Clock_Set_80MHz(void);

int main(void) {
    // 1. 系统初始化
    SYSCFG_DL_init();
    delay_cycles(100);
    
    // 2. 初始化相关模块
    FreqControl_Init();        // 初始化频率控制模块
    AmplitudeControl_Init();   // 初始化幅度控制模块

    // 继电器初始状态：高电平 (未触发)
    DL_GPIO_setPins(GPIO_SWITCHES_Relay_control_PORT, GPIO_SWITCHES_Relay_control_PIN);

    // 4. 使能中断 (用于频率与幅度调节)
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_Timer_startCounter(TIMER_0_INST);

    while (1) {
        // 主循环：等待中断触发频率或幅度改变
    }
}

// ======================================================
// 继电器处理逻辑：按下切换状态
// ======================================================
static bool g_relay_state = true; // 记录当前继电器状态，true为高电平，false为低电平

void Relay_1_Toggle(void) {
    // Relay_control 作为继电器电平控制引脚
    // 默认高电平，低电平触发
    g_relay_state = !g_relay_state;
    if (g_relay_state) {
        DL_GPIO_setPins(GPIO_SWITCHES_Relay_control_PORT, GPIO_SWITCHES_Relay_control_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_SWITCHES_Relay_control_PORT, GPIO_SWITCHES_Relay_control_PIN);
    }
}

// ======================================================
// 按键处理逻辑：支持短按和长按连续触发
// ======================================================
#define LONG_PRESS_DELAY    50  // 长按触发延迟 (50 * 10ms = 500ms)
#define REPEAT_RATE         10  // 连续触发速率 (10 * 10ms = 100ms)

typedef struct {
    GPIO_Regs *port;
    uint32_t pin;
    void (*handler)(void);
    uint16_t count;
    bool canRepeat;     // 是否支持长按连发
} Key_State;

static Key_State keys[] = {
    {GPIOA, GPIO_SWITCHES_Amplitude_decrease_PIN, AmplitudeControl_Decrease, 0, true},
    {GPIOB, GPIO_SWITCHES_frequency_increase_PIN, FreqControl_IncreaseFreq, 0, true},
    {GPIOB, GPIO_SWITCHES_frequency_decrease_PIN, FreqControl_DecreaseFreq, 0, true},
    {GPIOB, GPIO_SWITCHES_Amplitude_increase_PIN, AmplitudeControl_Increase, 0, true},
    {GPIOB, GPIO_SWITCHES_Relay_Key_1_PIN, Relay_1_Toggle, 0, false} // 继电器不需要连发
};

void Key_Process(void) {
    bool anyPressed = false;
    
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (DL_GPIO_readPins(keys[i].port, keys[i].pin) == 0) { // 按键按下 (低电平)
            keys[i].count++;
            anyPressed = true;
            
            if (keys[i].handler != NULL) {
                // 1. 初次按下 (单次触发，增加到 3 次计数即 30ms 消除抖动)
                if (keys[i].count == 3) {
                    keys[i].handler();
                }
                // 2. 长按逻辑 (仅在支持连发时执行)
                else if (keys[i].canRepeat && keys[i].count >= LONG_PRESS_DELAY) {
                    if ((keys[i].count - LONG_PRESS_DELAY) % REPEAT_RATE == 0) {
                        keys[i].handler();
                    }
                }
            }
        } else {
            keys[i].count = 0; // 按键松开，重置计数
        }
    }

    // LED 指示灯逻辑
    if (anyPressed) {
        DL_GPIO_setPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_LEDS_PORT, GPIO_LEDS_USER_LED_1_PIN);
    }
}

// ======================================================
// 定时器中断服务程序：10ms 触发一次
// ======================================================
void TIMER_0_INST_IRQHandler(void) {
    switch (DL_Timer_getPendingInterrupt(TIMER_0_INST)) {
        case DL_TIMER_IIDX_ZERO:
            Key_Process();
            break;
        default:
            break;
    }
}