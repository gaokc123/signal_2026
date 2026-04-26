## 示例概述

通过硬件切换寄存器控制三个 GPIO 引脚的切换。

## 外设与引脚分配总结

### 主要外设引脚分配
| 控制器/外设 | 引脚      | 功能                 | 映射关系          |
| ----------- | --------- | -------------------- | ----------------- |
| SPI0        | GPIOB.8   | PICO (MOSI)          | SPI1_PICO         |
|             | GPIOB.7   | POCI (MISO)          | SPI1_POCI         |
|             | GPIOB.9   | SCLK                 | SPI1_SCLK         |
|             | GPIOB.13  | CS                   | SPI1_CS1_POCI1    |
| AD7606      | GPIOB.4   | Busy 状态检测        | 中断触发          |
|             | GPIOB.0   | REST (硬件复位)       | 复位控制          |
|             | GPIOB.13  | CS (SPI)             | SPI1_CS1_POCI1    |
| AD9834      | GPIOA.13  | SYNC (频率控制)       | 同步信号          |
|             | GPIOB.13  | RST (复位)           | 硬件复位          |
| OLED        | GPIOB.17  | CS (片选)            | SPI1_CS1_POCI1    |
|             | GPIOA.14  | DC (数据/命令选择)    | GPIO控制          |
|             | GPIOA.13  | RES (复位)           | 硬件复位          |
| UART0       | GPIOA.11  | RX                   | UART0_RX          |
|             | GPIOA.10  | TX                   | UART0_TX          |
| I2C1        | GPIOB.3   | SDA                  | I2C1_SDA          |
|             | GPIOB.2   | SCL                  | I2C1_SCL          |

### 调试接口引脚
| 外设     | 引脚      | 功能                 | LaunchPad 设置              |
| -------- | --------- | -------------------- | --------------------------- |
| DEBUGSS  | PA20      | 调试时钟 (SWCLK)     | J101 15:16 ON 连接 XDS-110  |
|          | PA19      | 调试数据 (SWDIO)     | J101 13:14 ON 连接 XDS-110  |

### 功能说明
1. **SPI0 通信**：
   - 用于 AD7606 和 AD9834 的数据传输
   - 支持 DMA 传输和中断触发
   - 时序控制：CS 信号在传输前拉低，传输完成后释放
   - 数据组合公式：`(rxBuffer[0] << 8) | rxBuffer[1]`

2. **AD7606 模拟采集**：
   - ±10V 量程 ADC 采集
   - 转换公式：`电压值 = (原始值 / 32768.0) * 10.0`
   - 支持 8 通道数据采集
   - 使用浮点数组存储转换后的电压值

3. **AD9834 波形发生器**：
   - 支持正弦波/三角波/方波输出
   - 频率控制字计算公式：`FREQ = (f_out * 2^28) / MCLK`
   - 主要引脚：SYNC(同步信号)、RST(复位)

4. **OLED 显示模块**：
   - 使用 SPI 接口通信
   - 包含片选(CS)、数据/命令选择(DC)、复位(RES)控制引脚

5. **UART0 通信**：
   - 用于调试信息输出
   - 支持标准 printf 输出
   - 自定义格式化输出函数 UART_Printf

### 模块化功能说明
1. **SPI 通信模块**：
   - 管理 AD7606 和 AD9834 的数据传输
   - 支持 DMA 传输和中断处理

2. **AD7606 数据采集模块**：
   - 硬件复位和初始化
   - 原始数据到电压值转换
   - 通道分离和数据存储

3. **AD9834 波形生成模块**：
   - 频率和波形控制
   - 硬件复位序列
   - 频率寄存器配置

4. **OLED 显示模块**：
   - 提供图形显示功能
   - 使用 SPI 接口与控制器通信
   - 包含初始化和显示更新函数

5. **串口调试模块**：
   - 提供 fputc/fputs/puts 标准输出函数
   - 支持 printf 重定向到 UART0

### 设备迁移建议
本项目是为 LP_MSPM0G3507 LaunchPad 中包含的超集设备开发的。请访问 [CCS 用户指南](https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/tools/ccs_ide_guide/doc_guide/doc_guide-srcs/ccs_ide_guide.html#sysconfig-project-migration) 获取迁移到其他 MSPM0 设备的信息。

### 低功耗建议
TI 建议将未使用的引脚功能设置为 GPIO，并配置为输出低电平或带内部上下拉电阻的输入模式。开发者可以通过 SysConfig 轻松配置未使用引脚，选择 **Board → Configure Unused Pins**。

有关使用 MSPM0 LaunchPad 实现低功耗的跳线配置更多信息，请访问 [LP-MSPM0G3507 用户指南](https://www.ti.com/lit/slau873)。

## 示例使用方法
编译、加载并运行示例程序。RGB LED 将以红色与蓝绿色相反的方式交替闪烁。

USER_TEST_PIN GPIO 将模拟 BoosterPack 接头上的 LED1 和 LED3 行为，可用于验证 LED 功能。