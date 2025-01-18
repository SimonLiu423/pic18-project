#ifndef UART_H
#define UART_H

#include "settings.h"
#include "config.h"

#if _XTAL_FREQ == 4000000
#if UART_BAUD_RATE == 300
    #define SPBRG_VALUE 207
#elif UART_BAUD_RATE == 1200
    #define SPBRG_VALUE 51
#elif UART_BAUD_RATE == 2400
    #define SPBRG_VALUE 25
#elif UART_BAUD_RATE == 9600
    #define SPBRG_VALUE 6
#elif UART_BAUD_RATE == 19200
    #define SPBRG_VALUE 2
#elif UART_BAUD_RATE == 57600
    #define SPBRG_VALUE 0
#elif UART_BAUD_RATE == 115200
    #define SPBRG_VALUE 0
#endif
#else
#define SPBRG_VALUE 0
#endif

#define UART_BUFFER_SIZE 128

void UartInitialize(IntPriority tx_priority, IntPriority rx_priority);
void UartClearBuffer(void);
void UartSendChar(char c);
void UartSendString(char *str);
void UartReceiveChar(void);
char UartGetChar(void);
void UartSendInt(int num);
int UartBufferEndsWith(const char *str);
void UartCopyBufferToString(char *str);
#endif