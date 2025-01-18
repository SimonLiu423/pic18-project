#include "interrupt_manager.h"

void InterruptInitialize(void){
    RCONbits.IPEN = 1;      //enable Interrupt Priority mode
    INTCONbits.GIEH = 1;    //enable high priority interrupt
    INTCONbits.GIEL = 1;     //disable low priority interrupt
}

void ButtonInitialize(int interrupt){
    ADCON1 = 0x0F; // disable analog input
    TRISBbits.TRISB0 = 1; // INT0 input
    if (interrupt){ // enable interrupt
        INTCONbits.INT0IF = 0; // clear interrupt flag
        INTCONbits.INT0IE = 1; // enable interrupt
    }
}