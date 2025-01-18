#include "adc.h"
#include "settings.h"
#include "config.h"

void AdcInitialize(IntPriority int_priority){
    TRISAbits.RA0 = 1; // AN0 as input

    ADCON1bits.VCFG0 = 0;
    ADCON1bits.VCFG1 = 0;
    ADCON1bits.PCFG = 0b1110; // AN0 as analog input, rest as digital

    ADCON2bits.ADFM = ADC_JUSTIFICATION;    // left justified 

    ADCON2bits.ADCS = ADCS_VALUE;
    ADCON2bits.ACQT = 0b010;  // 4Tad >= 4us

    if(int_priority != INTERRUPT_NONE){
        AdcEnableInterrupt(int_priority);
    }

    // use AN0 as analog input
    ADCON0bits.CHS = 0; // select AN0 as input channel
    ADCON0bits.ADON = 1; // enable ADC
    AdcStartConversion();
}

void AdcEnableInterrupt(IntPriority priority){
    PIE1bits.ADIE = 1;
    PIR1bits.ADIF = 0;
    IPR1bits.ADIP = priority;
}
