#include <ioCC2530.h>
#include "pwm.h"

/*
 * Software PWM for LED2 (P0.1).
 * LED2 is active-low: writing 0 turns it ON, 1 turns it OFF.
 * PWM period = 256 calls to PWM_Process1ms() = 256 ms (~3.9 Hz).
 * duty = 0  -> LED2 always off
 * duty = 255 -> LED2 always on (brightest)
 */

#define LED2  P0_1

static unsigned char pwm_counter = 0;
static unsigned char pwm_duty = 0;

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
    pwm_duty = duty;
}

void PWM_Process1ms(void)
{
    pwm_counter++;
    /* pwm_counter wraps 0-255 automatically (unsigned char) */

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
