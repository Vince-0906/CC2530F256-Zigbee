#include <ioCC2530.h>
#include "pwm.h"

void PWM_Init(void)
{
    /* Route Timer3 Ch1 to P1.4 (Alt 1) */
    PERCFG &= ~0x20;     /* Timer3 Alt 1: T3CH1 -> P1.4 */
    P1SEL |= 0x10;       /* P1.4 peripheral function */
    P1DIR |= 0x10;       /* P1.4 output */
    P2SEL |= 0x20;       /* Timer3 priority over USART1 on Port 1 */

    /* Configure Timer3: prescaler /128, free-running, ~977 Hz PWM */
    T3CTL = 0x00;         /* stop timer */
    T3CCTL1 = 0x1C;      /* Ch1 compare mode, CMP=011 (set on compare, clear on 0) */
    T3CC1 = 0;            /* initial duty = 0 (LED off) */
    T3CTL = 0xE4;         /* prescaler /128, free-running, clear counter */
    T3CTL |= 0x10;        /* start timer */
}

void PWM_SetDuty(unsigned char duty)
{
    T3CC1 = duty;
}
