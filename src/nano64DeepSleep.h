#pragma once

#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

extern volatile bool wakeWDT;
extern volatile byte wakeExternal;
extern volatile byte wakeInterruptPins[3];
extern volatile byte wakeReasonCode;
extern volatile bool wakeExternalSawLow;
extern volatile uint8_t packagingToggleCount;
extern volatile uint32_t packagingLastAcceptedToggleMs;
extern const uint16_t packagingToggleDebounceMs;

void clearPendingExternalInterrupt(int irq);
void recordPackagingToggle(byte pinState);

void xInterrupt0();
void xInterrupt1();
void xInterrupt2();

enum wakeReasonCode {
  WAKE_NONE = 0,
  WAKE_INT0_H = 1,
  WAKE_INT0_L = 2,
  WAKE_INT1_H = 3,
  WAKE_INT1_L = 4,
  WAKE_INT2_H = 5,
  WAKE_INT2_L = 6,
  WAKE_WDT = 7
};
typedef wakeReasonCode wakeReasonCodeEnum;

class n64DS {
  private:
    uint32_t _wakePinsMask = 0;
    uint32_t _pullupPinsMask = 0;
    uint32_t _sleepSeconds = 0;
    bool _wdt = false;
    bool _xint = false;
    uint32_t _eights = 0;
    uint32_t _fours = 0;
    uint32_t _twos = 0;
    uint32_t _ones = 0;
    uint32_t _sixteenths = 0;
    uint32_t _externalCooldownMs[3] = {0, 0, 0};
    uint32_t _lastAcceptedExternalMs[3] = {0, 0, 0};
    bool _hasAcceptedExternalWake[3] = {false, false, false};

    byte _pinToInterruptIndex(byte inPin);
    bool _shouldSuppressExternalWake();
    void _sleep();
    void _lightSleep();
    void _setWDTPrescaler(byte inTime);
    void _disableADC();
    void _enableADC();
    void _setPins();

  public:
    byte wakeReason = WAKE_NONE;
    void enableWakeTimer(unsigned long inHours, unsigned long inMins, unsigned long inSecs, unsigned long inMs = 0);
    void enableWakeExternal(byte xint, unsigned long cooldownMs = 50, bool usePullup = true);
    void disableWakeTimer();
    void disableWakeExternal(byte xint);
    void sleep();
    void lightSleep();
};
