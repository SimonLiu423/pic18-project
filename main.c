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
    ComponentInitialize(COMPONENT_LED | COMPONENT_UART | COMPONENT_PWM | COMPONENT_ADC | COMPONENT_BUTTON,
                        &int_config, component_config);
    MotorRotateDegree(-90);
    Motor2RotateDegree(0);
}

void main(void) {
    SystemInitialize();
    while(1){
        __delay_ms(200);
        AdcStartConversion();
    };
    return;
}

void __interrupt(high_priority) HighIsr(void){
    if(BUTTON_IF){ 
        if(state){
            int next_degree = base_degree + degree_delta;
            if(next_degree > 90){
                next_degree = 90;
            }
            Motor2RotateDegree(next_degree);
            UartSendString("Motor degree: ");
            UartSendInt(next_degree);
            UartSendString("\n\r");
        }else{
            int next_degree = base_degree - degree_delta;
            if(next_degree < -90){
                next_degree = -90;
            }
            Motor2RotateDegree(next_degree);
            UartSendString("Motor degree: ");
            UartSendInt(next_degree);
            UartSendString("\n\r");
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

            char *token = strtok(str, " ");
            if (token != NULL) {
                if(strcmp(token, "pitch") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        // int pitch = atoi(token);
                        // PWMSetDutyCycle(pitch_duty_cycles[pitch]);
                        int degree = atoi(token);
                        if(-90 <= degree && degree <= 90){
                            MotorRotateDegree(degree);
                        }
                    }
                } else if(strcmp(token, "base") == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        int degree = atoi(token);
                        if(-90 <= degree && degree <= 90){
                            base_degree = degree;
                            Motor2RotateDegree(base_degree);
                            UartSendString("Base degree: ");
                            UartSendInt(base_degree);
                            UartSendString("\n\r");
                        } else {
                            UartSendString("Base degree must be between -90 and 90\n\r");
                        }
                    }
                }
            }
            UartClearBuffer();
        }
    }
    if(ADC_IF){
        int val = AdcGetResultHigh();
        if(abs(val - prev_val) > 10){
            degree_delta = (long long)(0b11111111 - val) * 90 / 0b11111111;
            // UartSendString("Degree delta: ");
            // UartSendInt(degree_delta);
            // UartSendString("\n\r");
            prev_val = val;
        }
        AdcIntDone();
    }
}