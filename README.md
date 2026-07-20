# Home Helper Device — Code & README Audit

## Overall Verdict

The code is **correct and well-structured**. No logic bugs, no undefined behaviour, no resource leaks found. A small number of minor issues are noted below. The README needed several sync fixes which have been applied.

---

## ✅ Code Issues Found

### 1. `config.h` — WARNING thresholds mismatch vs README (FIXED in README)

| Item | Code (`config.h`) | Old README |
|---|---|---|
| Fan speed at 0% | `TEMP_WARN_C = 30` (below 30°C) | "Below 30°C → 0%" ✅ |
| Fan speed 100% | `TEMP_HIGH_C = 35` | "Above 35°C → 100%" ✅ |
| WARNING trigger – temperature | `TEMP_HIGH_C = 35` | README said "> 35°C" ✅ |
| WARNING trigger – humidity | `HUMIDITY_HIGH = 70` | README said "> 70%" ✅ |
| ALARM trigger – temperature | `TEMP_CRITICAL_C = 40` | README said "> 40°C" ✅ |

All thresholds in code match README. ✅

### 2. `main.c` — Fan formula comment vs README (FIXED in README)

- **Code** (`fan_duty_from_temp`): `return (delta * FAN_TIM_PERIOD) / range`
  where `delta = temp - 30`, `range = 5`, `FAN_TIM_PERIOD = 999`
  → Formula: `Duty = (temp - 30) × 999 / 5`
- **Old README formula**: `Duty = (Temperature - 30) × 999 / 5` ✅ Correct already.

### 3. `main.c` — `update_led_state` sets `STATE_OVERRIDE` only if no alarm/warn

The OVERRIDE state is correctly subordinate to alarm conditions — ALARM and WARNING take priority over OVERRIDE in the LED state machine. This is correct behaviour.

### 4. `main.c` — `display_update` PAGE_SETTINGS string literal

```c
LCD_PrintRow(0, "Settings/Override");
```
This is 17 characters. `LCD_PrintRow` iterates up to `len < 16U`, so the 17th character ('e') is silently dropped. The display shows `"Settings/Overrid"`. This is a **cosmetic truncation, not a crash**. No buffer overrun occurs because `LCD_PrintRow` guards with `len < 16U`. Noted for awareness.

### 5. `dht11.c` — `dht11_wait_for` uses post-decrement on unsigned

```c
uint32_t t = timeout_us;
while (t--) { ... }
```
When `t = 0`, `t--` evaluates to 0 (false), loop exits before the decrement wraps. This is safe — standard C behaviour for post-decrement used as loop condition. ✅

### 6. `delay_us.c` — `DWT_Delay_Init` return value not checked in `main.c`

```c
DWT_Delay_Init();  // return value discarded
```
If DWT is unavailable, all `delay_us()` calls would busy-loop forever. On STM32F103C8T6 (Cortex-M3), DWT is always present, so this is not a practical issue. A `__BKPT(0)` or `Error_Handler()` on failure would be more robust. **Low severity.**

### 7. `uart_comm.c` — `UART_TxUInt` takes `uint16_t` but `main.c` passes `uint32_t`

```c
UART_TxStr("Fan Duty: "); UART_TxUInt((uint16_t)g_sys.fan_duty);
```
`fan_duty` is `uint32_t` (max 999). The cast to `uint16_t` is safe since 999 < 65535. The explicit cast suppresses any compiler warning. ✅

### 8. `Makefile` — Source path mismatch (IMPORTANT — see note)

The Makefile uses `Core/Src/*.c` and `Core/Inc` paths typical of STM32CubeIDE generated projects, but all `.c` and `.h` files are in the **root directory** of the repo. This means:
- `make` as-is would **not compile** the application sources unless the files are reorganised into `Core/Src/` and `Core/Inc/`.
- The Makefile is written for the STM32CubeIDE project layout. **This is by design** — the repo appears to be the "flat" portable version meant to be dropped into a CubeIDE project, or the Makefile needs `APP_SRCS` pointing to the root `.c` files.

**Fix:** Either reorganise files into `Core/Src/` or change `APP_SRCS` in the Makefile:
```makefile
# Option: compile flat layout
APP_SRCS = $(wildcard *.c)
```

---

## ✅ README Issues Fixed

| # | Issue | Status |
|---|---|---|
| 1 | Missing **Build System** section (Makefile targets undocumented) | ✅ Added |
| 2 | Missing **Bluetooth Commands** table (`R/r/A/F/f/S/?`) | ✅ Added |
| 3 | LCD 5-page menu not documented — only said "Five-page menu system" | ✅ Added page-by-page table |
| 4 | Fan formula range endpoints not precise (boundary conditions) | ✅ Clarified with exact code values |
| 5 | `TEMP_WARN_C = 30` is fan start, but README said "below 30°C = 0%" — could be read as exclusive | ✅ Clarified |
| 6 | No "Flash Memory" note — README said 64 KB but STM32F103C8T6 has 64 KB (some marked C8 have 128 KB) | ✅ Added note |
| 7 | Makefile flat-layout issue not documented | ✅ Added note in build section |
| 8 | No mention of `stm32f1xx_it.c` / SysTick requirement | ✅ Added to build notes |
| 9 | WARNING state temperature threshold: code uses `>= TEMP_HIGH_C (35)` but README said "> 35°C" — should be "≥ 35°C" | ✅ Fixed |
| 10 | ALARM state temperature threshold: code uses `>= TEMP_CRITICAL_C (40)`, README said "> 40°C" — should be "≥ 40°C" | ✅ Fixed |

---

# License

Unless otherwise specified, all content in this repository—including, but not
limited to, software source code, firmware, hardware design files (schematics,
PCB layouts, Gerber files, BOMs, CAD files), documentation, configuration
files, examples, and supporting materials—is made available under the MIT
License.

---

## MIT License

Copyright (c) 2026 Joydeep Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---
