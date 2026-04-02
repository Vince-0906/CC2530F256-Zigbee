#include <ioCC2530.h>
#include "uart.h"

#define UART_RX_BUF_SIZE  32

static volatile unsigned char rx_buf[UART_RX_BUF_SIZE];
static volatile unsigned char rx_head = 0;
static volatile unsigned char rx_tail = 0;

void UART_Init(void)
{
    /* USART0 Alt 1: P0.2 = TX, P0.3 = RX */
    PERCFG &= ~0x01;       /* USART0 Alt 1 */
    P0SEL |= 0x0C;         /* P0.2, P0.3 peripheral function */
    P0DIR |= 0x04;         /* P0.2 (TX) output */
    P0DIR &= ~0x08;        /* P0.3 (RX) input */

    /* UART mode, receiver enable */
    U0CSR = 0xC0;           /* bit7=UART mode, bit6=RX enable */

    /*
     * Baud rate 115200 at 32 MHz system clock:
     * BAUD_M = 216, BAUD_E = 11
     */
    U0BAUD = 216;
    U0GCR = 11;

    /* 8-N-1: 8 data bits, no parity, 1 stop bit (default) */
    U0UCR = 0x02;           /* flush, 8-N-1 */

    /* Enable USART0 RX interrupt */
    URX0IF = 0;
    URX0IE = 1;

    rx_head = 0;
    rx_tail = 0;
}

void UART_SendByte(unsigned char byte)
{
    U0DBUF = byte;
    while (!(U0CSR & 0x02))   /* wait for TX_BYTE flag */
    {
    }
    U0CSR &= ~0x02;           /* clear TX_BYTE flag */
}

void UART_SendString(const char *str)
{
    while (*str != '\0')
    {
        UART_SendByte((unsigned char)*str);
        str++;
    }
}

void UART_SendNumber(unsigned char num)
{
    unsigned char hundreds, tens, ones;

    hundreds = num / 100;
    tens = (num % 100) / 10;
    ones = num % 10;

    if (hundreds > 0)
    {
        UART_SendByte('0' + hundreds);
    }
    UART_SendByte('0' + tens);
    UART_SendByte('0' + ones);
}

unsigned char UART_GetRxByte(unsigned char *byte)
{
    if (rx_head == rx_tail)
    {
        return 0;   /* buffer empty */
    }
    *byte = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) % UART_RX_BUF_SIZE;
    return 1;
}

/* USART0 RX interrupt */
#pragma vector = 0x13
__interrupt void UART0_RX_ISR(void)
{
    unsigned char next_head;

    URX0IF = 0;
    next_head = (rx_head + 1) % UART_RX_BUF_SIZE;

    if (next_head != rx_tail)   /* buffer not full */
    {
        rx_buf[rx_head] = U0DBUF;
        rx_head = next_head;
    }
    /* if full, discard the byte */
}
