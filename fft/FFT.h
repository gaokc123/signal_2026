#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <stdio.h>
// FFT点数
#define SAMPLES 1024
// 单边谱长度 = N/2 + 1
#define HALF_SAMPLES (SAMPLES / 2 + 1)

// 滤波后实数信号输入数组（Q15格式）
extern int16_t fir_output[SAMPLES];

// FFT复数输入缓冲区（实部+虚部交替）
extern int16_t fft_input[2 * SAMPLES];

// 双边幅度谱缓冲区，长度 SAMPLES
extern int16_t fft_mag_double[SAMPLES];

// 单边幅度谱缓冲区，长度 HALF_SAMPLES
extern int16_t fft_mag_single[HALF_SAMPLES];

/**
 * @brief 对fir_output信号加汉宁窗，执行1024点FFT，
 *        计算双边幅度谱，并转换为单边谱。
 */
void FFT_Run_With_Window(void);

/**
 * @brief 计算指定频点对应的频率（单位Hz）
 * @param binIndex 频点索引
 * @return 该频点对应频率值
 */
float FFT_GetFrequency(uint16_t binIndex);

/**
 * @brief 对幅度谱数据进行归一化处理，方便显示或比较
 * @param mag 输入幅度谱数组指针
 * @param length 数组长度
 * @param output 输出归一化结果数组指针（float数组）
 */
void FFT_NormalizeMagnitude(const int16_t* mag, uint16_t length, float* output);

static void print_array_q15(const char* name, const int16_t* arr, int len, int maxPrint);

static void print_hann_window(int samples);

#endif // FFT_H
