# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.
See .claude/skills/cc2530-firmware for additional specialized workflows.

## Project Overview

CC2530F256 bare-metal firmware project (8051 core) implementing an interrupt-driven running-light demo with key debounce. Built with **IAR Embedded Workbench for 8051**. Target board is the E18-MS1 development board.

## Build

- **IDE**: IAR EW8051 (no command-line build path configured)
- **Project file**: `led.ewp` — open in IAR, select Debug or Release, build, then download via CC2530 debug interface
- **Device header**: `ioCC2530.h` (provided by IAR toolkit, not in repo)
- **No Makefile or CLI build** — all compilation is done through the IAR GUI

## Architecture

All code is register-level C with no OS or HAL library. Two interrupt sources drive the system:

1. **Port 1 ISR** (`key.c`, vector `0x7B`) — detects SW1/SW2 press edges, disables further key interrupts, starts the debounce state machine.
2. **Timer4 ISR** (`main.c`, vector `0x63`) — fires every 1 ms; ticks the key debounce/release-confirm FSM (`Key_Process1ms`) and the running-light step counter.

Main loop in `main.c` polls `Key_GetEvent()` and drives the running-light state machine (200 ms step, LED1→LED2→LED3→LED4 cycle).

### Module responsibilities

| Module | Role |
|---|---|
| `main.c` | Clock init (RC→32 MHz XOSC), Timer4 setup, running-light FSM, Timer4 ISR |
| `led.c/h` | GPIO init & control for LED1–LED4 (active-low) |
| `key.c/h` | Port 1 ISR, 4-state debounce machine (IDLE→PRESS→RELEASE→REL_CHECK, 20 ms), event output |
| `buzzer.c/h` | Buzzer GPIO init & on/off (P1.7, active-high, default off) |

## Hardware Pin Map

| Signal | Pin | Active | Notes |
|---|---|---|---|
| LED1 | P1.4 | Low | |
| LED2 | P0.1 | Low | Shared with external header |
| LED3 | P1.0 | Low | |
| LED4 | P1.1 | Low | |
| SW1 | P1.5 | Low | Shared with ESP reset line |
| SW2 | P1.6 | Low | |
| Buzzer | P1.7 | High | Kept off by default |

## Coding Conventions

- All GPIO and peripheral configuration done via direct CC2530 register writes (`P1DIR`, `P1SEL`, `CLKCONCMD`, `T4CTL`, etc.)
- ISR handlers use `#pragma vector = <vector_addr>` followed by `__interrupt void HandlerName(void)`
- LED IDs are 1-based (LED1=1 through LED4=4) in the public API
- Key debounce timing and running-light step interval are hardcoded constants, dependent on the 32 MHz XOSC and Timer4 1 ms tick
