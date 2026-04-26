#include "FIR.h"
#include "ti_msp_dl_config.h"
#include <string.h>
#include "SPI_AD7606.h"
#include "Delay/Delay.h"
#include <stdio.h>

// 可配置参数
#define FIR_TAPS      24      // FIR滤波器系数长度，即滤波器的阶数 + 1
#define QN            15      // Q格式参数，表示Q15格式，定点数小数点右移15位 (2^15=32768)


uint32_t gResult = 0;                  // 滤波器计算结果
// FIR滤波器系数，采用Q15格式存储，与设计的滤波器特性一致
static int16_t filterCoeff[FIR_TAPS] = 
{ 10, 254, 385, 592, 861, 1170, 1498, 1819, 2111, 2351, 2522, 2610, 2610, 2522, 2351, 2111, 1819, 1498, 1170, 861, 592, 385, 254,210};

// MathACL乘加操作配置，设置为有符号MAC操作，Q15格式
const DL_MathACL_operationConfig gMpyConfig = {
    .opType      = DL_MATHACL_OP_TYPE_MAC,       // 乘累加操作
    .opSign      = DL_MATHACL_OPSIGN_SIGNED,     // 有符号操作
    .iterations  = 0,                             // 迭代次数，0表示一次操作
    .scaleFactor = 0,                             // 缩放因子，这里不缩放
    .qType       = DL_MATHACL_Q_TYPE_Q16         // Q15格式定点运算  
};

// FIR状态延迟线，用于保存最近FIR_TAPS个输入样本
int16_t delayLine[FIR_TAPS];

// 滤波器输出数组，长度为样本数SAMPLES，存储每次滤波结果
int16_t fir_output[SAMPLES];

// =========== FIR 初始化（配置硬件乘累加单元） ===========
void FIR_Init(void) {
    // 配置MathACL模块为乘累加操作，使用之前定义的配置参数
    DL_MathACL_configOperation(MATHACL, &gMpyConfig, 0, 0);

    // 延迟线清零，初始状态无数据
    memset(delayLine, 0, sizeof(delayLine));
        // 先将从AD7606采集到的初始数据复制到delayLine前FIR_TAPS个元素中，作为初始状态
  //  memcpy(delayLine, AD7606_RX, FIR_TAPS * sizeof(delayLine[0]));
}

// =========== 整块滤波 ===========
// input: 指向输入信号缓冲区的指针（长度samples）
// output: 指向输出缓冲区的指针，存放滤波结果
// samples: 输入样本数
void FIR_Process_MathACL(int16_t* input, int16_t* output, int samples) {

     
    for (int n = 0; n < samples; n++) {
       // 每输入一个新样本，延迟线右移一位（将旧样本向后移动）
        memmove(&delayLine[1], delayLine,sizeof(delayLine) - sizeof(delayLine[0]));

        // 最新输入样本放入delayLine最前端（delayLine[0]）
        delayLine[0] = input[n];

        // 调试打印当前样本序号、输入值，以及延迟线前5个样本
        printf("n=%d, input=%d, delayLine[0..4]=", n, input[n]);
        for (int i = 0; i < 1; i++) {
            printf("%d ", delayLine[i]);
        }
        printf("\r\n");

        // 使用硬件乘累加单元逐个计算滤波器输出
        for (int k = 0; k < FIR_TAPS; k++) {
            // 设置第k个滤波系数作为乘数OperandTwo
            DL_MathACL_setOperandTwo(MATHACL, filterCoeff[k]);

            // 设置delayLine中对应样本作为乘数OperandOne
            DL_MathACL_setOperandOne(MATHACL, delayLine[k]);
              // 延时一小段时间，确保乘累加硬件操作完成
                Delay_us(2000);    
            // 等待硬件完成一次乘加运算
            DL_MathACL_waitForOperation(MATHACL);              
        }
        
        // 读取硬件乘累加结果（累加器值）
        int32_t  acc = DL_MathACL_getResultOne(MATHACL);

        //调试打印未移位的原始累加值，以及右移QN位后的定点结果
      // printf("n=%d, acc(raw)=%ld, acc(shifted)=%d\r\n", n, acc, (int)(acc >> QN));
   
        //右移QN位，将Q30格式结果转换为Q15格式，存入输出数组
        output[n] = (int16_t)(acc >> QN);

        // 清除硬件乘累加结果寄存器，准备下一次计算
        DL_MathACL_clearResults(MATHACL);
    }
   
}

