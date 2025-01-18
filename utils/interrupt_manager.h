#ifndef INTERRUPT_MANAGER_H
#define INTERRUPT_MANAGER_H

#define BUTTON_IF (INTCONbits.INT0IE && INTCONbits.INT0IF)
#define ButtonIntDone() INTCONbits.INT0IF = 0

#include <xc.h>

void InterruptInitialize(void);
void ButtonInitialize(int);

#endif
