#include "Sweep.h"
#include "ti_msp_dl_config.h"
#include "FreqControl/FreqControl.h"
#include "AD9834/AD9834.h"
#include "ADC_Internal.h"
#include "Delay/Delay.h"
#include "OLED/oled.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define MAX_SWEEP_POINTS 200
#define DEFAULT_NUM_SAMPLES 16
#define SWEEP_DMA_MAX_SAMPLES 64

#if defined(DMA_CH3_CHAN_ID)
#define SWEEP_USE_ADC_DMA 1
#define SWEEP_DMA_CHAN_ID DMA_CH3_CHAN_ID
#elif defined(DMA_CH2_CHAN_ID)
#define SWEEP_USE_ADC_DMA 1
#define SWEEP_DMA_CHAN_ID DMA_CH2_CHAN_ID
#else
#define SWEEP_USE_ADC_DMA 0
#endif

static Sweep_Config g_sweep;
static Sweep_SubState g_subState = SWEEP_SUB_IDLE;
static uint32_t g_freqList[MAX_SWEEP_POINTS];
static Sweep_Point g_results[MAX_SWEEP_POINTS];
static uint16_t g_resultCount = 0;
static float g_sampleSum = 0;
static uint16_t g_sampleCount = 0;
static uint32_t g_timerMs = 0;
static uint32_t g_lastOledUpdateMs = 0;
#if SWEEP_USE_ADC_DMA
static uint16_t g_adcDmaBuf[SWEEP_DMA_MAX_SAMPLES];
#endif

#define SWEEP_ADC_WAIT_GUARD 100000u

static void adc_start_conversion_and_wait(void) {
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_startConversion(ADC12_0_INST);

    uint32_t guard = 0;
    while (DL_ADC12_getRawInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED) == 0u) {
        guard++;
        if (guard >= SWEEP_ADC_WAIT_GUARD) {
            break;
        }
    }
    DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
}

#if SWEEP_USE_ADC_DMA
static uint16_t clamp_u16(uint16_t v, uint16_t minV, uint16_t maxV) {
    if (v < minV) return minV;
    if (v > maxV) return maxV;
    return v;
}

static bool is_adc_dma_done(void);

static void pump_adc_conversions_until_dma_done(uint16_t maxConversions) {
    uint16_t burst = clamp_u16(maxConversions, 1, 16);
    for (uint16_t i = 0; i < burst; i++) {
        if (is_adc_dma_done()) {
            break;
        }
        adc_start_conversion_and_wait();
    }
}

static void start_adc_dma(uint16_t samples) {
    uint8_t ch = (uint8_t)SWEEP_DMA_CHAN_ID;
    uint16_t n = clamp_u16(samples, 1, SWEEP_DMA_MAX_SAMPLES);

    DL_ADC12_disableConversions(ADC12_0_INST);

    DL_ADC12_enableDMA(ADC12_0_INST);
    DL_ADC12_setDMASamplesCnt(ADC12_0_INST, n);

    DL_DMA_disableChannel(DMA, ch);
    DL_DMA_setSrcAddr(DMA, ch, DL_ADC12_getMemResultAddress(ADC12_0_INST, DL_ADC12_MEM_IDX_0));
    DL_DMA_setDestAddr(DMA, ch, (uint32_t)g_adcDmaBuf);
    DL_DMA_setTransferSize(DMA, ch, n);

    DL_ADC12_enableDMATrigger(ADC12_0_INST, DL_ADC12_DMA_MEM0_RESULT_LOADED);

    DL_ADC12_enableConversions(ADC12_0_INST);

    DL_DMA_enableChannel(DMA, ch);
    DL_DMA_startTransfer(DMA, ch);
    g_timerMs = 0;
    g_lastOledUpdateMs = 0;
    adc_start_conversion_and_wait();
}

static bool is_adc_dma_done(void) {
    uint8_t ch = (uint8_t)SWEEP_DMA_CHAN_ID;
    return DL_DMA_getTransferSize(DMA, ch) == 0;
}

static void clear_adc_dma_done(void) {
    uint8_t ch = (uint8_t)SWEEP_DMA_CHAN_ID;
    DL_DMA_disableChannel(DMA, ch);
    
    DL_ADC12_disableConversions(ADC12_0_INST);
    DL_ADC12_disableDMATrigger(ADC12_0_INST, DL_ADC12_DMA_MEM0_RESULT_LOADED);
    DL_ADC12_disableDMA(ADC12_0_INST);
    DL_ADC12_enableConversions(ADC12_0_INST);
}

static float compute_avg_voltage_from_dma(uint16_t samples) {
    uint16_t n = clamp_u16(samples, 1, SWEEP_DMA_MAX_SAMPLES);
    uint32_t sum = 0;
    for (uint16_t i = 0; i < n; i++) {
        sum += (uint32_t)g_adcDmaBuf[i];
    }

    float avgRaw = (float)sum / (float)n;
#ifdef ADC12_0_ADCMEM_0_REF_VOLTAGE_V
    float vRef = (float)ADC12_0_ADCMEM_0_REF_VOLTAGE_V;
#else
    float vRef = 3.3f;
#endif
    return (avgRaw / 4095.0f) * vRef;
}

static float get_voltage_from_mem0(void) {
    uint16_t raw = (uint16_t)DL_ADC12_getMemResult(ADC12_0_INST, DL_ADC12_MEM_IDX_0);
#ifdef ADC12_0_ADCMEM_0_REF_VOLTAGE_V
    float vRef = (float)ADC12_0_ADCMEM_0_REF_VOLTAGE_V;
#else
    float vRef = 3.3f;
#endif
    return ((float)raw / 4095.0f) * vRef;
}
#endif

static void oled_write_page(uint8_t page, const uint8_t* data128) {
    OLED_SetPos(0, page);
    for (uint8_t x = 0; x < 128; x++) {
        WriteData(data128[x]);
    }
}

static void oled_draw_graph(const Sweep_Point* points, uint16_t count, float* outMinV, float* outMaxV) {
    if (count == 0) return;

    float vMin = points[0].voltage;
    float vMax = points[0].voltage;
    for (uint16_t i = 1; i < count; i++) {
        if (points[i].voltage < vMin) vMin = points[i].voltage;
        if (points[i].voltage > vMax) vMax = points[i].voltage;
    }

    float vRange = vMax - vMin;
    if (vRange < 0.001f) {
        vMin -= 0.001f;
        vMax += 0.001f;
        vRange = vMax - vMin;
    }

    if (outMinV) *outMinV = vMin;
    if (outMaxV) *outMaxV = vMax;

    enum { GRAPH_Y0_PAGE = 2, GRAPH_H = 48 };
    uint8_t buf[6][128];
    memset(buf, 0, sizeof(buf));

    int16_t prevY = -1;
    for (uint16_t x = 0; x < 128; x++) {
        uint16_t idx = (uint16_t)(((uint32_t)x * (uint32_t)(count - 1)) / 127u);
        float v = points[idx].voltage;
        float norm = (v - vMin) / vRange;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        int16_t y = (int16_t)((1.0f - norm) * (float)(GRAPH_H - 1) + 0.5f);
        if (y < 0) y = 0;
        if (y > (GRAPH_H - 1)) y = (GRAPH_H - 1);

        if (prevY >= 0) {
            int16_t y0 = prevY < y ? prevY : y;
            int16_t y1 = prevY < y ? y : prevY;
            for (int16_t yy = y0; yy <= y1; yy++) {
                uint8_t page = (uint8_t)(GRAPH_Y0_PAGE + (yy / 8));
                uint8_t bit = (uint8_t)(1u << (yy % 8));
                buf[page - GRAPH_Y0_PAGE][x] |= bit;
            }
        } else {
            uint8_t page = (uint8_t)(GRAPH_Y0_PAGE + (y / 8));
            uint8_t bit = (uint8_t)(1u << (y % 8));
            buf[page - GRAPH_Y0_PAGE][x] |= bit;
        }

        prevY = y;
    }

    for (uint8_t p = 0; p < 6; p++) {
        oled_write_page((uint8_t)(GRAPH_Y0_PAGE + p), buf[p]);
    }
}

static const char* classify_filter(const Sweep_Point* points, uint16_t count, float vMin, float vMax) {
    if (count < 6) return "Type: UNK";

    uint16_t n = 5;
    if (count < 10) n = (uint16_t)(count / 2);
    if (n < 3) n = 3;

    float lowSum = 0.0f;
    float highSum = 0.0f;
    for (uint16_t i = 0; i < n; i++) {
        lowSum += points[i].voltage;
        highSum += points[count - 1 - i].voltage;
    }
    float lowAvg = lowSum / (float)n;
    float highAvg = highSum / (float)n;

    float range = vMax - vMin;
    float delta = highAvg - lowAvg;
    float thresh = range * 0.20f;
    if (thresh < 0.02f) thresh = 0.02f;

    if (delta > thresh) return "Type: HIGH-P";
    if (delta < -thresh) return "Type: LOW-P ";
    return "Type: FLAT  ";
}

/* 生成对数频率列表 */
static uint16_t generate_freq_list(uint32_t start, uint32_t stop, uint16_t pointsPerDecade) {
    double startLog = log10((double)start);
    double endLog = log10((double)stop);
    double rangeLog = endLog - startLog;
    
    uint16_t numPoints = (uint16_t)(rangeLog * pointsPerDecade) + 1;
    if (numPoints > MAX_SWEEP_POINTS) numPoints = MAX_SWEEP_POINTS;
    
    for (uint16_t i = 0; i < numPoints; i++) {
        double logFreq = startLog + (rangeLog * i) / (numPoints - 1);
        g_freqList[i] = (uint32_t)(pow(10.0, logFreq) + 0.5);
    }
    return numPoints;
}

static uint16_t generate_freq_list_linear(uint32_t start, uint32_t stop, uint32_t stepHz) {
    if (stepHz == 0) return 0;
    if (start > stop) return 0;

    uint32_t freq = start;
    uint16_t count = 0;
    while ((freq <= stop) && (count < MAX_SWEEP_POINTS)) {
        g_freqList[count++] = freq;
        freq += stepHz;
    }
    return count;
}

void Sweep_Init(void) {
    memset(&g_sweep, 0, sizeof(g_sweep));
    memset(g_results, 0, sizeof(g_results));
    g_resultCount = 0;
    g_subState = SWEEP_SUB_IDLE;
}

void Sweep_Start(uint32_t start, uint32_t stop, uint16_t pointsPerDecade, uint16_t delayMs) {
    if (g_sweep.isRunning) return;
    
    g_sweep.startFreq = start;
    g_sweep.stopFreq = stop;
    g_sweep.pointsPerDecade = pointsPerDecade;
    g_sweep.stepDelayMs = delayMs;
    g_sweep.numSamples = DEFAULT_NUM_SAMPLES;
    g_sweep.stepHz = 0;
#if SWEEP_USE_ADC_DMA
    if (g_sweep.numSamples > SWEEP_DMA_MAX_SAMPLES) g_sweep.numSamples = SWEEP_DMA_MAX_SAMPLES;
#endif
    
    g_sweep.totalPoints = generate_freq_list(start, stop, pointsPerDecade);
    g_sweep.currentIndex = 0;
    g_sweep.isRunning = true;
    g_sweep.isComplete = false;
    g_resultCount = 0;
    g_timerMs = 0;
    g_lastOledUpdateMs = 0;
    g_sampleSum = 0;
    g_sampleCount = 0;
    OLED_Clear();
    
    g_subState = SWEEP_SUB_SET_FREQ;
}

void Sweep_StartLinear(uint32_t start, uint32_t stop, uint32_t stepHz, uint16_t delayMs) {
    if (g_sweep.isRunning) return;

    g_sweep.startFreq = start;
    g_sweep.stopFreq = stop;
    g_sweep.pointsPerDecade = 0;
    g_sweep.stepDelayMs = delayMs;
    g_sweep.numSamples = DEFAULT_NUM_SAMPLES;
    g_sweep.stepHz = stepHz;
#if SWEEP_USE_ADC_DMA
    if (g_sweep.numSamples > SWEEP_DMA_MAX_SAMPLES) g_sweep.numSamples = SWEEP_DMA_MAX_SAMPLES;
#endif

    g_sweep.totalPoints = generate_freq_list_linear(start, stop, stepHz);
    g_sweep.currentIndex = 0;
    g_sweep.isRunning = true;
    g_sweep.isComplete = false;
    g_resultCount = 0;
    g_timerMs = 0;
    g_lastOledUpdateMs = 0;
    g_sampleSum = 0;
    g_sampleCount = 0;
    OLED_Clear();

    g_subState = SWEEP_SUB_SET_FREQ;
}

void Sweep_Stop(void) {
    g_sweep.isRunning = false;
    g_subState = SWEEP_SUB_IDLE;
}

bool Sweep_IsRunning(void) {
    return g_sweep.isRunning;
}

bool Sweep_IsComplete(void) {
    return g_sweep.isComplete;
}

void Sweep_ClearComplete(void) {
    g_sweep.isComplete = false;
    if (!g_sweep.isRunning) {
        g_subState = SWEEP_SUB_IDLE;
    }
}

void Sweep_Process(void) {
    if (!g_sweep.isRunning) return;

    // 假设每 10ms 调用一次
    g_timerMs += 10;

    switch (g_subState) {
        case SWEEP_SUB_SET_FREQ:
            g_sweep.currentFreq = g_freqList[g_sweep.currentIndex];
            AD9834_SetOutput(g_sweep.currentFreq, SINE_WAVE);
            g_timerMs = 0;
            g_subState = SWEEP_SUB_WAIT_SETTLE;
            Sweep_DisplayOLED();
            break;

        case SWEEP_SUB_WAIT_SETTLE:
            if (g_timerMs >= g_sweep.stepDelayMs) {
                g_sampleSum = 0;
                g_sampleCount = 0;
#if SWEEP_USE_ADC_DMA
                start_adc_dma(g_sweep.numSamples);
                g_subState = SWEEP_SUB_WAIT_DMA;
#else
                g_subState = SWEEP_SUB_SAMPLING;
#endif
            }
            break;

        case SWEEP_SUB_SAMPLING:
        {
            uint16_t remaining = (g_sweep.numSamples > g_sampleCount) ? (uint16_t)(g_sweep.numSamples - g_sampleCount) : 0;
            uint16_t burst = remaining;
            if (burst > 4) burst = 4;
            for (uint16_t i = 0; i < burst; i++) {
                g_sampleSum += ADC_Internal_GetVoltage();
                g_sampleCount++;
            }
        }
            
            if (g_sampleCount >= g_sweep.numSamples) {
                float avgVolt = g_sampleSum / g_sampleCount;

                if (g_sweep.currentIndex < MAX_SWEEP_POINTS) {
                    g_results[g_sweep.currentIndex].frequency = g_sweep.currentFreq;
                    g_results[g_sweep.currentIndex].voltage = avgVolt;
                    if (g_resultCount < (g_sweep.currentIndex + 1)) {
                        g_resultCount = g_sweep.currentIndex + 1;
                    }
                }
                
                // 准备进入下一个点
                g_sweep.currentIndex++;
                if (g_sweep.currentIndex >= g_sweep.totalPoints) {
                    g_sweep.isRunning = false;
                    g_sweep.isComplete = true;
                    g_subState = SWEEP_SUB_DONE;
                    Sweep_DisplayResultOLED();
                } else {
                    g_subState = SWEEP_SUB_SET_FREQ;
                }
            }
            break;

#if SWEEP_USE_ADC_DMA
        case SWEEP_SUB_WAIT_DMA:
            pump_adc_conversions_until_dma_done(4);
            if (is_adc_dma_done()) {
                clear_adc_dma_done();
                float avgVolt = compute_avg_voltage_from_dma(g_sweep.numSamples);
                g_sampleSum = avgVolt;
                g_sampleCount = 1;

                if (g_sweep.currentIndex < MAX_SWEEP_POINTS) {
                    g_results[g_sweep.currentIndex].frequency = g_sweep.currentFreq;
                    g_results[g_sweep.currentIndex].voltage = avgVolt;
                    if (g_resultCount < (g_sweep.currentIndex + 1)) {
                        g_resultCount = g_sweep.currentIndex + 1;
                    }
                }

                g_sweep.currentIndex++;
                if (g_sweep.currentIndex >= g_sweep.totalPoints) {
                    g_sweep.isRunning = false;
                    g_sweep.isComplete = true;
                    g_subState = SWEEP_SUB_DONE;
                    Sweep_DisplayResultOLED();
                } else {
                    g_subState = SWEEP_SUB_SET_FREQ;
                }
            }
            else if ((g_timerMs - g_lastOledUpdateMs) >= 50) {
                g_lastOledUpdateMs = g_timerMs;
                g_sampleSum = get_voltage_from_mem0();
                g_sampleCount = 1;
                Sweep_DisplayOLED();
            }
            else if (g_timerMs >= 300) {
                clear_adc_dma_done();
                adc_start_conversion_and_wait();
                float avgVolt = get_voltage_from_mem0();
                g_sampleSum = avgVolt;
                g_sampleCount = 1;

                if (g_sweep.currentIndex < MAX_SWEEP_POINTS) {
                    g_results[g_sweep.currentIndex].frequency = g_sweep.currentFreq;
                    g_results[g_sweep.currentIndex].voltage = avgVolt;
                    if (g_resultCount < (g_sweep.currentIndex + 1)) {
                        g_resultCount = g_sweep.currentIndex + 1;
                    }
                }

                g_sweep.currentIndex++;
                if (g_sweep.currentIndex >= g_sweep.totalPoints) {
                    g_sweep.isRunning = false;
                    g_sweep.isComplete = true;
                    g_subState = SWEEP_SUB_DONE;
                    Sweep_DisplayResultOLED();
                } else {
                    g_subState = SWEEP_SUB_SET_FREQ;
                }
            }
            break;
#endif

        default:
            break;
    }
}

const Sweep_Point* Sweep_GetResults(uint16_t* outCount) {
    if (outCount != NULL) {
        *outCount = g_resultCount;
    }
    return g_results;
}

void Sweep_DisplayResultOLED(void) {
    uint16_t count = 0;
    const Sweep_Point* points = Sweep_GetResults(&count);
    if (count == 0) return;

    OLED_Clear();

    float vMin = 0.0f;
    float vMax = 0.0f;
    oled_draw_graph(points, count, &vMin, &vMax);

    OLED_ShowStr(0, 0, (unsigned char*)classify_filter(points, count, vMin, vMax), 2);
}

void Sweep_DisplayOLED(void) {
    char str[20];
    if (g_sweep.isRunning) {
        OLED_ShowStr(0, 0, "Sweeping...  ", 2);
        sprintf(str, "F:%6luHz", g_sweep.currentFreq);
        OLED_ShowStr(0, 2, str, 2);
        sprintf(str, "P:%3d/%d", g_sweep.currentIndex + 1, g_sweep.totalPoints);
        OLED_ShowStr(0, 4, str, 2);
        sprintf(str, "V:%.3fV", (g_sampleCount == 0) ? 0.0f : (g_sampleSum / g_sampleCount));
        OLED_ShowStr(0, 6, str, 2);
    } else if (g_sweep.isComplete) {
        Sweep_DisplayResultOLED();
    }
}
