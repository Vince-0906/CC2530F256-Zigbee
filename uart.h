#ifndef UART_H
#define UART_H

void UART_Init(void);
void UART_SendByte(unsigned char byte);
void UART_SendString(const char *str);
void UART_SendNumber(unsigned char num);

/*
 * Non-blocking receive. Returns 1 if a byte was available, 0 otherwise.
 * The received byte is stored in *byte.
 */
unsigned char UART_GetRxByte(unsigned char *byte);

#endif
