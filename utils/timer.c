#include "timer.h"
#include "settings.h"

int Timer1Prescaler = 8;
int Timer2Prescaler = 16;
int Timer2Postscaler = 16;

void Timer1Initialize(IntPriority priority, int prescaler){
    T1CONbits.RD16 = 1;
    Timer1Prescaler = prescaler;
    switch(prescaler){
        case 8:
            T1CONbits.T1CKPS = 0b11;
            break;
        case 4:
            T1CONbits.T1CKPS = 0b10;
            break;
        case 2:
            T1CONbits.T1CKPS = 0b01;
            break;
        case 1:
            T1CONbits.T1CKPS = 0b00;
            break;
        default:
            T1CONbits.T1CKPS = 0b11;
            break;
    }
    if(priority != INTERRUPT_NONE){
        IPR1bits.TMR1IP = priority;
    }

    T1CONbits.TMR1ON = 1;
}

void Timer1SetPeriod(double period_ms){
    TMR1 = (65535 - (period_ms * (_XTAL_FREQ / 1000)) / (4 * Timer1Prescaler)) + 1;
}

void Timer1StartInterrupt(double period_ms){
    Timer1SetPeriod(period_ms);
    PIE1bits.TMR1IE = 1;
}

void Timer1StopInterrupt(void){
    PIE1bits.TMR1IE = 0;
}

void Timer2Initialize(IntPriority priority, int prescaler, int postscaler, double period_ms){
    Timer2Prescaler = prescaler;
    Timer2Postscaler = postscaler;
    if(Timer2Prescaler == 4){
        T2CONbits.T2CKPS = 0b01; // prescaler = 4
    } else {
        T2CONbits.T2CKPS = 0b10; // prescaler = 16
    }

    T2CONbits.T2OUTPS = Timer2Postscaler - 1;

    if(priority != INTERRUPT_NONE){
        PIE1bits.TMR2IE = 1;
        IPR1bits.TMR2IP = priority;
    }

    PR2 = (period_ms * (_XTAL_FREQ / 1000)) / (4 * Timer2Prescaler * Timer2Postscaler) - 1;
    T2CONbits.TMR2ON = 1;
}

void Timer2SetPeriod(double period_ms){
    PR2 = (period_ms * (_XTAL_FREQ / 1000)) / (4 * Timer2Prescaler * Timer2Postscaler) - 1;
}

int Timer2GetPrescaler(void){
    return Timer2Prescaler;
}
