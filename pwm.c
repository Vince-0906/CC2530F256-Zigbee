#include <ioCC2530.h>
#include "pwm.h"

/*
 * Software PWM for LED2 (P0.1).
 * LED2 is active-low: writing 0 turns it ON, 1 turns it OFF.
 *
 * PWM period = 20 calls to PWM_Process1ms() = 20 ms (50 Hz).
 * duty range: 0-20.
 *   duty = 0   -> LED2 always off
 *   duty = 20  -> LED2 always on (brightest)
 */

#define LED2           P0_1
#define SW_PWM_PERIOD  20

static unsigned char pwm_counter = 0;
static unsigned char pwm_duty = 0;       /* 0 - SW_PWM_PERIOD */

void PWM_Init(void)
{
    /* LED2 GPIO is already configured by LED_Init().
     * Just ensure initial state is off. */
    LED2 = 1;       /* active-low: 1 = off */
    pwm_counter = 0;
    pwm_duty = 0;
}

void PWM_SetDuty(unsigned char duty)
{
    /* Map 0-255 input to 0-SW_PWM_PERIOD range */
    pwm_duty = (unsigned char)((unsigned short)duty * SW_PWM_PERIOD / 255);
}

void PWM_Process1ms(void)
{
    pwm_counter++;
    if (pwm_counter >= SW_PWM_PERIOD)
    {
        pwm_counter = 0;
    }

    if (pwm_duty == 0)
    {
        LED2 = 1;       /* fully off */
    }
    else if (pwm_counter < pwm_duty)
    {
        LED2 = 0;       /* on (active-low) */
    }
    else
    {
        LED2 = 1;       /* off */
    }
}
