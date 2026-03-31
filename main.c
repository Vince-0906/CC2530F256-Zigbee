#include <ioCC2530.h>
#include "led.h"
#include "buzzer.h"
#include "key.h"
#include "adc.h"
#include "pwm.h"

#define TIMER4_CH0_FLAG   0x10
#define TIMER4_TICK_1MS   249
#define LED_STEP_MS       200

#define LIGHT_SAMPLE_MS    50
#define ADC_DARK_THRESH    245
#define ADC_BRIGHT_THRESH  10

static volatile unsigned char led_running = 0;
static volatile unsigned char current_led = LED_ID_2;
static volatile unsigned int led_tick_count = 0;
static volatile unsigned char light_tick = 0;

static void Clock_Init32MHz(void);
static void Timer4_Init1ms(void);
static void App_StartRunningLight(void);
static void App_StopRunningLight(void);
static void App_Process1ms(void);
static unsigned char LightCtrl_Map(unsigned char adc_val);
static void LightCtrl_Process1ms(void);

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

    T4CTL = 0xE2;
    T4CTL |= 0x04;
    T4CTL |= 0x10;
}

static void App_StartRunningLight(void)
{
    led_running = 1;
    current_led = LED_ID_2;
    led_tick_count = 0;
    LED_ShowSingle(current_led);
}

static void App_StopRunningLight(void)
{
    led_running = 0;
    led_tick_count = 0;
    LED_AllOff();
}

static void App_Process1ms(void)
{
    unsigned char key_event = Key_GetEvent();

    if (key_event == KEY_EVENT_SW1)
    {
        App_StartRunningLight();
    }
    else if (key_event == KEY_EVENT_SW2)
    {
        App_StopRunningLight();
    }

    if (led_running == 0)
    {
        return;
    }

    led_tick_count++;
    if (led_tick_count >= LED_STEP_MS)
    {
        led_tick_count = 0;
        current_led++;
        if (current_led > LED_ID_4)
        {
            current_led = LED_ID_2;
        }
        LED_ShowSingle(current_led);
    }
}

void main(void)
{
    Clock_Init32MHz();
    LED_Init();
    Buzzer_Init();
    Key_Init();
    PWM_Init();
    ADC_Init();
    Timer4_Init1ms();
    EA = 1;

    while (1)
    {
    }
}

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
        PWM_SetDuty(LightCtrl_Map(ADC_ReadChannel7_8bit()));
    }
}

#pragma vector = 0x63
__interrupt void Timer4_ISR(void)
{
    if (TIMIF & TIMER4_CH0_FLAG)
    {
        TIMIF &= (unsigned char)~TIMER4_CH0_FLAG;
        Key_Process1ms();
        App_Process1ms();
        LightCtrl_Process1ms();
    }
}
