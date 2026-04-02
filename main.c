#include <ioCC2530.h>
#include "led.h"
#include "buzzer.h"
#include "key.h"
#include "adc.h"
#include "pwm.h"
#include "dht11.h"
#include "uart.h"

/* ---------- Timer4 1ms tick configuration ---------- */
#define TIMER4_CH0_FLAG   0x10
#define TIMER4_TICK_1MS   249

/* ---------- Light sensor (ADC) config ---------- */
#define LIGHT_SAMPLE_MS    50
#define ADC_DARK_THRESH    245
#define ADC_BRIGHT_THRESH  10

/* ---------- DHT11 sampling interval ---------- */
#define DHT11_INTERVAL_MS  2000

/* ---------- Alarm timing ---------- */
#define ALARM_BUZZER_MS    200     /* buzzer toggle every 200ms (beep-beep) */
#define ALARM_LED_MS       100     /* LED1 toggle every 100ms (fast flash) */

/* ---------- UART command buffer ---------- */
#define CMD_BUF_SIZE       16

/* ====================================================================== */
/*                          Global Variables                              */
/* ====================================================================== */

/* Alarm thresholds (settable from PC) */
static unsigned char alarm_temp_th = 40;   /* default 28 C */
static unsigned char alarm_humi_th = 60;   /* default 60 % */

/* Alarm state */
static volatile unsigned char alarm_active = 0;
static volatile unsigned int  alarm_buzzer_tick = 0;
static volatile unsigned int  alarm_led_tick = 0;

/* Light control */
static volatile unsigned char light_tick = 0;
static volatile unsigned char last_light_adc = 0;

/* DHT11 interval counter */
static volatile unsigned int dht11_tick = 0;
static volatile unsigned char dht11_ready = 0;

/* Latest sensor readings */
static unsigned char cur_temp = 0;
static unsigned char cur_humi = 0;

/* UART command receive buffer */
static unsigned char cmd_buf[CMD_BUF_SIZE];
static unsigned char cmd_idx = 0;

/* ====================================================================== */
/*                       Forward Declarations                             */
/* ====================================================================== */
static void Clock_Init32MHz(void);
static void Timer4_Init1ms(void);
static unsigned char LightCtrl_Map(unsigned char adc_val);
static void LightCtrl_Process1ms(void);
static void Alarm_Process1ms(void);
static void UART_SendSensorData(void);
static void UART_ParseCommand(void);
static unsigned char ParseNumber(const unsigned char *str, unsigned char len);

/* ====================================================================== */
/*                        Clock & Timer Init                              */
/* ====================================================================== */

static void Clock_Init32MHz(void)
{
    CLKCONCMD &= (unsigned char)~0x40;
    while (CLKCONSTA & 0x40)
    {
    }
    CLKCONCMD &= (unsigned char)~0x3F;
}

static void Timer4_Init1ms(void)
{
    T4CTL = 0x00;
    T4CCTL0 = 0x44;
    T4CC0 = TIMER4_TICK_1MS;
    TIMIF &= (unsigned char)~0x18;
    IEN1 |= 0x10;

    T4CTL = 0xE2;       /* prescaler /128, modulo, clear counter */
    T4CTL |= 0x04;      /* clear counter */
    T4CTL |= 0x10;      /* start */
}

/* ====================================================================== */
/*                  Light Sensor -> Software PWM                          */
/* ====================================================================== */

static unsigned char LightCtrl_Map(unsigned char adc_val)
{
    unsigned short duty;

    if (adc_val <= ADC_BRIGHT_THRESH)
    {
        return 0;
    }
    if (adc_val >= ADC_DARK_THRESH)
    {
        return 255;
    }

    duty = (unsigned short)(adc_val - ADC_BRIGHT_THRESH) * 255;
    duty /= (unsigned short)(ADC_DARK_THRESH - ADC_BRIGHT_THRESH);
    return (unsigned char)duty;
}

static void LightCtrl_Process1ms(void)
{
    light_tick++;
    if (light_tick >= LIGHT_SAMPLE_MS)
    {
        light_tick = 0;
        last_light_adc = ADC_ReadChannel7_8bit();
        PWM_SetDuty(LightCtrl_Map(last_light_adc));
    }
}

/* ====================================================================== */
/*              Alarm: Buzzer beeping + LED1 fast flash                   */
/* ====================================================================== */

static void Alarm_Process1ms(void)
{
    if (alarm_active == 0)
    {
        return;
    }

    /* Buzzer beep-beep */
    alarm_buzzer_tick++;
    if (alarm_buzzer_tick >= ALARM_BUZZER_MS)
    {
        alarm_buzzer_tick = 0;
        /* Toggle buzzer */
        if (P1_7 == 1)
        {
            Buzzer_Off();
        }
        else
        {
            Buzzer_On();
        }
    }

    /* LED1 fast flash */
    alarm_led_tick++;
    if (alarm_led_tick >= ALARM_LED_MS)
    {
        alarm_led_tick = 0;
        LED_Toggle(LED_ID_1);
    }
}

/* ====================================================================== */
/*                      UART Data Output                                  */
/* ====================================================================== */

static void UART_SendSensorData(void)
{
    /* Format: "T:25 H:60 L:180\r\n" */
    UART_SendString("T:");
    UART_SendNumber(cur_temp);
    UART_SendString(" H:");
    UART_SendNumber(cur_humi);
    UART_SendString(" L:");
    UART_SendNumber(last_light_adc);
    UART_SendString("\r\n");
}

/* ====================================================================== */
/*                     UART Command Parsing                               */
/* ====================================================================== */

/*
 * Parse a decimal number from a byte array.
 * Returns the parsed value (0 if no valid digits found).
 */
static unsigned char ParseNumber(const unsigned char *str, unsigned char len)
{
    unsigned char i;
    unsigned short val = 0;

    for (i = 0; i < len; i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
        {
            val = val * 10 + (str[i] - '0');
        }
        else
        {
            break;
        }
    }
    if (val > 255) val = 255;
    return (unsigned char)val;
}

/*
 * Try to parse a complete command from cmd_buf.
 * Supported commands (terminated by '\n'):
 *   TH:30   -> set temperature threshold to 30
 *   HH:70   -> set humidity threshold to 70
 */
static void UART_ParseCommand(void)
{
    unsigned char byte;

    while (UART_GetRxByte(&byte))
    {
        if (byte == '\n' || byte == '\r')
        {
            if (cmd_idx >= 4)
            {
                /* Check for "TH:xx" */
                if (cmd_buf[0] == 'T' && cmd_buf[1] == 'H' && cmd_buf[2] == ':')
                {
                    alarm_temp_th = ParseNumber(&cmd_buf[3], cmd_idx - 3);
                    UART_SendString("SET TEMP_TH=");
                    UART_SendNumber(alarm_temp_th);
                    UART_SendString("\r\n");
                }
                /* Check for "HH:xx" */
                else if (cmd_buf[0] == 'H' && cmd_buf[1] == 'H' && cmd_buf[2] == ':')
                {
                    alarm_humi_th = ParseNumber(&cmd_buf[3], cmd_idx - 3);
                    UART_SendString("SET HUMI_TH=");
                    UART_SendNumber(alarm_humi_th);
                    UART_SendString("\r\n");
                }
            }
            cmd_idx = 0;   /* reset for next command */
        }
        else
        {
            if (cmd_idx < CMD_BUF_SIZE - 1)
            {
                cmd_buf[cmd_idx++] = byte;
            }
            else
            {
                cmd_idx = 0;   /* overflow, discard */
            }
        }
    }
}

/* ====================================================================== */
/*                            Main                                        */
/* ====================================================================== */

void main(void)
{
    Clock_Init32MHz();
    LED_Init();
    Buzzer_Init();
    Key_Init();
    ADC_Init();
    PWM_Init();
    DHT11_Init();
    UART_Init();
    Timer4_Init1ms();
    EA = 1;

    /* Send startup message */
    UART_SendString("CC2530 Monitor Ready\r\n");
    UART_SendString("TEMP_TH=");
    UART_SendNumber(alarm_temp_th);
    UART_SendString(" HUMI_TH=");
    UART_SendNumber(alarm_humi_th);
    UART_SendString("\r\n");

    while (1)
    {
        /* ----- DHT11 sampling (triggered by timer tick) ----- */
        if (dht11_ready)
        {
            unsigned char result;
            dht11_ready = 0;

            /*
             * DHT11 uses a timing-critical single-wire protocol.
             * Interrupts MUST be disabled during the read to prevent
             * Timer4 / UART ISR from corrupting the microsecond timing.
             */
            EA = 0;                 /* disable all interrupts */
            result = DHT11_Read(&cur_temp, &cur_humi);
            EA = 1;                 /* re-enable interrupts */

            if (result == 0)
            {
                /* Check alarm condition: temp OR humi exceeds threshold */
                if (cur_temp >= alarm_temp_th || cur_humi >= alarm_humi_th)
                {
                    if (alarm_active == 0)
                    {
                        alarm_active = 1;
                        alarm_buzzer_tick = 0;
                        alarm_led_tick = 0;
                    }
                }
                else
                {
                    if (alarm_active)
                    {
                        alarm_active = 0;
                        Buzzer_Off();
                        LED_Off(LED_ID_1);
                    }
                }

                /* Send sensor data via UART */
                UART_SendSensorData();
            }
            else
            {
                /* DHT11 read failed - send error for debugging */
                UART_SendString("DHT11 ERR:");
                UART_SendNumber(result);
                UART_SendString("\r\n");
            }
        }

        /* ----- Parse incoming UART commands ----- */
        UART_ParseCommand();
    }
}

/* ====================================================================== */
/*                      Timer4 ISR (1ms tick)                              */
/* ====================================================================== */

#pragma vector = 0x63
__interrupt void Timer4_ISR(void)
{
    if (TIMIF & TIMER4_CH0_FLAG)
    {
        TIMIF &= (unsigned char)~TIMER4_CH0_FLAG;

        Key_Process1ms();
        LightCtrl_Process1ms();
        PWM_Process1ms();
        Alarm_Process1ms();

        /* DHT11 interval counter */
        dht11_tick++;
        if (dht11_tick >= DHT11_INTERVAL_MS)
        {
            dht11_tick = 0;
            dht11_ready = 1;    /* signal main loop to read DHT11 */
        }
    }
}
