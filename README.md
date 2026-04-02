<div id="top"></div>

# CC2530F256 Serial Communication and DHT11 Alarm Demo

Language / 语言: [中文](#zh-cn) | [English](#en-us)

---

<div id="zh-cn"></div>

# 中文

[切换到英文](#en-us)

## 1. 项目概述

本项目基于 TI `CC2530F256`，开发环境为 `IAR Embedded Workbench for 8051`。当前代码已经从早期的按键跑马灯示例演进为一个以传感采集和串口交互为主的监测程序，核心能力如下：

- 使用 `UART0` 与上位机进行串行通讯
- 周期读取 `DHT11` 温湿度数据
- 使用 `ADC` 采集光照强度，并通过软件 `PWM` 调节 `LED2` 亮度
- 当温度或湿度超过阈值时，触发蜂鸣器和 LED 的声光双报警
- 支持通过串口命令动态修改报警阈值

当前主流程由 `main.c`、`uart.c`、`dht11.c`、`adc.c`、`pwm.c`、`led.c`、`buzzer.c` 组成；`key.c` 仍保留在工程中，但目前未被主循环消费，不再作为当前主功能描述。

## 2. 硬件与引脚映射

以下映射依据当前源码中的寄存器配置整理。

| 信号 | CC2530 引脚 | 说明 |
| --- | --- | --- |
| UART0 RX | `P0.2` | `USART0 Alt.1` 接收脚 |
| UART0 TX | `P0.3` | `USART0 Alt.1` 发送脚 |
| DHT11 Data | `P2.0` | 单总线数据脚，空闲高电平 |
| Light Sensor ADC | `P0.7` | `AIN7` 模拟输入 |
| LED1 | `P1.4` | 有源低，用于报警闪烁 |
| LED2 / PWM | `P0.1` | 有源低，按光照值调节亮度 |
| Buzzer | `P1.7` | 高电平使能 |
| SW1 | `P1.5` | 保留在 `key.c` 中，当前主流程未使用 |
| SW2 | `P1.6` | 保留在 `key.c` 中，当前主流程未使用 |

硬件说明：

- `LED2` 和软件 PWM 共用 `P0.1`，当前不再用于跑马灯显示。
- `DHT11` 使用 `P2.0` 普通 GPIO 实现单总线时序，并非硬件外设接口。
- `P2.1`、`P2.2` 和 `RESET` 建议继续保留给调试与下载接口。

## 3. 软件架构与模块说明

### 3.1 主控流程

`main.c` 完成以下工作：

- 切换系统时钟到 `32 MHz XOSC`
- 初始化 `LED`、`Buzzer`、`ADC`、`PWM`、`DHT11`、`UART` 和 `Timer4`
- 启动后通过串口输出启动信息和默认阈值
- 在主循环中处理 DHT11 采样结果和 UART 命令解析

`Timer4` 被配置为 `1 ms` 周期中断，在中断服务程序中执行：

- `Key_Process1ms()`，用于保留的按键扫描逻辑
- `LightCtrl_Process1ms()`，每 `50 ms` 采样一次光照
- `PWM_Process1ms()`，更新 `LED2` 软件 PWM 输出
- `Alarm_Process1ms()`，驱动蜂鸣器与 `LED1` 报警节拍
- 维护 `DHT11` 的 `2000 ms` 周期采样计时

### 3.2 模块职责

- `main.c`
  - 系统初始化、阈值管理、传感器数据主流程、报警状态控制
- `uart.c` / `uart.h`
  - `UART0` 初始化、字符串发送、数字发送、接收环形缓冲区、中断接收
- `dht11.c` / `dht11.h`
  - `P2.0` 单总线初始化、起始信号时序、40 bit 数据读取、校验和判断
- `adc.c` / `adc.h`
  - `P0.7 / AIN7` 模拟输入初始化和 8 bit 光照采样
- `pwm.c` / `pwm.h`
  - 基于 `1 ms` 调用节拍的软件 PWM，控制 `LED2` 亮度
- `led.c` / `led.h`
  - `LED1` 到 `LED4` 的 GPIO 控制接口，其中当前主流程只直接使用 `LED1` 和 `LED2`
- `buzzer.c` / `buzzer.h`
  - 蜂鸣器引脚初始化和开关控制
- `key.c` / `key.h`
  - 仍保留在工程内，具备中断消抖和按键事件输出能力，但当前 `main.c` 未调用 `Key_GetEvent()`

## 4. 功能开发

### 4.1 开发串行通讯

当前串口功能基于 `UART0` 实现，关键行为如下：

- 管脚映射：
  - `P0.2 = RX0`
  - `P0.3 = TX0`
- 波特率配置：`115200`
- 帧格式：`8-N-1`
- 接收方式：`URX0` 中断接收 + 环形缓冲区缓存
- 主循环通过非阻塞方式读取接收缓存并解析命令

启动后会先输出以下信息：

```text
CC2530 Monitor Ready
TEMP_TH=40 HUMI_TH=60
```

当 DHT11 读取成功时，程序会向上位机周期上报一帧数据，格式如下：

```text
T:<temp> H:<humi> L:<adc>
```

示例：

```text
T:25 H:60 L:180
```

其中：

- `T` 表示当前温度
- `H` 表示当前湿度
- `L` 表示当前光照 ADC 采样值

支持的串口下发命令如下：

```text
TH:<value>
HH:<value>
```

命令说明：

- `TH:<value>`：设置温度报警阈值
- `HH:<value>`：设置湿度报警阈值
- 命令以回车、换行或回车换行为结束
- 命令缓冲区长度为 `16` 字节，超出后当前命令会被丢弃并重新开始
- 接收环形缓冲区长度为 `32` 字节
- 数值按十进制解析，超过 `255` 时会被钳位到 `255`

命令处理成功后，会立即返回确认信息，例如：

```text
SET TEMP_TH=30
SET HUMI_TH=70
```

### 4.2 开发 DHT11 测量温湿度并进行声光双报警

当前 DHT11 与报警逻辑已经在主程序中联通，工作方式如下：

- DHT11 数据脚接在 `P2.0`
- `Timer4` 每 `2000 ms` 置位一次采样标志
- 主循环检测到采样标志后，调用 `DHT11_Read()` 读取温湿度
- 为避免微秒级时序被中断打断，读取 DHT11 期间会暂时关闭总中断

`DHT11_Read()` 的返回值定义为：

- `0`：读取成功
- `1`：无响应或握手失败
- `2`：校验和错误

默认报警阈值如下：

- 温度阈值：`40 C`
- 湿度阈值：`60 %`

报警触发规则：

- 只要 `温度 >= 温度阈值` 或 `湿度 >= 湿度阈值`，就进入报警状态
- 当温度和湿度都恢复到阈值以下时，自动退出报警状态

报警表现为声光双通道同时动作：

- 蜂鸣器 `P1.7` 每 `200 ms` 翻转一次，实现间歇鸣叫
- `LED1 (P1.4)` 每 `100 ms` 翻转一次，实现快速闪烁
- 报警解除时，蜂鸣器关闭，`LED1` 熄灭

当 DHT11 读取失败时，串口会输出错误码，格式如下：

```text
DHT11 ERR:<code>
```

为便于观察环境变化，当前程序还同时实现了光照联动：

- `P0.7` 通过 `ADC` 每 `50 ms` 采样一次环境光强
- 采样值按阈值区间映射到 `0~255` 亮度级别
- 软件 PWM 将该亮度映射到 `LED2 (P0.1)`，用于直观显示光照变化
- 该光照值也会随串口帧中的 `L:<adc>` 一起上报

## 5. 构建、下载与验证依据

### 5.1 构建环境

- IDE：`IAR Embedded Workbench for 8051`
- 目标器件：`CC2530F256`
- 设备头文件：`ioCC2530.h`
- 工程文件：`led.ewp`
- 常见构建配置：`Debug`、`Release`

### 5.2 下载说明

1. 使用 IAR 打开 `led.ewp`
2. 选择 `Debug` 或 `Release`
3. 编译工程
4. 通过 CC2530 调试下载接口烧录到目标板

### 5.3 当前可追溯依据

- 工程文件 `led.ewp` 已包含 `main.c`、`uart.c`、`dht11.c`、`adc.c`、`pwm.c`、`led.c`、`buzzer.c`、`key.c`
- 仓库中存在 `Debug/Exe/led.hex` 与 `Debug/Exe/led.d51`，可作为历史构建产物依据
- 本 README 内容已按当前源码实现整理，不再沿用旧版跑马灯说明

### 5.4 未验证项

以下内容在本次 README 重写过程中未重新执行，需在目标板上补充验证：

- 未重新执行 IAR 编译
- 未进行本轮真机烧录
- 未进行串口联调和上位机收发实测
- 未进行 DHT11 实测温湿度验证
- 未进行蜂鸣器与 LED 报警现象确认

## 6. 当前限制与说明

- `DHT11` 读取依赖 `32 MHz` 时钟假设和软件延时，时序精度尚未在当前硬件上重新标定，结论为未验证。
- 读取 DHT11 时会关闭总中断，可能导致该时段内的 `UART` 接收或 `Timer4` 节拍出现短暂延迟。
- 报警阈值仅保存在 RAM 中，掉电后会恢复默认值 `40 C / 60 %`。
- 阈值命令只做了基本十进制解析，没有限制到 DHT11 的典型量程范围。
- `key.c` 模块仍保留在工程中，但当前主流程未使用，不应再将其视为当前主功能的一部分。

[返回顶部](#top)

---

<div id="en-us"></div>

# English

[Switch to Chinese](#zh-cn)

## 1. Project Overview

This project targets the TI `CC2530F256` and is developed with `IAR Embedded Workbench for 8051`. The current codebase has evolved from an early running-light example into a sensor-monitoring application centered on serial communication and alarm handling. The main capabilities are:

- `UART0` serial communication with a host PC
- Periodic temperature and humidity acquisition from `DHT11`
- Light sampling through `ADC`, with software `PWM` brightness control on `LED2`
- Audible and visual alarm when temperature or humidity exceeds the configured threshold
- Runtime alarm-threshold update through UART commands

The active application flow is implemented by `main.c`, `uart.c`, `dht11.c`, `adc.c`, `pwm.c`, `led.c`, and `buzzer.c`. The `key.c` module is still present in the project, but it is not consumed by the current main loop and is therefore no longer described as part of the main feature set.

## 2. Hardware and Pin Mapping

The following mapping is derived from the current register configuration in the source code.

| Signal | CC2530 Pin | Description |
| --- | --- | --- |
| UART0 RX | `P0.2` | `USART0 Alt.1` receive pin |
| UART0 TX | `P0.3` | `USART0 Alt.1` transmit pin |
| DHT11 Data | `P2.0` | Single-wire data pin, idle high |
| Light Sensor ADC | `P0.7` | `AIN7` analog input |
| LED1 | `P1.4` | Active-low, used for alarm flashing |
| LED2 / PWM | `P0.1` | Active-low, brightness follows light level |
| Buzzer | `P1.7` | Enabled by high level |
| SW1 | `P1.5` | Retained in `key.c`, not used by current main flow |
| SW2 | `P1.6` | Retained in `key.c`, not used by current main flow |

Hardware notes:

- `LED2` shares `P0.1` with the software PWM output and is no longer used as a running-light indicator.
- `DHT11` is implemented on `P2.0` using GPIO bit timing, not through a dedicated hardware peripheral.
- `P2.1`, `P2.2`, and `RESET` should remain reserved for debug and download use.

## 3. Software Architecture and Modules

### 3.1 Main Control Flow

`main.c` is responsible for:

- switching the system clock to `32 MHz XOSC`
- initializing `LED`, `Buzzer`, `ADC`, `PWM`, `DHT11`, `UART`, and `Timer4`
- sending startup information and default thresholds over UART
- processing DHT11 samples and UART commands in the main loop

`Timer4` is configured as a `1 ms` periodic interrupt source. The ISR performs:

- `Key_Process1ms()` for the retained key-scan logic
- `LightCtrl_Process1ms()` to sample light every `50 ms`
- `PWM_Process1ms()` to update the software PWM on `LED2`
- `Alarm_Process1ms()` to drive the buzzer and `LED1` alarm pattern
- the `2000 ms` timing base for DHT11 sampling

### 3.2 Module Responsibilities

- `main.c`
  - system initialization, threshold management, sensor-processing flow, alarm-state control
- `uart.c` / `uart.h`
  - `UART0` initialization, string transmit, number transmit, RX ring buffer, interrupt-based receive
- `dht11.c` / `dht11.h`
  - `P2.0` single-wire initialization, start-signal timing, 40-bit frame read, checksum validation
- `adc.c` / `adc.h`
  - `P0.7 / AIN7` analog-input initialization and 8-bit light sampling
- `pwm.c` / `pwm.h`
  - software PWM driven by the `1 ms` task cadence to control `LED2` brightness
- `led.c` / `led.h`
  - GPIO helpers for `LED1` to `LED4`; the current main flow directly uses only `LED1` and `LED2`
- `buzzer.c` / `buzzer.h`
  - buzzer pin initialization and on/off control
- `key.c` / `key.h`
  - still present in the project and capable of interrupt debounce plus key-event output, but `main.c` does not currently call `Key_GetEvent()`

## 4. Feature Development

### 4.1 Serial Communication Development

The current serial communication feature is implemented with `UART0`. Its behavior is:

- Pin mapping:
  - `P0.2 = RX0`
  - `P0.3 = TX0`
- Baud rate: `115200`
- Frame format: `8-N-1`
- Receive method: `URX0` interrupt reception with an RX ring buffer
- The main loop parses commands in a non-blocking manner from the buffered bytes

At startup, the firmware sends:

```text
CC2530 Monitor Ready
TEMP_TH=40 HUMI_TH=60
```

Whenever a DHT11 read succeeds, the firmware reports one data frame to the host:

```text
T:<temp> H:<humi> L:<adc>
```

Example:

```text
T:25 H:60 L:180
```

Where:

- `T` is the current temperature
- `H` is the current humidity
- `L` is the current light ADC sample

The supported UART commands are:

```text
TH:<value>
HH:<value>
```

Command behavior:

- `TH:<value>` sets the temperature alarm threshold
- `HH:<value>` sets the humidity alarm threshold
- commands are terminated by carriage return, line feed, or CRLF
- the command buffer length is `16` bytes; overflow discards the current command
- the RX ring buffer length is `32` bytes
- values are parsed as decimal and saturated to `255` if they overflow

After a valid command, the firmware returns an acknowledgement such as:

```text
SET TEMP_TH=30
SET HUMI_TH=70
```

### 4.2 DHT11 Temperature/Humidity Measurement with Audible and Visual Alarm

The DHT11 acquisition and alarm logic are already integrated into the application:

- the DHT11 data line is connected to `P2.0`
- `Timer4` raises a sampling flag every `2000 ms`
- the main loop reacts to that flag and calls `DHT11_Read()`
- global interrupts are temporarily disabled during the read so that microsecond timing is not corrupted

`DHT11_Read()` returns:

- `0`: success
- `1`: no response or handshake failure
- `2`: checksum error

The default alarm thresholds are:

- temperature threshold: `40 C`
- humidity threshold: `60 %`

Alarm trigger rule:

- alarm becomes active if `temperature >= threshold` or `humidity >= threshold`
- alarm clears automatically when both values return below their thresholds

The audible and visual alarm actions run together:

- the buzzer on `P1.7` toggles every `200 ms`, creating an intermittent beep
- `LED1 (P1.4)` toggles every `100 ms`, creating a fast flash
- when the alarm clears, the buzzer is turned off and `LED1` is forced off

If a DHT11 read fails, the UART reports:

```text
DHT11 ERR:<code>
```

To make environmental change visible, the current firmware also includes light-linked behavior:

- `P0.7` is sampled by the ADC every `50 ms`
- the ADC value is mapped into a `0~255` brightness level
- software PWM drives `LED2 (P0.1)` according to that brightness value
- the same light value is also reported in the UART frame as `L:<adc>`

## 5. Build, Download, and Verification Basis

### 5.1 Build Environment

- IDE: `IAR Embedded Workbench for 8051`
- target device: `CC2530F256`
- device header: `ioCC2530.h`
- project file: `led.ewp`
- common build configurations: `Debug`, `Release`

### 5.2 Download Procedure

1. Open `led.ewp` in IAR
2. Select `Debug` or `Release`
3. Build the project
4. Download the image through the CC2530 debug interface

### 5.3 Current Traceable Evidence

- the project file `led.ewp` already includes `main.c`, `uart.c`, `dht11.c`, `adc.c`, `pwm.c`, `led.c`, `buzzer.c`, and `key.c`
- the repository contains `Debug/Exe/led.hex` and `Debug/Exe/led.d51` as existing build artifacts
- this README has been rewritten to match the current source code rather than the old running-light description

### 5.4 Items Not Re-verified

The following actions were not re-executed during this README rewrite and still require target-board verification:

- no fresh IAR build was run
- no new on-board programming was performed
- no serial communication test with a PC was executed
- no physical DHT11 measurement was re-verified
- no buzzer or LED alarm behavior was physically confirmed

## 6. Current Limitations and Notes

- `DHT11` timing depends on the `32 MHz` clock assumption and software delay loops; timing accuracy has not been recalibrated on the current hardware in this task.
- Global interrupts are disabled during DHT11 reads, which may briefly delay `UART` reception or `Timer4` scheduling.
- Alarm thresholds are stored only in RAM and revert to the default `40 C / 60 %` after reset.
- The UART threshold commands perform only basic decimal parsing and do not clamp values to the typical DHT11 operating range.
- The `key.c` module remains in the project, but it is not part of the current main application flow and should not be described as an active feature.

[Back to Top](#top)
