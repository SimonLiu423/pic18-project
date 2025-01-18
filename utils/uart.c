#include "uart.h"
#include "settings.h"
#include <stdio.h>
#include <xc.h>
#include <string.h>

char uart_buffer[UART_BUFFER_SIZE];
int uart_buffer_idx = 0;


void SetBaudRate(void){
    // works only for 4MHz oscillator
    TXSTAbits.SYNC = 0;           
    BAUDCONbits.BRG16 = 0;          
    TXSTAbits.BRGH = 0;
    SPBRG = SPBRG_VALUE;
}

void TxEnableInterrupt(IntPriority priority){
    IPR1bits.TXIP = priority;
    PIE1bits.TXIE = 1;
}

void RxEnableInterrupt(IntPriority priority){
    IPR1bits.RCIP = priority;
    PIE1bits.RCIE = 1;
}

void UartInitialize(IntPriority tx_priority, IntPriority rx_priority){
    TRISCbits.RC6 = 1;
    TRISCbits.RC7 = 1;

    SetBaudRate();

    //   Serial enable
    RCSTAbits.SPEN = 1; // enable async serial port
    TXSTAbits.TXEN = 1; // enable transmitter

    PIR1bits.TXIF = 1; // clear TX interrupt flag
    PIR1bits.RCIF = 0; // clear RX interrupt flag
    RCSTAbits.CREN = 1; // cleared when error detected
    
    // interrupt
    if(tx_priority != INTERRUPT_NONE){
        TxEnableInterrupt(tx_priority);
    }
    if(rx_priority != INTERRUPT_NONE){
        RxEnableInterrupt(rx_priority);
    }
}

void UartClearBuffer(void){
    uart_buffer_idx = 0;
    uart_buffer[0] = '\0';
}

void UartSendChar(char c){
    while(TXSTAbits.TRMT == 0); // wait for previous transmission to finish
    TXREG = c;
}

void UartSendString(char *str){
    for(int i = 0; str[i] != '\0'; i++){
        UartSendChar(str[i]);
    }
}

void UartSendInt(int num){
    char str[10];
    sprintf(str, "%d", num);
    UartSendString(str);
}

void UartReceiveChar(void){
    if(RCSTAbits.OERR == 1){
        // clear overrun error
        RCSTAbits.CREN = 0;
        RCSTAbits.CREN = 1;
    }
    char c = RCREG;
    if(c == '\r') UartSendChar('\n');
    uart_buffer[uart_buffer_idx++] = c;
    UartSendChar(c);
}

char UartGetChar(void){
    if(uart_buffer_idx == 0) return '\0';
    return uart_buffer[uart_buffer_idx - 1];
}

int UartBufferEndsWith(const char *str) {
    int str_len = strlen(str);
    
    // Check if buffer has enough characters
    if (uart_buffer_idx < str_len) {
        return 0;
    }
    
    // Compare str with end of buffer
    for (int i = 0; i < str_len; i++) {
        if (uart_buffer[uart_buffer_idx - str_len + i] != str[i]) {
            return 0;
        }
    }
    return 1;
}

void UartCopyBufferToString(char *str) {
    for(int i = 0; i < uart_buffer_idx; i++) {
        str[i] = uart_buffer[i];
    }
    str[uart_buffer_idx] = '\0';
}
