#ifndef LED_H
#define LED_H
#include <xc.h>
#include <pic18f4520.h>

#define LedClear() (LATA = 0x00 | (LATA & 0x01))
#define LedSet(value) (LATA = ((value) << 1) | (LATA & 0x01))
#define LedValue() ((LATA >> 1) & 0x0F)
#define Led0Toggle() (LATAbits.LATA1 ^= 1)
#define Led1Toggle() (LATAbits.LATA2 ^= 1)
#define Led2Toggle() (LATAbits.LATA3 ^= 1)
#define Led3Toggle() (LATAbits.LATA4 ^= 1)

void LedInitialize(void);

#endif