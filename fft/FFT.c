#include <math.h>
#include "arm_const_structs.h"
#include "arm_math.h"
#include "fft/FFT.h"
#include <stdio.h>
#define SAMPLES 1024
#define HALF_SAMPLES (SAMPLES/2 + 1)

// 输入：fir_output是int16_t的实数信号，长度SAMPLES
extern int16_t fir_output[SAMPLES];

// FFT复数输入缓冲区（实虚交替，长度2*SAMPLES）
q15_t fft_input[2 * SAMPLES];

// 双边幅度谱，长度SAMPLES
q15_t fft_mag_double[SAMPLES];

// 单边幅度谱，长度HALF_SAMPLES
q15_t fft_mag_single[HALF_SAMPLES];

// 打印数组（简易版）
static void print_array_q15(const char* name, const int16_t* arr, int len, int maxPrint) {
    printf("%s = [", name);
    int printLen = (len > maxPrint) ? maxPrint : len;
    for (int i = 0; i < printLen; i++) {
        printf("%d", arr[i]);
        if (i < printLen - 1) printf(", ");
    }
    if (len > maxPrint) printf(", ...");
    printf("]\n");
}

// 打印浮点窗函数系数，方便调试窗函数
static void print_hann_window(int samples) {
    printf("Hann window coefficients (first 16):\n[");
    for (int i = 0; i < 16 && i < samples; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (samples - 1)));
        printf("%.6f", w);
        if (i < 15 && i < samples-1) printf(", ");
    }
    printf("]\n");
}

void FFT_Run_With_Window(void) {
    printf("== FFT_Run_With_Window start ==\n");

    // 1) 汉宁窗加权 + 转复数格式（虚部置0）
    print_hann_window(SAMPLES);
    for (int i = 0; i < SAMPLES; i++) {
        float w = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (SAMPLES - 1)));
        int32_t val = (int32_t)(fir_output[i] * w);  // 加窗，Q15近似
        fft_input[2 * i] = (q15_t)val;                // 实部
        fft_input[2 * i + 1] = 0;                      // 虚部清零
    }
    print_array_q15("fft_input (real parts)", fft_input, 32, 32);

    // 2) 做1024点FFT，正变换，带位反转
    arm_cfft_q15(&arm_cfft_sR_q15_len1024, fft_input, 0, 1);
    print_array_q15("fft_input (post FFT, partial)", fft_input, 32, 32);

    // 3) 计算复数幅度谱，双边谱长度1024
    arm_cmplx_mag_q15(fft_input, fft_mag_double, SAMPLES);
    print_array_q15("fft_mag_double (magnitude, partial)", fft_mag_double, 32, 32);

    // 4) 转单边谱，长度513，直流和Nyquist点幅值不变，其余乘2
    fft_mag_single[0] = fft_mag_double[0];
    fft_mag_single[HALF_SAMPLES - 1] = fft_mag_double[SAMPLES / 2];
    for (int i = 1; i < SAMPLES / 2; i++) {
        fft_mag_single[i] = fft_mag_double[i] << 1;  // 乘2放大单边能量
    }
    print_array_q15("fft_mag_single (single side spectrum, partial)", fft_mag_single, 32, 32);

    printf("== FFT_Run_With_Window end ==\n");
}