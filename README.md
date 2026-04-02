<div id="top"></div>

# CC2530F256 Running Light Demo

Language / 语言: [中文](#zh-cn) | [English](#en-us)

---

<div id="zh-cn"></div>

# 中文

[切换到英文](#en-us)

## 项目概述
- 4.2使用DHT11测量温湿度并进行声光双报警
- TH:(设温度阈值)
- HH:(设湿度阈值)
本项目基于 TI `CC2530F256`，开发环境为 IAR EW8051。当前实现的是一个基于中断的跑马灯示例，包含以下功能：

- 系统时钟切换到 `32 MHz XOSC`
- `SW1` 和 `SW2` 使用中断方式处理
- 按键软件消抖
- 按键必须释放后才执行对应功能
- 按下并释放 `SW1` 后启动 `LED1` 到 `LED4` 跑马灯
- 按下并释放 `SW2` 后熄灭全部 LED
- 上电初始化时将蜂鸣器保持关闭

## 当前行为

上电后：

- MCU 从默认 RC 时钟切换到 `32 MHz XOSC`
- `LED1` 到 `LED4` 初始为熄灭状态
- 蜂鸣器引脚完成初始化并保持关闭
- 为 `SW1` 和 `SW2` 使能 `Port 1` 中断
- `Timer4` 产生 `1 ms` 周期中断

按键行为：

- 按下并释放 `SW1` 后启动跑马灯
- 按下并释放 `SW2` 后停止跑马灯并熄灭全部 LED
- 只有在所有按键释放后才确认按键事件
- 消抖时间为 `20 ms`
- 如果两个按键同时按下，则忽略本次事件

跑马灯行为：

- 任意时刻仅有一个 LED 点亮
- 顺序为 `LED1 -> LED2 -> LED3 -> LED4 -> LED1`
- 切换间隔为 `200 ms`

## 硬件映射

以下引脚说明基于当前代码和仓库中使用的开发板参考资料。

| 信号 | CC2530 引脚 | 说明 |
| --- | --- | --- |
| LED1 | `P1.4` | 低电平点亮 |
| LED2 | `P0.1` | 低电平点亮 |
| LED3 | `P1.0` | 低电平点亮 |
| LED4 | `P1.1` | 低电平点亮 |
| SW1 | `P1.5` | 低电平有效，与 ESP 复位共用 |
| SW2 | `P1.6` | 低电平有效 |
| Buzzer | `P1.7` | 高电平使能，软件默认关闭 |

硬件注意事项：

- `SW1` 与板上的 ESP 复位线共用 `P1.5`
- `LED2` 与外部排针共用 `P0.1`
- `P2.1`、`P2.2` 和 `RESET` 保留给调试与下载接口

## 软件结构

- `main.c`
  - 时钟初始化
  - `Timer4` 初始化
  - 跑马灯应用状态机
  - `Timer4` 中断服务函数
- `led.c` / `led.h`
  - LED GPIO 初始化
  - 低电平点亮 LED 的控制接口
- `key.c` / `key.h`
  - `Port 1` 按键中断入口
  - 消抖与释放确认状态机
  - 按键事件输出接口
- `buzzer.c` / `buzzer.h`
  - 蜂鸣器 GPIO 初始化
  - 蜂鸣器开关接口
- `led.ewp`
  - IAR EW8051 工程文件，目标器件为 `CC2530F256`

## 中断设计

项目使用两个中断源：

1. `Port 1` 中断
   - 检测 `SW1` 或 `SW2` 的按下边沿
   - 暂时关闭后续按键中断
   - 启动按键扫描状态机
2. `Timer4` 的 `1 ms` 周期中断
   - 执行按键消抖和释放确认
   - 生成稳定的按键事件
   - 更新跑马灯节拍

该设计保持了按键响应的中断触发方式，同时避免在不稳定的按键边沿上直接执行业务动作。

## 构建环境

- IDE：`IAR Embedded Workbench for 8051`
- 目标器件：`CC2530F256`
- 设备头文件：`ioCC2530.h`
- 工程文件：`led.ewp`
- 构建配置：`Debug`、`Release`

## 编译与下载

1. 使用 IAR EW8051 打开 `led.ewp`
2. 选择 `Debug` 或 `Release`
3. 编译工程
4. 通过 CC2530 调试接口下载程序

当前工作环境中没有可用的 IAR 命令行构建工具路径，因此这里没有完成实际构建验证。

## 对外接口

### LED

- `void LED_Init(void);`
- `void LED_On(unsigned char led_id);`
- `void LED_Off(unsigned char led_id);`
- `void LED_AllOff(void);`
- `void LED_ShowSingle(unsigned char led_id);`

### 按键

- `void Key_Init(void);`
- `void Key_Process1ms(void);`
- `unsigned char Key_GetEvent(void);`

按键事件值：

- `KEY_EVENT_NONE`
- `KEY_EVENT_SW1`
- `KEY_EVENT_SW2`

### 蜂鸣器

- `void Buzzer_Init(void);`
- `void Buzzer_On(void);`
- `void Buzzer_Off(void);`

## 当前限制

- 当前版本未实现显示器输出
- 工程直接基于 CC2530 寄存器编写，并绑定当前开发板引脚映射
- 按键时序依赖 `32 MHz` 时钟配置和当前 `Timer4` 设置

[返回顶部](#top)

---

<div id="en-us"></div>

# English

[Switch to Chinese](#zh-cn)

## Overview

This project targets the TI `CC2530F256` and is built with IAR EW8051. It currently implements an interrupt-driven running-light demo with the following behavior:

- system clock switched to `32 MHz XOSC`
- `SW1` and `SW2` handled by interrupts
- software key debounce
- action executed only after key release
- pressing and releasing `SW1` starts the `LED1` to `LED4` running light
- pressing and releasing `SW2` turns off all LEDs
- buzzer forced to the off state during initialization

## Current Behavior

After power-on:

- the MCU switches from the default RC clock to `32 MHz XOSC`
- `LED1` to `LED4` are off
- the buzzer GPIO is initialized and kept off
- `Port 1` interrupt is enabled for `SW1` and `SW2`
- `Timer4` generates a `1 ms` periodic interrupt

Key behavior:

- pressing and releasing `SW1` starts the running light
- pressing and releasing `SW2` stops the running light and turns off all LEDs
- key events are accepted only after all keys are released
- debounce time is `20 ms`
- if both keys are pressed at the same time, the event is ignored

Running-light behavior:

- only one LED is on at a time
- sequence is `LED1 -> LED2 -> LED3 -> LED4 -> LED1`
- step interval is `200 ms`

## Hardware Mapping

The following mapping is based on the current code and the board reference used in this repository.

| Signal | CC2530 Pin | Notes |
| --- | --- | --- |
| LED1 | `P1.4` | Active-low |
| LED2 | `P0.1` | Active-low |
| LED3 | `P1.0` | Active-low |
| LED4 | `P1.1` | Active-low |
| SW1 | `P1.5` | Active-low, shared with ESP reset |
| SW2 | `P1.6` | Active-low |
| Buzzer | `P1.7` | Enabled by high level, kept off by software |

Hardware notes:

- `SW1` shares `P1.5` with the ESP reset line on this board
- `LED2` shares `P0.1` with an external header pin
- `P2.1`, `P2.2`, and `RESET` are reserved for debug/download

## Software Structure

- `main.c`
  - clock initialization
  - `Timer4` initialization
  - application state machine for the running light
  - `Timer4` interrupt service routine
- `led.c` / `led.h`
  - LED GPIO initialization
  - active-low LED control helpers
- `key.c` / `key.h`
  - `Port 1` key interrupt entry
  - debounce and release-confirm state machine
  - key event output interface
- `buzzer.c` / `buzzer.h`
  - buzzer GPIO initialization
  - buzzer on/off helpers
- `led.ewp`
  - IAR EW8051 project file for `CC2530F256`

## Interrupt Design

The project uses two interrupt sources:

1. `Port 1` interrupt
   - detects a press edge on `SW1` or `SW2`
   - temporarily disables further key interrupts
   - starts the key scan state machine
2. `Timer4` interrupt at `1 ms`
   - runs key debounce and release confirmation
   - produces stable key events
   - updates the running-light timing

This design keeps key handling interrupt-driven while avoiding direct action on an unstable key edge.

## Build Environment

- IDE: `IAR Embedded Workbench for 8051`
- target device: `CC2530F256`
- device header: `ioCC2530.h`
- project file: `led.ewp`
- build configurations: `Debug`, `Release`

## Build and Download

1. Open `led.ewp` in IAR EW8051
2. Select `Debug` or `Release`
3. Build the project
4. Download through the CC2530 debug interface

This workspace does not provide a verified IAR command-line build tool path, so no build verification was executed here.

## Public Interfaces

### LED

- `void LED_Init(void);`
- `void LED_On(unsigned char led_id);`
- `void LED_Off(unsigned char led_id);`
- `void LED_AllOff(void);`
- `void LED_ShowSingle(unsigned char led_id);`

### Key

- `void Key_Init(void);`
- `void Key_Process1ms(void);`
- `unsigned char Key_GetEvent(void);`

Key event values:

- `KEY_EVENT_NONE`
- `KEY_EVENT_SW1`
- `KEY_EVENT_SW2`

### Buzzer

- `void Buzzer_Init(void);`
- `void Buzzer_On(void);`
- `void Buzzer_Off(void);`

## Limitations

- no display output is implemented in the current codebase
- the project is written directly against CC2530 registers and tied to the current board pin mapping
- key timing depends on the `32 MHz` clock configuration and the current `Timer4` setup

[Back to Top](#top)
