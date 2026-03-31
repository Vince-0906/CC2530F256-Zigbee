#include <ioCC2530.h>
#include "adc.h"

void ADC_Init(void)
{
    APCFG |= 0x80;       /* AIN7 (P0.7) analog input enable */
    P0SEL |= 0x80;       /* P0.7 peripheral function */
    P0DIR &= ~0x80;      /* P0.7 input */
    P0INP |= 0x80;       /* P0.7 tri-state (no pull) */
}

unsigned char ADC_ReadChannel7_8bit(void)
{
    unsigned char dummy;

    ADCCON3 = 0xB7;       /* AVDD5 ref, 12-bit, AIN7 */
    while (!(ADCCON1 & 0x80))
    {
    }
    dummy = ADCL;         /* read ADCL first to latch ADCH */
    (void)dummy;
    return ADCH;          /* top 8 bits of 12-bit result */
}
