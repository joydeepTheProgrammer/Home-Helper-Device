/**
 * @file    main.c
 * @brief   Home Helper Device — STM32F103C8T6 Main Application
 *
 * ═══════════════════════════════════════════════════════════════════
 *  CLOCK CONFIGURATION:
 *   HSE = 8 MHz → PLL ×9 → SYSCLK = 72 MHz
 *   AHB = 72 MHz  |  APB1 = 36 MHz  |  APB2 = 72 MHz
 *   SysTick → HAL_GetTick() → 1 ms resolution
 *
 *  PERIPHERAL INIT SUMMARY:
 *   MX_GPIO_Init()       — All GPIO ports
 *   MX_USART1_UART_Init()— 115200 baud, 8N1
 *   MX_ADC1_Init()       — PA0(ch0), PA1(ch1), software trigger
 *   MX_TIM3_Init()       — PWM CH1 on PA6, 1 kHz, 0–100% duty
 *   DWT_Delay_Init()     — µs delay via cycle counter
 *
 *  FEATURES:
 *   1. Temperature & Humidity (DHT11) → fan PWM auto-control
 *   2. Gas / Smoke Detection (MQ-2)   → alarm + relay off
 *   3. Ambient Light (LDR)            → auto light via relay
 *   4. Motion Detection (PIR)         → occupancy logic
 *   5. 16×2 LCD 5-page menu           → UP/DOWN/SELECT buttons
 *   6. Bluetooth HC-05 via USART1     → remote control
 * ═══════════════════════════════════════════════════════════════════
 */

#include "main.h"
#include "config.h"
#include "delay_us.h"
#include "dht11.h"
#include "lcd.h"
#include "uart_comm.h"
#include "adc_sensor.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ═══════════════════════════════════════════════════════════════════
 *  HAL PERIPHERAL HANDLES (extern declared in config.h)
 * ═══════════════════════════════════════════════════════════════════ */
UART_HandleTypeDef huart1;
TIM_HandleTypeDef  htim3;
ADC_HandleTypeDef  hadc1;

/* ═══════════════════════════════════════════════════════════════════
 *  SYSTEM DATA
 * ═══════════════════════════════════════════════════════════════════ */
typedef struct {
    uint8_t  temperature;
    uint8_t  humidity;
    uint16_t gas_adc;          /* 0–4095 (12-bit) */
    uint16_t ldr_adc;          /* 0–4095 (12-bit) */
    bool     motion_detected;
    bool     dht_valid;

    uint32_t fan_duty;         /* 0–FAN_TIM_PERIOD (999) */
    bool     relay_on;
    bool     buzzer_on;
    bool     relay_override;

    SystemState_t  state;
    DisplayPage_t  display_page;
    uint32_t       uptime_s;
} HomeHelper_t;

static HomeHelper_t g_sys;

/* ═══════════════════════════════════════════════════════════════════
 *  PRIVATE PROTOTYPES
 * ═══════════════════════════════════════════════════════════════════ */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);

static void     fan_set_duty(uint32_t duty);
static uint32_t fan_duty_from_temp(uint8_t temp);
static void     control_fan(void);
static void     control_relay(void);
static void     control_buzzer(void);
static void     update_led_state(void);
static void     display_update(void);
static void     handle_buttons(void);
static void     handle_bluetooth(void);
static void     uart_status_dump(void);
static bool     timer_elapsed(uint32_t *last_ms, uint32_t interval_ms);

/* ═══════════════════════════════════════════════════════════════════
 *  NON-BLOCKING TIMER CHECK
 * ═══════════════════════════════════════════════════════════════════ */
static bool timer_elapsed(uint32_t *last_ms, uint32_t interval_ms) {
    uint32_t now = HAL_GetTick();
    if ((now - *last_ms) >= interval_ms) {
        *last_ms = now;
        return true;
    }
    return false;
}

/* ═══════════════════════════════════════════════════════════════════
 *  FAN PWM CONTROL
 * ═══════════════════════════════════════════════════════════════════ */
static void fan_set_duty(uint32_t duty) {
    if (duty > FAN_TIM_PERIOD) duty = FAN_TIM_PERIOD;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
}

/**
 * @brief  Linear map: temperature → TIM3 compare value (0–999).
 *         30°C = 0%, 35°C = 100%, gas alarm = 100%
 */
static uint32_t fan_duty_from_temp(uint8_t temp) {
    if (temp < TEMP_WARN_C)  return 0U;
    if (temp >= TEMP_HIGH_C) return FAN_TIM_PERIOD;

    /* Linear interpolation in uint32 arithmetic to avoid overflow */
    uint32_t range = (uint32_t)(TEMP_HIGH_C - TEMP_WARN_C);
    uint32_t delta = (uint32_t)(temp - TEMP_WARN_C);
    return (delta * FAN_TIM_PERIOD) / range;
}

static void control_fan(void) {
    if (g_sys.gas_adc >= GAS_ALARM_ADC) {
        g_sys.fan_duty = FAN_TIM_PERIOD;   /* Max ventilation on gas */
    } else if (g_sys.dht_valid) {
        g_sys.fan_duty = fan_duty_from_temp(g_sys.temperature);
    } else {
        g_sys.fan_duty = 0U;
    }
    fan_set_duty(g_sys.fan_duty);
}

/* ═══════════════════════════════════════════════════════════════════
 *  RELAY CONTROL
 * ═══════════════════════════════════════════════════════════════════ */
static void control_relay(void) {
    /* Manual override: skip auto logic */
    if (g_sys.relay_override) return;

    /* Safety: gas alarm → relay OFF unconditionally */
    if (g_sys.gas_adc >= GAS_ALARM_ADC) {
        g_sys.relay_on = false;
        RELAY_OFF();
        return;
    }

    /* Safety: critical temperature → protect appliance */
    if (g_sys.dht_valid && (g_sys.temperature >= TEMP_CRITICAL_C)) {
        g_sys.relay_on = false;
        RELAY_OFF();
        return;
    }

    /* Automation: Light ON when dark AND motion present */
    bool is_dark    = (g_sys.ldr_adc < LDR_DARK_THRESHOLD);
    bool has_motion = g_sys.motion_detected;

    if (is_dark && has_motion) {
        g_sys.relay_on = true;
        RELAY_ON();
    } else if (!is_dark) {
        /* Daytime → light off */
        g_sys.relay_on = false;
        RELAY_OFF();
    }
    /* Dark + no motion → maintain current relay state */
}

/* ═══════════════════════════════════════════════════════════════════
 *  BUZZER CONTROL
 * ═══════════════════════════════════════════════════════════════════ */
static void control_buzzer(void) {
    if (g_sys.gas_adc >= GAS_ALARM_ADC) {
        /* Continuous alarm */
        g_sys.buzzer_on = true;
        BUZZER_ON();
    } else if ((g_sys.gas_adc >= GAS_WARN_ADC) ||
               (g_sys.dht_valid && (g_sys.temperature >= TEMP_CRITICAL_C))) {
        /* Intermittent beep: toggle on each 500 ms poll */
        g_sys.buzzer_on = !g_sys.buzzer_on;
        if (g_sys.buzzer_on) BUZZER_ON();
        else                  BUZZER_OFF();
    } else {
        g_sys.buzzer_on = false;
        BUZZER_OFF();
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  LED STATUS UPDATE
 * ═══════════════════════════════════════════════════════════════════ */
static void update_led_state(void) {
    if ((g_sys.gas_adc >= GAS_ALARM_ADC) ||
        (g_sys.dht_valid && g_sys.temperature >= TEMP_CRITICAL_C)) {
        g_sys.state = STATE_ALARM;
        LED_RED();
    } else if ((g_sys.gas_adc >= GAS_WARN_ADC) ||
               (g_sys.dht_valid && g_sys.temperature >= TEMP_HIGH_C) ||
               (g_sys.dht_valid && g_sys.humidity    >= HUMIDITY_HIGH)) {
        g_sys.state = STATE_WARNING;
        LED_YELLOW();
    } else if (g_sys.relay_override) {
        g_sys.state = STATE_OVERRIDE;
        LED_BLUE();
    } else {
        g_sys.state = STATE_NORMAL;
        LED_GREEN();
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  LCD DISPLAY — 5-page menu
 * ═══════════════════════════════════════════════════════════════════ */
static void display_update(void) {
    char row0[17];
    char row1[17];

    /* Compute fan percentage for display */
    uint8_t fan_pct = (uint8_t)((g_sys.fan_duty * 100U) / (FAN_TIM_PERIOD + 1U));

    switch (g_sys.display_page) {

        /* ─── Page 0: Home / Uptime ─────────────────────────────── */
        case PAGE_HOME: {
            uint32_t h = g_sys.uptime_s / 3600U;
            uint32_t m = (g_sys.uptime_s % 3600U) / 60U;
            uint32_t s = g_sys.uptime_s % 60U;

            LCD_PrintRow(0, "Home Helper STM32");

            row1[0]='U'; row1[1]='p'; row1[2]=':';
            row1[3]=(char)('0'+(h/10U)%10U); row1[4]=(char)('0'+h%10U);
            row1[5]=':';
            row1[6]=(char)('0'+m/10U); row1[7]=(char)('0'+m%10U);
            row1[8]=':';
            row1[9]=(char)('0'+s/10U); row1[10]=(char)('0'+s%10U);
            row1[11]=' ';

            const char *st = "OK ";
            if (g_sys.state == STATE_WARNING) st = "WRN";
            if (g_sys.state == STATE_ALARM)   st = "ALM";
            row1[12]=st[0]; row1[13]=st[1]; row1[14]=st[2]; row1[15]=' ';
            row1[16]='\0';
            LCD_PrintRow(1, row1);
            break;
        }

        /* ─── Page 1: Temperature & Humidity ────────────────────── */
        case PAGE_TEMP_HUM: {
            /* Row 0: "T:25C  H:60%   " */
            if (g_sys.dht_valid) {
                uint8_t t = g_sys.temperature;
                uint8_t h = g_sys.humidity;
                row0[0]='T'; row0[1]=':';
                row0[2]=(char)('0'+t/10); row0[3]=(char)('0'+t%10);
                row0[4]=(char)0xDF;   /* ° character (HD44780 char 0xDF = °) */
                row0[5]='C'; row0[6]=' '; row0[7]=' ';
                row0[8]='H'; row0[9]=':';
                row0[10]=(char)('0'+h/10); row0[11]=(char)('0'+h%10);
                row0[12]='%'; row0[13]=' '; row0[14]=' '; row0[15]=' ';
                row0[16]='\0';
                LCD_PrintRow(0, row0);
            } else {
                LCD_PrintRow(0, "T:-- C  H:--%  ");
            }

            /* Row 1: "Fan:xxx% RLY:ON " */
            row1[0]='F'; row1[1]='a'; row1[2]='n'; row1[3]=':';
            row1[4]=(char)('0'+fan_pct/100);
            row1[5]=(char)('0'+(fan_pct/10)%10);
            row1[6]=(char)('0'+fan_pct%10);
            row1[7]='%'; row1[8]=' ';
            row1[9]='R'; row1[10]='L'; row1[11]='Y'; row1[12]=':';
            if (g_sys.relay_on) {
                row1[13]='O'; row1[14]='N'; row1[15]=' ';
            } else {
                row1[13]='O'; row1[14]='F'; row1[15]='F';
            }
            row1[16]='\0';
            LCD_PrintRow(1, row1);
            break;
        }

        /* ─── Page 2: Gas Sensor ─────────────────────────────────── */
        case PAGE_GAS: {
            LCD_PrintRow(0, "Gas/Smoke Sensor");
            uint16_t g = g_sys.gas_adc;
            row1[0]='V'; row1[1]='a'; row1[2]='l'; row1[3]=':';
            row1[4]=(char)('0'+(g/1000));
            row1[5]=(char)('0'+((g/100)%10));
            row1[6]=(char)('0'+((g/10)%10));
            row1[7]=(char)('0'+(g%10));
            row1[8]=' ';

            if (g >= GAS_ALARM_ADC) {
                row1[9]='A'; row1[10]='L'; row1[11]='A';
                row1[12]='R'; row1[13]='M'; row1[14]='!'; row1[15]='!';
            } else if (g >= GAS_WARN_ADC) {
                row1[9]='W'; row1[10]='A'; row1[11]='R';
                row1[12]='N'; row1[13]='!'; row1[14]=' '; row1[15]=' ';
            } else {
                row1[9]='S'; row1[10]='a'; row1[11]='f';
                row1[12]='e'; row1[13]=' '; row1[14]=' '; row1[15]=' ';
            }
            row1[16]='\0';
            LCD_PrintRow(1, row1);
            break;
        }

        /* ─── Page 3: Light & Motion ─────────────────────────────── */
        case PAGE_LIGHT: {
            LCD_PrintRow(0, "Light & Motion  ");
            uint16_t ldr = g_sys.ldr_adc;
            row1[0]='L'; row1[1]='D'; row1[2]='R'; row1[3]=':';
            row1[4]=(char)('0'+(ldr/1000));
            row1[5]=(char)('0'+((ldr/100)%10));
            row1[6]=(char)('0'+((ldr/10)%10));
            row1[7]=(char)('0'+(ldr%10));
            row1[8]=' ';
            row1[9]='M'; row1[10]='O'; row1[11]='T'; row1[12]=':';
            row1[13] = g_sys.motion_detected ? 'Y' : 'N';
            row1[14]=' '; row1[15]=' '; row1[16]='\0';
            LCD_PrintRow(1, row1);
            break;
        }

        /* ─── Page 4: Settings / Override ───────────────────────── */
        case PAGE_SETTINGS: {
            LCD_PrintRow(0, "Settings/Override");
            LCD_PrintRow(1, g_sys.relay_override ?
                             "RLY:MANUAL      " :
                             "RLY:AUTO        ");
            break;
        }

        default:
            g_sys.display_page = PAGE_HOME;
            break;
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  BUTTON HANDLER (debounced, non-blocking)
 * ═══════════════════════════════════════════════════════════════════ */
static void handle_buttons(void) {
    static uint32_t last_btn_ms = 0U;
    uint32_t now = HAL_GetTick();

    if ((now - last_btn_ms) < DEBOUNCE_MS) return;

    bool pressed = false;

    if (BTN_UP_PRESSED()) {
        if (g_sys.display_page == PAGE_HOME) {
            g_sys.display_page = (DisplayPage_t)(PAGE_COUNT - 1U);
        } else {
            g_sys.display_page = (DisplayPage_t)((uint8_t)g_sys.display_page - 1U);
        }
        LCD_Clear();
        pressed = true;
    }
    else if (BTN_DOWN_PRESSED()) {
        g_sys.display_page = (DisplayPage_t)(
            ((uint8_t)g_sys.display_page + 1U) % (uint8_t)PAGE_COUNT);
        LCD_Clear();
        pressed = true;
    }
    else if (BTN_SEL_PRESSED()) {
        if (!g_sys.relay_override) {
            /* Enter override: relay ON */
            g_sys.relay_override = true;
            g_sys.relay_on       = true;
            RELAY_ON();
            UART_Log("BTN", "Override ON: Relay ON");
        } else if (g_sys.relay_on) {
            /* Override active, relay ON → turn OFF */
            g_sys.relay_on = false;
            RELAY_OFF();
            UART_Log("BTN", "Override: Relay OFF");
        } else {
            /* Override active, relay OFF → exit override */
            g_sys.relay_override = false;
            UART_Log("BTN", "Override OFF: Auto mode");
        }
        pressed = true;
    }

    if (pressed) last_btn_ms = now;
}

/* ═══════════════════════════════════════════════════════════════════
 *  BLUETOOTH COMMAND HANDLER (non-blocking poll)
 *  Commands: R/r=relay, A=auto, F/f=fan, S=status, ?=help
 * ═══════════════════════════════════════════════════════════════════ */
static void uart_status_dump(void) {
    UART_TxLine("=== HOME HELPER STM32 STATUS ===");
    UART_TxStr("Temp: ");     UART_TxUInt(g_sys.temperature);   UART_TxLine("C");
    UART_TxStr("Humidity: "); UART_TxUInt(g_sys.humidity);      UART_TxLine("%");
    UART_TxStr("Gas ADC: ");  UART_TxUInt(g_sys.gas_adc);       UART_TxChar('\n');
    UART_TxStr("LDR ADC: ");  UART_TxUInt(g_sys.ldr_adc);       UART_TxChar('\n');
    UART_TxStr("Motion: ");   UART_TxLine(g_sys.motion_detected ? "YES" : "NO");
    UART_TxStr("Relay: ");    UART_TxLine(g_sys.relay_on ? "ON" : "OFF");
    UART_TxStr("Fan Duty: "); UART_TxUInt((uint16_t)g_sys.fan_duty); UART_TxChar('\n');
    const char *states[] = {"NORMAL", "WARNING", "ALARM", "OVERRIDE"};
    UART_TxStr("State: ");    UART_TxLine(states[(uint8_t)g_sys.state]);
    UART_TxLine("================================");
}

static void handle_bluetooth(void) {
    if (!UART_RxAvailable()) return;

    char cmd = UART_RxChar();
    switch (cmd) {
        case 'R':
            g_sys.relay_override = true;
            g_sys.relay_on       = true;
            RELAY_ON();
            UART_Log("BT", "Relay ON (override)");
            break;
        case 'r':
            g_sys.relay_override = true;
            g_sys.relay_on       = false;
            RELAY_OFF();
            UART_Log("BT", "Relay OFF (override)");
            break;
        case 'A':
            g_sys.relay_override = false;
            UART_Log("BT", "Auto mode restored");
            break;
        case 'F':
            fan_set_duty(FAN_MAX_DUTY);
            g_sys.fan_duty = FAN_MAX_DUTY;
            UART_Log("BT", "Fan MAX");
            break;
        case 'f':
            fan_set_duty(FAN_OFF_DUTY);
            g_sys.fan_duty = 0U;
            UART_Log("BT", "Fan OFF");
            break;
        case 'S':
        case 's':
            uart_status_dump();
            break;
        case '?':
            UART_TxLine("Commands: R=Relay ON  r=Relay OFF  A=Auto");
            UART_TxLine("          F=Fan MAX   f=Fan OFF    S=Status");
            break;
        default:
            break;
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════════════════════════ */
int main(void) {
    /* ── HAL Init ─────────────────────────────────────────────────── */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_TIM3_Init();
    DWT_Delay_Init();

    /* ── Start PWM output on TIM3 CH1 ────────────────────────────── */
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    /* ── Calibrate ADC ───────────────────────────────────────────── */
    HAL_ADCEx_Calibration_Start(&hadc1);

    /* ── Custom driver init ──────────────────────────────────────── */
    LCD_Init();
    UART_Init();   /* Sends boot banner */

    /* ── Initial system state ────────────────────────────────────── */
    memset(&g_sys, 0, sizeof(g_sys));
    g_sys.display_page = PAGE_HOME;
    g_sys.state        = STATE_NORMAL;

    /* ── Splash screen ───────────────────────────────────────────── */
    LCD_PrintRow(0, "Home Helper STM32");
    LCD_PrintRow(1, "  Booting...    ");
    LED_BLUE();
    delay_ms(2000U);
    LCD_Clear();
    LED_GREEN();

    /* ── Timing trackers ──────────────────────────────────────────── */
    uint32_t last_dht_ms     = 0U;
    uint32_t last_sensor_ms  = 0U;
    uint32_t last_display_ms = 0U;
    uint32_t last_uptime_ms  = 0U;

    /* ════════════════════════════════════════════════════════════════
     *  MAIN LOOP
     * ═══════════════════════════════════════════════════════════════ */
    for (;;) {
        /* 1. Bluetooth / UART command handler (non-blocking) */
        handle_bluetooth();

        /* 2. Button press handler (non-blocking, debounced) */
        handle_buttons();

        /* 3. DHT11 read — every 2 seconds */
        if (timer_elapsed(&last_dht_ms, DHT11_READ_INTERVAL_MS)) {
            DHT11_Data_t dht;
            if (DHT11_Read(&dht)) {
                g_sys.temperature = dht.temperature;
                g_sys.humidity    = dht.humidity;
                g_sys.dht_valid   = true;
                UART_TxStr("[DHT] T=");
                UART_TxUInt(g_sys.temperature);
                UART_TxStr("C H=");
                UART_TxUInt(g_sys.humidity);
                UART_TxLine("%");
            } else {
                g_sys.dht_valid = false;
                UART_Log("DHT", "Read error (check wiring)");
            }
        }

        /* 4. Fast sensors (MQ-2, LDR, PIR) — every 500 ms */
        if (timer_elapsed(&last_sensor_ms, SENSOR_POLL_INTERVAL_MS)) {
            g_sys.gas_adc         = ADC_ReadAvg(ADC_CH_GAS, 8U);
            g_sys.ldr_adc         = ADC_ReadAvg(ADC_CH_LDR, 4U);
            g_sys.motion_detected = PIR_READ();

            /* Run control algorithms */
            control_fan();
            control_relay();
            control_buzzer();
            update_led_state();

            /* Log if gas threshold crossed */
            if (g_sys.gas_adc >= GAS_WARN_ADC) {
                UART_TxStr("[GAS] ADC=");
                UART_TxUInt(g_sys.gas_adc);
                UART_TxLine(g_sys.gas_adc >= GAS_ALARM_ADC ? " ALARM!" : " WARN");
            }
        }

        /* 5. LCD refresh — every 1 second */
        if (timer_elapsed(&last_display_ms, DISPLAY_REFRESH_MS)) {
            display_update();
        }

        /* 6. Uptime counter — every 1 second */
        if (timer_elapsed(&last_uptime_ms, 1000U)) {
            g_sys.uptime_s++;
        }
    }
    /* Never reached */
}

/* ═══════════════════════════════════════════════════════════════════
 *  CLOCK CONFIGURATION
 *  HSE 8 MHz → PLL ×9 → SYSCLK 72 MHz
 * ═══════════════════════════════════════════════════════════════════ */
static void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    RCC_PeriphCLKInitTypeDef periph = {0};

    /* Enable HSE and PLL */
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;   /* 8 MHz × 9 = 72 MHz */
    HAL_RCC_OscConfig(&osc);

    /* Select PLL as system clock, configure AHB/APB prescalers */
    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;    /* AHB  = 72 MHz */
    clk.APB1CLKDivider = RCC_HCLK_DIV2;      /* APB1 = 36 MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV1;      /* APB2 = 72 MHz */
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);

    /* ADC clock: APB2 / 6 = 12 MHz (≤ 14 MHz spec) */
    periph.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    periph.AdcClockSelection    = RCC_ADCPCLK2_DIV6;
    HAL_RCCEx_PeriphCLKConfig(&periph);
}

/* ═══════════════════════════════════════════════════════════════════
 *  GPIO INIT
 * ═══════════════════════════════════════════════════════════════════ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef gpio = {0};

    /* Enable all needed GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* ── Outputs: Relay, Buzzer, RGB LED ── */
    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |   /* LED R, G, B   */
        GPIO_PIN_3 |                               /* Relay         */
        GPIO_PIN_4,                                /* Buzzer        */
        GPIO_PIN_RESET);

    gpio.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                 GPIO_PIN_3 | GPIO_PIN_4;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* Relay starts OFF (Active LOW → HIGH = off) */
    HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, GPIO_PIN_SET);

    /* ── PIR: Input, no pull (external) ── */
    gpio.Pin  = GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &gpio);

    /* ── DHT11: Start as output-OD HIGH (bus idle) ── */
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
    gpio.Pin   = DHT11_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);

    /* ── LCD: PB8–PB13 as output push-pull ── */
    gpio.Pin   = GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10 |
                 GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &gpio);
    HAL_GPIO_WritePin(GPIOB,
        GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
        GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13,
        GPIO_PIN_RESET);

    /* ── Buttons: PC13, PC14, PC15 — input pull-up ── */
    gpio.Pin  = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio);

    /* ── PA6: TIM3_CH1 alternate function (configured in MX_TIM3_Init) ── */
    /* ── PA0, PA1: ADC analog (configured in MX_ADC1_Init) ── */
}

/* ═══════════════════════════════════════════════════════════════════
 *  USART1 INIT — PA9 TX, PA10 RX, 115200 8N1
 * ═══════════════════════════════════════════════════════════════════ */
static void MX_USART1_UART_Init(void) {
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUD_RATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ═══════════════════════════════════════════════════════════════════
 *  ADC1 INIT — Software trigger, single conversion mode
 *  Channels configured per read call (in adc_sensor.c)
 * ═══════════════════════════════════════════════════════════════════ */
static void MX_ADC1_Init(void) {
    GPIO_InitTypeDef gpio = {0};

    /* PA0, PA1 as analog inputs */
    gpio.Pin  = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;       /* Single channel */
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1U;
    HAL_ADC_Init(&hadc1);
}

/* ═══════════════════════════════════════════════════════════════════
 *  TIM3 INIT — PWM CH1 on PA6, 1 kHz, 0–999 duty
 *  TIM3 is on APB1 → clock = 72 MHz (APB1 × 2 when APB1 divider > 1)
 * ═══════════════════════════════════════════════════════════════════ */
static void MX_TIM3_Init(void) {
    GPIO_InitTypeDef       gpio   = {0};
    TIM_OC_InitTypeDef     oc_cfg = {0};

    /* PA6 → TIM3_CH1 alternate function push-pull */
    gpio.Pin       = GPIO_PIN_6;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* Timer base */
    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = FAN_TIM_PRESCALER;   /* 72 MHz / 72 = 1 MHz */
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = FAN_TIM_PERIOD;      /* 1 MHz / 1000 = 1 kHz */
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim3);

    /* PWM channel 1 */
    oc_cfg.OCMode       = TIM_OCMODE_PWM1;
    oc_cfg.Pulse        = 0U;       /* Start with fan OFF */
    oc_cfg.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc_cfg.OCFastMode   = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &oc_cfg, TIM_CHANNEL_1);
}

/* ═══════════════════════════════════════════════════════════════════
 *  ERROR HANDLER
 * ═══════════════════════════════════════════════════════════════════ */
void Error_Handler(void) {
    __disable_irq();
    LED_RED();
    BUZZER_ON();
    for (;;) {
        /* Halt on fatal HAL error */
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  SysTick_Handler is provided by HAL in stm32f1xx_it.c
 *  HAL_IncTick() must be called there — provided by STM32CubeIDE
 * ═══════════════════════════════════════════════════════════════════ */
