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
#define BUFFER_SIZE 64

typedef struct {
    unsigned int pwm_values[BUFFER_SIZE];
    unsigned int delays[BUFFER_SIZE];
    unsigned int count;
    unsigned int current_idx;
} NoteBuffer;

NoteBuffer buffer1 = {0};

__bit is_playing = 0;
__bit pick_state = 0;
int degree_delta = 0;
int base_degree = 0;
unsigned int pending_notes = 0;

void reset(){
    buffer1.count = 0;
    buffer1.current_idx = 0;

    is_playing = 0;
    degree_delta = 20;
    base_degree = 0;
    pending_notes = 0;
}

void SystemInitialize(void){
    reset();
    IntConfig int_config = {
        .button = INTERRUPT_HIGH,
        .adc = INTERRUPT_LOW,
        .timer1 = INTERRUPT_LOW,
        .timer2 = INTERRUPT_NONE,
        .uart_tx = INTERRUPT_NONE,
        .uart_rx = INTERRUPT_LOW,
    };
    ComponentConfig component_config = {
        .prescaler1 = 8,
        .prescaler2 = 16,
        .postscaler2 = 16,
        .timer_period_ms = 1000,
        .pwm_period_ms = MOTOR_PERIOD_MS,
    };

    OscillatorInitialize();
    ComponentInitialize(COMPONENT_LED | COMPONENT_UART | COMPONENT_PWM | COMPONENT_BUTTON | COMPONENT_TIMER1,
                        &int_config, component_config);
    PWMSetDutyCycle(1120);
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

void delay(unsigned int ms){
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


void play_midi(){
    for(int i = 0; i < buffer1.count; i++){
        UartSendString("Playing note: ");
        UartSendInt(buffer1.pwm_values[i]);
        UartSendString(", delay: ");
        UartSendInt(buffer1.delays[i]);
        UartSendString("\n\r");
        UartSendString("<end>");

        PWMSetDutyCycle(900);
        __delay_ms(75);
        PWMSetDutyCycle(buffer1.pwm_values[i]);
        __delay_ms(5);
        rotate_pick_motor();
        if(buffer1.delays[i] >= 80) delay(buffer1.delays[i] - 80);
    }
    buffer1.count = 0;
    buffer1.current_idx = 0;

}

void parse_to_buffer(char *str){
    char *token = strtok(str, " ");
    while(token != NULL && buffer1.count < BUFFER_SIZE && pending_notes > 0){
        char tmp[UART_BUFFER_SIZE];
        strcpy(tmp, token);

        int pwm_val, delay_val;
        if(!(sscanf(tmp, "%d,%d", &pwm_val, &delay_val) == 2)){
            return;
        }
        buffer1.pwm_values[buffer1.count] = pwm_val;
        buffer1.delays[buffer1.count] = delay_val;
        buffer1.count++;
        pending_notes--;

        token = strtok(NULL, " ");
    }
}

void main(void) {
    SystemInitialize();
    while(1){
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

    if(Timer1IF){
        Timer1IntDone();
    }
    if(RCIF){
        UartReceiveChar();
        char ch = UartGetChar();

        // enter received
        if(ch == '\r'){
            char str[UART_BUFFER_SIZE];
            UartCopyBufferToString(str);

            int pitch_val, base_val, delta_val;
            if(strcmp(str, "reset\r") == 0){
                reset();
                UartSendString("<end>");
            } else if(sscanf(str, "pitch set pulse width us %d", &pitch_val) == 1) {
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
            // }else if (strncmp(str, "play end", 8) == 0) {
            //     play_flag = 0;
            //     UartSendString("<end>");
            } else if(strncmp(str, "play", 4) == 0) {
                char play_str[UART_BUFFER_SIZE];
                strcpy(play_str, str + 5);
                if(strcmp(play_str, "start\r") == 0){
                    play_midi();
                    UartSendString("<done><end>");
                } else if(pending_notes == 0){
                    pending_notes = atoi(play_str);
                    UartSendString("<ready><end>");
                } else {
                    parse_to_buffer(play_str);
                    UartSendString("<end>");
                }
            }

            UartClearBuffer();
        }
    }
    if(ADC_IF){
        AdcIntDone();
    }
}