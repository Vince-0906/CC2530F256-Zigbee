#include <ioCC2530.h>
#include "dht11.h"

/* DHT11 data pin: P2.0 */
#define DHT11_PIN   P2_0
#define DHT11_DIR   P2DIR
#define DHT11_BIT   0x01

/*
 * Microsecond-level delay.
 * At 32 MHz, 1 instruction cycle = 1/32 us.
 * Each NOP takes 1 cycle; a tight loop iteration is ~4 cycles.
 * So ~8 NOPs per us.  We use a calibrated loop.
 */
static void Delay_us(unsigned int us)
{
    while (us--)
    {
        asm("NOP"); asm("NOP"); asm("NOP"); asm("NOP");
        asm("NOP"); asm("NOP"); asm("NOP"); asm("NOP");
    }
}

static void Delay_ms(unsigned int ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

/* Set DHT11 pin as output */
static void DHT11_SetOutput(void)
{
    DHT11_DIR |= DHT11_BIT;
}

/* Set DHT11 pin as input */
static void DHT11_SetInput(void)
{
    DHT11_DIR &= ~DHT11_BIT;
}

void DHT11_Init(void)
{
    P2SEL &= ~DHT11_BIT;   /* P2.0 as GPIO */
    DHT11_SetOutput();
    DHT11_PIN = 1;          /* idle high */
}

/*
 * Read one byte (8 bits) from DHT11.
 * Each bit starts with ~50us low, then:
 *   - 26-28us high = '0'
 *   - 70us high = '1'
 * We sample after ~30us to distinguish.
 */
static unsigned char DHT11_ReadByte(void)
{
    unsigned char i;
    unsigned char byte_val = 0;
    unsigned char timeout;

    for (i = 0; i < 8; i++)
    {
        byte_val <<= 1;

        /* Wait for low period to end */
        timeout = 100;
        while ((DHT11_PIN == 0) && (timeout > 0))
        {
            Delay_us(1);
            timeout--;
        }
        if (timeout == 0) return 0;

        /* Delay 30us then sample */
        Delay_us(30);

        if (DHT11_PIN == 1)
        {
            byte_val |= 0x01;
            /* Wait for high period to end */
            timeout = 100;
            while ((DHT11_PIN == 1) && (timeout > 0))
            {
                Delay_us(1);
                timeout--;
            }
        }
    }
    return byte_val;
}

unsigned char DHT11_Read(unsigned char *temp, unsigned char *humi)
{
    unsigned char humi_int, humi_dec, temp_int, temp_dec, checksum;
    unsigned char timeout;

    /* --- Start signal: pull low >= 18ms, then release --- */
    DHT11_SetOutput();
    DHT11_PIN = 0;
    Delay_ms(20);           /* hold low 20ms */
    DHT11_PIN = 1;
    Delay_us(30);           /* wait 20-40us for DHT11 response */
    DHT11_SetInput();

    /* --- Wait for DHT11 response: low ~80us --- */
    timeout = 100;
    while ((DHT11_PIN == 1) && (timeout > 0))
    {
        Delay_us(1);
        timeout--;
    }
    if (timeout == 0) return 1;   /* no response */

    timeout = 100;
    while ((DHT11_PIN == 0) && (timeout > 0))
    {
        Delay_us(1);
        timeout--;
    }
    if (timeout == 0) return 1;

    /* --- Wait for DHT11 response: high ~80us --- */
    timeout = 100;
    while ((DHT11_PIN == 1) && (timeout > 0))
    {
        Delay_us(1);
        timeout--;
    }
    if (timeout == 0) return 1;

    /* --- Read 40 bits (5 bytes) --- */
    humi_int = DHT11_ReadByte();
    humi_dec = DHT11_ReadByte();
    temp_int = DHT11_ReadByte();
    temp_dec = DHT11_ReadByte();
    checksum = DHT11_ReadByte();

    /* Restore pin to output high (idle) */
    DHT11_SetOutput();
    DHT11_PIN = 1;

    /* Verify checksum */
    if (checksum != (unsigned char)(humi_int + humi_dec + temp_int + temp_dec))
    {
        return 2;   /* checksum error */
    }

    *humi = humi_int;
    *temp = temp_int;
    return 0;
}
