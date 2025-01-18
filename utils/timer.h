#ifndef TIMER_H
#define TIMER_H

#include "settings.h"
#include "config.h"

#define Timer1IF (PIR1bits.TMR1IF && PIE1bits.TMR1IE)
#define Timer1IntDone() PIR1bits.TMR1IF = 0

#define Timer2IF (PIR1bits.TMR2IF && PIE1bits.TMR2IE)
#define Timer2IntDone() PIR1bits.TMR2IF = 0

void Timer1Initialize(IntPriority priority, int prescaler);
void Timer1StartInterrupt(double period_ms);
void Timer1StopInterrupt(void);
void Timer1SetPeriod(double period_ms);

void Timer2Initialize(IntPriority priority, int prescaler, int postscaler, double period_ms);
void Timer2SetPeriod(double period_ms);

#endif