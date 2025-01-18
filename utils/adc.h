#ifndef ADC_H
#define ADC_H

#include <xc.h>
#include "settings.h"
#include "config.h"

#if _XTAL_FREQ == 8000000
// Tad = 1/(8Mhz/8) = 1us
// 8Mhz < 11.43Mhz
#define ADCS_VALUE 0b001 
#elif _XTAL_FREQ == 4000000
// Tad = 1/(4Mhz/4) = 1us
// 4Mhz < 5.71Mhz
#define ADCS_VALUE 0b100 
#else
// Tad = 1/((2Mhz,1Mhz,500kHz,...)/2) >= 1us
// < 2.86Mhz
#define ADCS_VALUE 0b000 
#endif

#define ADC_IF PIR1bits.ADIF
#define ADC_GO ADCON0bits.GO

#define AdcGetResultHigh() (ADRESH)
#define AdcGetResultLow() (ADRESL)
#define AdcStartConversion() ADCON0bits.GO = 1
#define AdcIntDone() PIR1bits.ADIF = 0

void AdcInitialize(IntPriority int_priority);
void AdcEnableInterrupt(IntPriority);

#endif