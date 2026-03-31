#include <ioCC2530.h>
#include "key.h"

#define SW1_MASK            0x20
#define SW2_MASK            0x40
#define KEY_SCAN_IDLE       0
#define KEY_SCAN_PRESS      1
#define KEY_SCAN_RELEASE    2
#define KEY_SCAN_REL_CHECK  3
#define KEY_DEBOUNCE_MS     20

static volatile unsigned char key_scan_state = KEY_SCAN_IDLE;
static volatile unsigned char key_debounce_count = 0;
static volatile unsigned char key_candidate = KEY_EVENT_NONE;
static volatile unsigned char key_event = KEY_EVENT_NONE;

static unsigned char Key_ReadPressed(void);
static unsigned char Key_SelectCandidate(unsigned char pressed_mask);
static void Key_EnableInterrupt(void);

static unsigned char Key_ReadPressed(void)
{
    unsigned char pressed_mask = 0;

    if ((P1 & SW1_MASK) == 0)
    {
        pressed_mask |= SW1_MASK;
    }

    if ((P1 & SW2_MASK) == 0)
    {
        pressed_mask |= SW2_MASK;
    }

    return pressed_mask;
}

static unsigned char Key_SelectCandidate(unsigned char pressed_mask)
{
    if (pressed_mask == SW1_MASK)
    {
        return KEY_EVENT_SW1;
    }

    if (pressed_mask == SW2_MASK)
    {
        return KEY_EVENT_SW2;
    }

    return KEY_EVENT_NONE;
}

static void Key_EnableInterrupt(void)
{
    P1IFG &= ~(SW1_MASK | SW2_MASK);
    IRCON2 &= (unsigned char)~0x08;
    P1IEN |= (SW1_MASK | SW2_MASK);
}

void Key_Init(void)
{
    P1SEL &= ~(SW1_MASK | SW2_MASK);
    P1DIR &= ~(SW1_MASK | SW2_MASK);
    P1INP &= ~(SW1_MASK | SW2_MASK);
    P2INP &= (unsigned char)~0x40;

    PICTL |= 0x04;

    key_scan_state = KEY_SCAN_IDLE;
    key_debounce_count = 0;
    key_candidate = KEY_EVENT_NONE;
    key_event = KEY_EVENT_NONE;

    Key_EnableInterrupt();
    IEN2 |= 0x10;
}

void Key_Process1ms(void)
{
    unsigned char pressed_mask = Key_ReadPressed();

    switch (key_scan_state)
    {
    case KEY_SCAN_IDLE:
        break;

    case KEY_SCAN_PRESS:
        key_debounce_count++;
        if (key_debounce_count >= KEY_DEBOUNCE_MS)
        {
            key_debounce_count = 0;
            key_candidate = Key_SelectCandidate(pressed_mask);

            if (pressed_mask == 0)
            {
                key_scan_state = KEY_SCAN_IDLE;
                Key_EnableInterrupt();
            }
            else
            {
                key_scan_state = KEY_SCAN_RELEASE;
            }
        }
        break;

    case KEY_SCAN_RELEASE:
        if ((pressed_mask != 0) && (Key_SelectCandidate(pressed_mask) == KEY_EVENT_NONE))
        {
            key_candidate = KEY_EVENT_NONE;
        }

        if (pressed_mask == 0)
        {
            key_scan_state = KEY_SCAN_REL_CHECK;
            key_debounce_count = 0;
        }
        break;

    case KEY_SCAN_REL_CHECK:
        key_debounce_count++;
        if (pressed_mask != 0)
        {
            key_scan_state = KEY_SCAN_RELEASE;
            key_debounce_count = 0;
        }
        else if (key_debounce_count >= KEY_DEBOUNCE_MS)
        {
            key_debounce_count = 0;
            key_event = key_candidate;
            key_candidate = KEY_EVENT_NONE;
            key_scan_state = KEY_SCAN_IDLE;
            Key_EnableInterrupt();
        }
        break;

    default:
        key_scan_state = KEY_SCAN_IDLE;
        key_debounce_count = 0;
        key_candidate = KEY_EVENT_NONE;
        Key_EnableInterrupt();
        break;
    }
}

unsigned char Key_GetEvent(void)
{
    unsigned char event = key_event;
    key_event = KEY_EVENT_NONE;
    return event;
}

#pragma vector = 0x7B
__interrupt void Port1_ISR(void)
{
    P1IEN &= ~(SW1_MASK | SW2_MASK);
    P1IFG &= ~(SW1_MASK | SW2_MASK);
    IRCON2 &= (unsigned char)~0x08;

    if (key_scan_state == KEY_SCAN_IDLE)
    {
        key_scan_state = KEY_SCAN_PRESS;
        key_debounce_count = 0;
        key_candidate = KEY_EVENT_NONE;
    }
}
