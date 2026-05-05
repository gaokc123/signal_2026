#include <stdio.h>
#include "ti_msp_dl_config.h"
#include "Delay/Delay.h"
#include "AD9834/AD9834.h"
#include "FreqControl/FreqControl.h"
#include "AmplitudeControl/AmplitudeControl.h"
#include "OLED/oled.h"
#include "ADC_Internal.h"
#include "Sweep.h"

// 🔧 声明提前
void Clock_Set_80MHz(void);
void Update_OLED_Display(void);
void Start_Sweep_Task(void);

#define SWEEP_KEY_PORT GPIOA
#define SWEEP_KEY_PIN  DL_GPIO_PIN_8
#define SWEEP_KEY_IOMUX IOMUX_PINCM19

int main(void) {
    // 1. 系统初始化
    SYSCFG_DL_init();
    delay_cycles(100);

    DL_GPIO_initDigitalInputFeatures(SWEEP_KEY_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    
    // 2. 初始化相关模块
    FreqControl_Init();        // 初始化频率控制模块
    AmplitudeControl_Init();   // 初始化幅度控制模块
    ADC_Internal_Init();       // 初始化内部ADC
    Sweep_Init();              // 初始化扫频模块

    // 继电器初始状态：高电平 (未触发)
    DL_GPIO_setPins(GPIO_SWITCHES_Relay_control_PORT, GPIO_SWITCHES_Relay_control_PIN);

    // 4. 使能中断 (用于频率与幅度调节)
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_Timer_startCounter(TIMER_0_INST);

    // 5. OLED 显示测试
    OLED_Init();
    OLED_Clear();
    {
        char str[24];
        for (uint8_t i = 0; i < 10; i++) {
            uint16_t raw = ADC_Internal_Read();
            float v = ((float)raw / 4095.0f) * (float)ADC12_0_ADCMEM_0_REF_VOLTAGE_V;

            OLED_ShowStr(0, 0, "ADC TEST", 1);
            sprintf(str, "RAW:%4u   ", (unsigned)raw);
            OLED_ShowStr(0, 2, (unsigned char*)str, 1);
            sprintf(str, "V:%.3fV   ", v);
            OLED_ShowStr(0, 4, (unsigned char*)str, 1);
            delay_cycles(3200000);
        }
        OLED_Clear();
    }
    Update_OLED_Display();

    while (1) {
        // 主循环：等待中断触发频率或幅度改变
    }
}

// ======================================================
// 继电器处理逻辑：按下切换状态
// ======================================================
static bool g_relay_state = true; // 记录当前继电器状态，true为高电平，false为低电平

void Update_OLED_Display(void) {
    char str[20];
    
    // 1. 显示标题
    OLED_ShowStr(0, 0, "--- SIGNAL ---", 1);
    
    // 2. 如果正在扫频，显示扫频界面
    if (Sweep_IsRunning()) {
        Sweep_DisplayOLED();
        return;
    }
    
    if (Sweep_IsComplete()) {
        Sweep_DisplayResultOLED();
        return;
    }

    // 3. 显示频率 (Hz 或 kHz)
    uint32_t freq = FreqControl_GetCurrentFreq();
    if (freq >= 1000) {
        sprintf(str, "Freq: %lu.%lu kHz ", freq / 1000, (freq % 1000) / 100);
    } else {
        sprintf(str, "Freq: %lu Hz    ", freq);
    }
    OLED_ShowStr(0, 2, str, 1);
    
    // 4. 显示幅度 (V)
    float amp = AmplitudeControl_GetCurrentVoltage();
    sprintf(str, "Amp:  %.2f V   ", amp);
    OLED_ShowStr(0, 4, str, 1);
    
    // 5. 显示继电器状态 (高通/低通)
    if (g_relay_state) {
        OLED_ShowStr(0, 6, "Filter: HIGH-P", 1);
    } else {
        OLED_ShowStr(0, 6, "Filter: LOW-P ", 1);
    }
}

void Start_Sweep_Task(void) {
    Sweep_StartLinear(1000, 100000, 1000, 20);
}

void Relay_1_Toggle(void) {
    // Relay_control 作为继电器电平控制引脚
    // 默认高电平，低电平触发
    g_relay_state = !g_relay_state;
    if (g_relay_state) {
        DL_GPIO_setPins(GPIO_SWITCHES_Relay_control_PORT, GPIO_SWITCHES_Relay_control_PIN);
    } else {
        DL_GPIO_clearPins(GPIO_SWITCHES_Relay_control_PORT, GPIO_SWITCHES_Relay_control_PIN);
    }
    Update_OLED_Display(); // 状态改变时刷新显示
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
    void (*longHandler)(void); // 新增：长按处理函数
    uint16_t count;
    uint8_t releaseCount;
    bool isPressed;
    bool canRepeat;     // 是否支持长按连发
} Key_State;

static Key_State keys[] = {
    {GPIOA, GPIO_SWITCHES_Amplitude_decrease_PIN, AmplitudeControl_Decrease, NULL, 0, 0, false, true},
    {GPIOB, GPIO_SWITCHES_frequency_increase_PIN, FreqControl_IncreaseFreq, NULL, 0, 0, false, true},
    {GPIOB, GPIO_SWITCHES_frequency_decrease_PIN, FreqControl_DecreaseFreq, NULL, 0, 0, false, true},
    {GPIOB, GPIO_SWITCHES_Amplitude_increase_PIN, AmplitudeControl_Increase, NULL, 0, 0, false, true},
    {GPIOB, GPIO_SWITCHES_Relay_Key_1_PIN, Relay_1_Toggle, NULL, 0, 0, false, false},
    {SWEEP_KEY_PORT, SWEEP_KEY_PIN, Start_Sweep_Task, NULL, 0, 0, false, false}
};

static void Exit_Sweep_Result_If_Needed(void (*action)(void)) {
    if (Sweep_IsComplete() && (action != Start_Sweep_Task)) {
        Sweep_ClearComplete();
        OLED_Clear();
    }
}

void Key_Process(void) {
    bool anyPressed = false;
    bool statusChanged = false;
    
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (DL_GPIO_readPins(keys[i].port, keys[i].pin) == 0) { // 按键按下 (低电平)
            keys[i].releaseCount = 0;
            keys[i].count++;
            anyPressed = true;
            
            if (keys[i].handler != NULL) {
                // 1. 初次按下 (单次触发，增加到 3 次计数即 30ms 消除抖动)
                if (!keys[i].isPressed && keys[i].count == 3) {
                    keys[i].isPressed = true;
                    Exit_Sweep_Result_If_Needed(keys[i].handler);
                    keys[i].handler();
                    statusChanged = true;
                }
                // 2. 长按逻辑
                else if (keys[i].isPressed && keys[i].count == LONG_PRESS_DELAY) {
                    if (keys[i].longHandler != NULL) {
                        Exit_Sweep_Result_If_Needed(keys[i].longHandler);
                        keys[i].longHandler();
                        statusChanged = true;
                    } else if (keys[i].canRepeat) {
                        Exit_Sweep_Result_If_Needed(keys[i].handler);
                        keys[i].handler();
                        statusChanged = true;
                    }
                }
                // 3. 连发逻辑
                else if (keys[i].isPressed && keys[i].canRepeat && keys[i].count > LONG_PRESS_DELAY) {
                    if ((keys[i].count - LONG_PRESS_DELAY) % REPEAT_RATE == 0) {
                        Exit_Sweep_Result_If_Needed(keys[i].handler);
                        keys[i].handler();
                        statusChanged = true;
                    }
                }
            }
        } else {
            if (keys[i].count != 0 || keys[i].isPressed) {
                keys[i].releaseCount++;
                if (keys[i].releaseCount >= 3) {
                    keys[i].count = 0;
                    keys[i].releaseCount = 0;
                    keys[i].isPressed = false;
                }
            } else {
                keys[i].releaseCount = 0;
            }
        }
    }

    if (statusChanged) {
        Update_OLED_Display();
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
            Sweep_Process(); // 扫频逻辑处理
            break;
        default:
            break;
    }
}
