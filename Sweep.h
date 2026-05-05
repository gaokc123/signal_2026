#ifndef SWEEP_H
#define SWEEP_H

#include <stdint.h>
#include <stdbool.h>

/* 扫频配置参数 */
typedef struct {
    uint32_t startFreq;
    uint32_t stopFreq;
    uint16_t pointsPerDecade; // 每 decade 的点数
    uint16_t stepDelayMs;     // 稳定等待时间
    uint16_t numSamples;      // 采样次数
    uint32_t stepHz;
    
    uint32_t currentFreq;
    bool isRunning;
    bool isComplete;
    uint16_t currentIndex;
    uint16_t totalPoints;
} Sweep_Config;

typedef struct {
    uint32_t frequency;
    float voltage;
} Sweep_Point;

/* 状态机子状态 */
typedef enum {
    SWEEP_SUB_IDLE = 0,
    SWEEP_SUB_SET_FREQ,
    SWEEP_SUB_WAIT_SETTLE,
    SWEEP_SUB_SAMPLING,
    SWEEP_SUB_WAIT_DMA,
    SWEEP_SUB_DONE
} Sweep_SubState;

void Sweep_Init(void);
void Sweep_Start(uint32_t start, uint32_t stop, uint16_t pointsPerDecade, uint16_t delayMs);
void Sweep_StartLinear(uint32_t start, uint32_t stop, uint32_t stepHz, uint16_t delayMs);
void Sweep_Stop(void);
bool Sweep_IsRunning(void);
bool Sweep_IsComplete(void);
void Sweep_ClearComplete(void);
void Sweep_Process(void);
void Sweep_DisplayOLED(void);
void Sweep_DisplayResultOLED(void);
const Sweep_Point* Sweep_GetResults(uint16_t* outCount);

#endif
