#include "ccp.h"
#include "timer.h"
#include "config.h"

double PWMDutyCycle = 0;
double PWM2DutyCycle = 0;
int MotorDegree = 0;
int Motor2Degree = 0;

void PWMInitialize(double period_ms){
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC1 = 0;
    CCP1CONbits.CCP1M = 0b1100;
    CCP2CONbits.CCP2M = 0b1100;
    if(_XTAL_FREQ <= 1000000){
        Timer2Initialize(INTERRUPT_NONE, 4, 16, 0);
    } else {
        Timer2Initialize(INTERRUPT_NONE, 16, 16, 0);
    }
    PWMSetPeriod(period_ms);
}

void PWMSetPeriod(double period_ms){
    // Set up PR2, CCP to decide PWM period and Duty Cycle
    /** 
     * PWM period
     * = (PR2 + 1) * 4 * Tosc * (TMR2 prescaler)
     */
    int prescaler = Timer2GetPrescaler();
    PR2 = (period_ms * (_XTAL_FREQ / 1000)) / (4 * prescaler) - 1;
}

void PWMSetDutyCycle(double duty_cycle_us){
    /**
     * Duty cycle
     * = (CCPR1L:CCP1CON<5:4>) * Tosc * (TMR2 prescaler)
     * = (0x0b*4 + 0b01) * 8µs * 4
     * = 0.00144s ~= 1450µs
     */
    PWMDutyCycle = duty_cycle_us;
    int prescaler = Timer2GetPrescaler();
    long long val = (PWMDutyCycle * _XTAL_FREQ / 1000000) / prescaler;
    CCPR1L = (val >> 2) & 0xFF;
    CCP1CONbits.DC1B = (val & 0x03);
}

double PWMGetDutyCycle(){
    return PWMDutyCycle;
}

void MotorRotateWithDelay(double target_duty_cycle){
    while(PWMDutyCycle != target_duty_cycle){
        double next_duty_cycle;
        if(PWMDutyCycle < target_duty_cycle){
            next_duty_cycle = PWMDutyCycle + PWM_MOTOR_STRIDE;
            if(next_duty_cycle > target_duty_cycle) next_duty_cycle = target_duty_cycle;
            PWMSetDutyCycle(next_duty_cycle);
        } else if(PWMDutyCycle > target_duty_cycle){
            next_duty_cycle = PWMDutyCycle - PWM_MOTOR_STRIDE;
            if(next_duty_cycle < target_duty_cycle) next_duty_cycle = target_duty_cycle;
            PWMSetDutyCycle(next_duty_cycle);
        }
        __delay_ms(2);
    }
}

void MotorRotateDegree(int degree){
    MotorDegree = degree;
    double duty_cycle = (MOTOR_POS_90_DEG_US - MOTOR_NEG_90_DEG_US) * (double)(degree + 90) / 180 + MOTOR_NEG_90_DEG_US;
    PWMSetDutyCycle(duty_cycle);
}

void MotorRotateDegreeWithDelay(int degree){
    MotorDegree = degree;
    double duty_cycle = (MOTOR_POS_90_DEG_US - MOTOR_NEG_90_DEG_US) * (double)(degree + 90) / 180 + MOTOR_NEG_90_DEG_US;
    MotorRotateWithDelay(duty_cycle);
}

int MotorGetRotateDegree(){
    return MotorDegree;
}

void PWM2SetDutyCycle(double duty_cycle_us){
    PWM2DutyCycle = duty_cycle_us;
    int prescaler = Timer2GetPrescaler();
    long long val = (PWM2DutyCycle * _XTAL_FREQ / 1000000) / prescaler;
    CCPR2L = (val >> 2) & 0xFF;
    CCP2CONbits.DC2B = (val & 0x03);
}

double PWM2GetDutyCycle(){
    return PWM2DutyCycle;
}

void Motor2RotateWithDelay(double target_duty_cycle){
    while(PWM2DutyCycle != target_duty_cycle){
        double next_duty_cycle;
        if(PWM2DutyCycle < target_duty_cycle){
            next_duty_cycle = PWM2DutyCycle + PWM_MOTOR_STRIDE;
            if(next_duty_cycle > target_duty_cycle) next_duty_cycle = target_duty_cycle;
            PWM2SetDutyCycle(next_duty_cycle);
        } else if(PWM2DutyCycle > target_duty_cycle){
            next_duty_cycle = PWM2DutyCycle - PWM_MOTOR_STRIDE;
            if(next_duty_cycle < target_duty_cycle) next_duty_cycle = target_duty_cycle;
            PWM2SetDutyCycle(next_duty_cycle);
        }
        __delay_ms(2);
    }
}

void Motor2RotateDegree(int degree){
    Motor2Degree = degree;
    double duty_cycle = (MOTOR_POS_90_DEG_US - MOTOR_NEG_90_DEG_US) * (double)(degree + 90) / 180 + MOTOR_NEG_90_DEG_US;
    PWM2SetDutyCycle(duty_cycle);
}

void Motor2RotateDegreeWithDelay(int degree){
    Motor2Degree = degree;
    double duty_cycle = (MOTOR_POS_90_DEG_US - MOTOR_NEG_90_DEG_US) * (double)(degree + 90) / 180 + MOTOR_NEG_90_DEG_US;
    Motor2RotateWithDelay(duty_cycle);
}

int Motor2GetRotateDegree(){
    return Motor2Degree;
}