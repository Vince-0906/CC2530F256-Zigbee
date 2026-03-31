#include <ioCC2530.h>
#include "buzzer.h"

#define BUZZER P1_7

void Buzzer_Init(void)
{
    P1SEL &= ~0x80;
    P1DIR |= 0x80;

    Buzzer_Off();
}

void Buzzer_On(void)
{
    BUZZER = 1;
}

void Buzzer_Off(void)
{
    BUZZER = 0;
}
