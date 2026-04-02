#include <ioCC2530.h>
#include "led.h"

#define LED1 P1_4
#define LED2 P0_1
#define LED3 P1_0
#define LED4 P1_1

static void LED_WriteRaw(unsigned char led_id, unsigned char level);

static void LED_WriteRaw(unsigned char led_id, unsigned char level)
{
    switch (led_id)
    {
    case LED_ID_1:
        LED1 = level;
        break;
    case LED_ID_2:
        LED2 = level;
        break;
    case LED_ID_3:
        LED3 = level;
        break;
    case LED_ID_4:
        LED4 = level;
        break;
    default:
        break;
    }
}

void LED_Init(void)
{
    P1SEL &= ~0x13;       /* P1.0 (LED3), P1.1 (LED4), P1.4 (LED1) as GPIO */
    P1DIR |= 0x13;        /* P1.0, P1.1, P1.4 output */

    P0SEL &= ~0x02;       /* P0.1 (LED2) as GPIO */
    P0DIR |= 0x02;        /* P0.1 output */

    LED_AllOff();
}

void LED_On(unsigned char led_id)
{
    LED_WriteRaw(led_id, 0);
}

void LED_Off(unsigned char led_id)
{
    LED_WriteRaw(led_id, 1);
}

void LED_AllOff(void)
{
    LED_Off(LED_ID_1);
    LED_Off(LED_ID_2);
    LED_Off(LED_ID_3);
    LED_Off(LED_ID_4);
}

void LED_ShowSingle(unsigned char led_id)
{
    LED_AllOff();
    LED_On(led_id);
}

void LED_Toggle(unsigned char led_id)
{
    switch (led_id)
    {
    case LED_ID_1:
        LED1 ^= 1;
        break;
    case LED_ID_2:
        LED2 ^= 1;
        break;
    case LED_ID_3:
        LED3 ^= 1;
        break;
    case LED_ID_4:
        LED4 ^= 1;
        break;
    default:
        break;
    }
}
