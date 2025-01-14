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
#include <string.h>
#include <stdio.h>

#define MOTOR_PERIOD_MS 20

int flag = 0;
int prev_adc_val = 0;
int pick_state = 0;
int pitch_state = 0;
int start_val = 0;
int duty_cycle_us = MOTOR_NEG_90_DEG_US;

int pitch_degree_table[180];
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
    ComponentInitialize(COMPONENT_LED | COMPONENT_UART | COMPONENT_PWM | COMPONENT_BUTTON,
                        &int_config, component_config);
    MotorRotateDegree(-80);
    Motor2RotateDegree(0);
}

void rotate_pick_motor(){
    if(pick_state){
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
    pick_state = !pick_state;
}

void delay(int ms){
    if((ms) & 1){
        __delay_ms(1);
    }
    if((ms) & 2){
        __delay_ms(2);
    }
    if((ms) & 4){
        __delay_ms(4);
    }
    if((ms) & 8){
        __delay_ms(8);
    }
    if((ms) & 0x10){
        __delay_ms(16);
    }
    if((ms) & 0x20){
        __delay_ms(32);
    }
    if((ms) & 0x40){
        __delay_ms(64);
    }
    if((ms) & 0x80){
        __delay_ms(128);
    }
    if((ms) & 0x100){
        __delay_ms(256);
    }
    if((ms) & 0x200){
        __delay_ms(512);
    }
    if((ms) & 0x400){
        __delay_ms(1024);
    }
    if((ms) & 0x800){
        __delay_ms(2048);
    }
}

void play_midi(char *str){
    char *token = strtok(str, " ");
    while(token != NULL){
        int pwm_val, delay_val;
        char *token2 = strtok(token, ",");
        if (token2 == NULL) break;
        pwm_val = atoi(token2);
        
        token2 = strtok(NULL, " ");
        if (token2 == NULL) break;
        delay_val = atoi(token2);

        PWMSetDutyCycle(pwm_val);
        __delay_ms(5);
        UartSendString("Playing note: ");
        UartSendInt(pwm_val);
        UartSendString("\n\r");
        rotate_pick_motor();
        delay(delay_val);

        token = strtok(NULL, " ");
    }
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
        rotate_pick_motor();
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

        // enter received
        if(ch == '\r'){
            char str[UART_BUFFER_SIZE];
            UartCopyBufferToString(str);

            if(play_flag){
                if(UartBufferEndsWith("<end>\r")){
                    play_flag = 0;
                    str[strlen(str) - 5] = '\0';  // Remove "<end>"
                    UartSendString("<end>");
                }
                play_midi(str);
                UartClearBuffer();
                return;
            }

            int pitch_val, base_val, delta_val;
            if(sscanf(str, "pitch set pulse width us %d", &pitch_val) == 1) {
                if(MOTOR_NEG_90_DEG_US <= pitch_val && pitch_val <= MOTOR_POS_90_DEG_US){
                    PWMSetDutyCycle(pitch_val);
                    UartSendString("Set pitch motor pulse width to ");
                    UartSendInt(pitch_val);
                    UartSendString(" us\n\r");
                } else {
                    UartSendString("Failed to set pitch motor pulse width, must be between ");
                    UartSendInt(MOTOR_NEG_90_DEG_US);
                    UartSendString(" and ");
                    UartSendInt(MOTOR_POS_90_DEG_US);
                    UartSendString(" us\n\r");
                }
                UartSendString("<end>");
            } else if (sscanf(str, "pitch set degree %d", &pitch_val) == 1) {
                if(-90 <= pitch_val && pitch_val <= 90){
                    MotorRotateDegree(pitch_val);
                    UartSendString("Set pitch motor degree to ");
                    UartSendInt(pitch_val);
                    UartSendString(" degree\n\r");
                } else {
                    UartSendString("Failed to set pitch motor degree, must be between -90 and 90\n\r");
                }
                UartSendString("<end>");
            } else if(sscanf(str, "pick set base degree %d", &base_val) == 1) {
                if(-90 <= base_val && base_val <= 90){
                    base_degree = base_val;
                    Motor2RotateDegree(base_degree);
                    UartSendString("Set pick motor base degree to ");
                    UartSendInt(base_degree);
                    UartSendString(" degree\n\r");
                } else {
                    UartSendString("Failed to set pick motor base degree, must be between -90 and 90\n\r");
                }
                UartSendString("<end>");
            } else if(sscanf(str, "pick set degree delta %d", &delta_val) == 1) {
                if(-90 <= delta_val && delta_val <= 90){
                    degree_delta = delta_val;
                    UartSendString("Set pick motor degree delta to ");
                    UartSendInt(degree_delta);
                    UartSendString(" degree\n\r");
                } else {
                    UartSendString("Failed to set pick motor degree delta, must be between -90 and 90\n\r");
                }
                UartSendString("<end>");
            } else if(strcmp(str, "pick\r") == 0) {
                rotate_pick_motor();
                UartSendString("Rotate pick motor\n\r");
                UartSendString("<end>");
            } else if(strncmp(str, "play", 4) == 0) {
                UartSendString("Playing...\n\r");
                char play_str[UART_BUFFER_SIZE];
                
                strcpy(play_str, str + 5);  // Copy everything after "play "
                
                if(UartBufferEndsWith("<end>\r")) {
                    play_str[strlen(play_str) - 5] = '\0';  // Remove "<end>" from play_str
                    play_midi(play_str);
                    UartSendString("<end>");
                } else {
                    play_flag = 1;  // Set flag for continued input
                    play_midi(play_str);
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