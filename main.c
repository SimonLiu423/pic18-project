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
#define MOTOR_POS_90_DEG_US 2500
#define MOTOR_NEG_90_DEG_US 500

int flag = 0;
int prev_adc_val = 0;
int state = 0;
int start_val = 0;
int duty_cycle_us = MOTOR_NEG_90_DEG_US;

int pitch_duty_cycles[180];
int degree_delta = 0;
int prev_val = 0;
int base_degree = 0;


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
    ComponentInitialize(COMPONENT_LED | COMPONENT_UART | COMPONENT_PWM | COMPONENT_ADC,
                        &int_config, component_config);
    MotorRotateDegree(0);
    Motor2RotateDegree(0);
}

void main(void) {
    SystemInitialize();
    while(1){
    };
    return;
}

void __interrupt(high_priority) HighIsr(void){
    if(BUTTON_IF){ 
        if(state){
            int next_degree = current_degree + degree_delta;
            if(next_degree > 90){
                next_degree = 90;
            }
            MotorRotateDegree(next_degree);
            Motor2RotateDegree(next_degree);
            UartSendString("Motor degree: ");
            UartSendInt(next_degree);
            UartSendString("\n");
        }else{
            int next_degree = current_degree - degree_delta;
            if(next_degree < -90){
                next_degree = -90;
            }
            MotorRotateDegree(next_degree);
            Motor2RotateDegree(next_degree);
            UartSendString("Motor degree: ");
            UartSendInt(next_degree);
            UartSendString("\n");
        }
        state = !state;
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
//             int pitch = atoi(str);
//             PWMSetDutyCycle(pitch_duty_cycles[pitch]);

            
            base_degree = atoi(str);
            UartSendString("Base degree: ");
            UartSendInt(base_degree);
            UartSendString("\n");

//            int num = atoi(str);
//            LedSet(num);
            UartClearBuffer();
        }
    }
    if(ADC_IF){
        int val = AdcGetResultHigh();
        if(abs(val - prev_val) > 10){
            degree_delta = (long long)(0b11111111 - val) * 90 / 0b11111111;
            prev_val = val;
        }
        AdcStartConversion();
        AdcIntDone();
    }
}