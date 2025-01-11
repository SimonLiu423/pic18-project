/*
 * File:   main.c
 * Author: simon
 *
 * Created on December 23, 2024, 5:09 PM
 */

#include <xc.h>
#include "utils/settings.h"
#include "utils/led.h"
#include "utils/interrupt_manager.h"
#include "utils/adc.h"
#include "utils/ccp.h"
#include "utils/uart.h"
#include "utils/config.h"
#include "utils/timer.h"
#define MOTOR_PERIOD_MS 20
#define MOTOR_POS_90_DEG_US 2400
#define MOTOR_POS_45_DEG_US 1925
#define MOTOR_0_DEG_US 1450
#define MOTOR_NEG_45_DEG_US 975
#define MOTOR_NEG_90_DEG_US 500

int flag = 0;
int prev_adc_val = 0;
int state = 0;
int start_val = 0;
int duty_cycle_us = MOTOR_NEG_90_DEG_US;


//void putch(char c){
//    UartSendChar(c);
//}
//
//int getch(void){
//    return UartGetChar();
//}

void SystemInitialize(void){
    IntConfig int_config = {
        .button = INTERRUPT_HIGH,
        .adc = INTERRUPT_LOW,
        .timer = INTERRUPT_NONE,
        .uart_tx = INTERRUPT_NONE,
        .uart_rx = INTERRUPT_LOW,
    };
    ComponentConfig component_config = {
        .prescaler = 16,
        .postscaler = 16,
        .timer_period_ms = 1000,
        .pwm_period_ms = MOTOR_PERIOD_MS,
    };

    OscillatorInitialize();
    ComponentInitialize(COMPONENT_LED | COMPONENT_UART,
                        &int_config, component_config);
    PWMSetDutyCycle(MOTOR_0_DEG_US);
}

void main(void) {
    SystemInitialize();
    while(1){
    };
    return;
}

void __interrupt(high_priority) HighIsr(void){
    if(BUTTON_IF){
        ButtonIntDone();
    }
    if(Timer2IF){
        Timer2IntDone();
    }
}

void __interrupt(low_priority) LowIsr(void){
    if(RCIF){
        UartReceiveChar();
        char ch = UartGetChar();

        if(ch == '\r'){
            char str[20];
            UartCopyBufferToString(str);
            duty_cycle_us = atoi(str);
            PWMSetDutyCycle(duty_cycle_us);
            UartClearBuffer();
        }
    }
    if(ADC_IF){
        int val = AdcGetResultHigh();
        PWMSetDutyCycle((MOTOR_POS_90_DEG_US - MOTOR_0_DEG_US) * (double)val / 0b11111111 + MOTOR_0_DEG_US);
        AdcStartConversion();
        AdcIntDone();
    }
}