#include "nano64DeepSleep.h"

volatile bool wakeWDT = false;
volatile byte wakeExternal = 0;
volatile byte wakeInterruptPins[3] = {0, 0, 0};
volatile byte wakeReasonCode = 0;
volatile bool wakeExternalSawLow = false;
volatile uint8_t packagingToggleCount = 0;
volatile uint32_t packagingLastAcceptedToggleMs = 0;
const uint16_t packagingToggleDebounceMs = 75;

void clearPendingExternalInterrupt(int irq) {
  #if defined(EIFR)
    if (irq == 0) {
      #if defined(INTF0)
        EIFR |= (1 << INTF0);
      #endif
    } else if (irq == 1) {
      #if defined(INTF1)
        EIFR |= (1 << INTF1);
      #endif
    } else if (irq == 2) {
      #if defined(INTF2)
        EIFR |= (1 << INTF2);
      #endif
    }
  #endif
}

void recordPackagingToggle(byte pinState) {
  if (pinState != HIGH) {
    return;
  }

  unsigned long now = millis();
  if ((uint32_t)(now - packagingLastAcceptedToggleMs) < packagingToggleDebounceMs) {
    return;
  }

  packagingLastAcceptedToggleMs = now;
  if (packagingToggleCount < 255) {
    packagingToggleCount++;
  }
}

byte n64DS::_pinToInterruptIndex(byte inPin) {
  int irq = digitalPinToInterrupt(inPin);
  if (irq >= 0 && irq < 3) {
    return (byte)irq;
  }
  return 255;
}

bool n64DS::_shouldSuppressExternalWake() {
  if (wakeExternal == 0) {
    return false;
  }

  byte irqIndex = _pinToInterruptIndex(wakeExternal);
  if (irqIndex == 255) {
    return false;
  }

  uint32_t cooldown = _externalCooldownMs[irqIndex];
  if (cooldown == 0) {
    return false;
  }

  uint32_t now = millis();
  if (_hasAcceptedExternalWake[irqIndex]) {
    uint32_t elapsed = now - _lastAcceptedExternalMs[irqIndex];
    if (elapsed < cooldown) {
      wakeExternal = 0;
      wakeReasonCode = WAKE_NONE;
      wakeWDT = false;
      return true;
    }
  }

  _lastAcceptedExternalMs[irqIndex] = now;
  _hasAcceptedExternalWake[irqIndex] = true;
  return false;
}

void n64DS::disableWakeExternal(byte xint) {
  int irq = digitalPinToInterrupt(xint);
  if (irq == NOT_AN_INTERRUPT) {
    return;
  }

  detachInterrupt(irq);
  _wakePinsMask &= ~(1UL << xint);
  _pullupPinsMask &= ~(1UL << xint);
  if (irq >= 0 && irq < 3) {
    wakeInterruptPins[irq] = 0;
    _externalCooldownMs[irq] = 0;
    _lastAcceptedExternalMs[irq] = 0;
    _hasAcceptedExternalWake[irq] = false;
  }
}

void n64DS::disableWakeTimer() {
  _wdt = false;
}

void n64DS::enableWakeExternal(byte xint, unsigned long cooldownMs, bool usePullup) {
  int irq = digitalPinToInterrupt(xint);
  if (irq == NOT_AN_INTERRUPT) {
    return;
  }

  if (usePullup) {
    pinMode(xint, INPUT_PULLUP);
    _pullupPinsMask |= (1UL << xint);
  } else {
    pinMode(xint, INPUT);
    _pullupPinsMask &= ~(1UL << xint);
  }
  detachInterrupt(irq);

  _wakePinsMask |= (1UL << xint);
  wakeInterruptPins[irq] = xint;
  _externalCooldownMs[irq] = cooldownMs;
  _lastAcceptedExternalMs[irq] = 0;
  _hasAcceptedExternalWake[irq] = false;

  clearPendingExternalInterrupt(irq);

  if (irq == 0) {
    attachInterrupt(irq, xInterrupt0, CHANGE);
  } else if (irq == 1) {
    attachInterrupt(irq, xInterrupt1, CHANGE);
  } else if (irq == 2) {
    attachInterrupt(irq, xInterrupt2, CHANGE);
  } else {
    return;
  }
}

void n64DS::_setPins() {
  for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
    pinMode(i, INPUT);
  }

  for (int i = 0; i < NUM_DIGITAL_PINS; i++) {
    if ((_wakePinsMask & (1UL << i)) != 0) {
      if ((_pullupPinsMask & (1UL << i)) != 0) {
        pinMode(i, INPUT_PULLUP);
      } else {
        pinMode(i, INPUT);
      }
    }
  }
}

void n64DS::_disableADC() {
  ADCSRA &= ~(1 << 7);
}

void n64DS::_enableADC() {
  ADCSRA |= (1 << 7);
}

void n64DS::sleep() {
  uint32_t i;

  wakeReason = WAKE_NONE;
  wakeExternal = 0;
  wakeWDT = false;
  wakeExternalSawLow = false;

  while (true) {
    _setPins();
    _disableADC();

    if (_wdt) {
      if (_eights > 0) {
        _setWDTPrescaler(8);
        for (i = 0; i < _eights; i++) {
          _sleep();
          if (_shouldSuppressExternalWake()) {
            continue;
          }
          if (wakeExternal != 0) {
            wakeReason = (byte)wakeReasonCode;
            _enableADC();
            return;
          }
          wakeWDT = false;
        }
      }

      if (_fours > 0) {
        _setWDTPrescaler(4);
        for (i = 0; i < _fours; i++) {
          _sleep();
          if (_shouldSuppressExternalWake()) {
            continue;
          }
          if (wakeExternal != 0) {
            wakeReason = (byte)wakeReasonCode;
            _enableADC();
            return;
          }
          wakeWDT = false;
        }
      }

      if (_twos > 0) {
        _setWDTPrescaler(2);
        for (i = 0; i < _twos; i++) {
          _sleep();
          if (_shouldSuppressExternalWake()) {
            continue;
          }
          if (wakeExternal != 0) {
            wakeReason = (byte)wakeReasonCode;
            _enableADC();
            return;
          }
          wakeWDT = false;
        }
      }

      if (_ones > 0) {
        _setWDTPrescaler(1);
        for (i = 0; i < _ones; i++) {
          _sleep();
          if (_shouldSuppressExternalWake()) {
            continue;
          }
          if (wakeExternal != 0) {
            wakeReason = (byte)wakeReasonCode;
            _enableADC();
            return;
          }
          wakeWDT = false;
        }
      }

      wakeReason = (byte)wakeReasonCode;
      _enableADC();
      return;
    }

    _sleep();
    if (_shouldSuppressExternalWake()) {
      _enableADC();
      continue;
    }

    wakeReason = (byte)wakeReasonCode;
    _enableADC();
    return;
  }
}

void n64DS::lightSleep() {
  uint32_t i;
  _disableADC();

  if (_wdt) {
    _setWDTPrescaler(8);
    for (i = 0; i < _eights; i++) _lightSleep();

    _setWDTPrescaler(4);
    for (i = 0; i < _fours; i++) _lightSleep();

    _setWDTPrescaler(2);
    for (i = 0; i < _twos; i++) _lightSleep();

    _setWDTPrescaler(1);
    for (i = 0; i < _ones; i++) _lightSleep();

    _setWDTPrescaler(16);
    for (i = 0; i < _sixteenths; i++) _lightSleep();
  } else {
    _lightSleep();
  }
  _enableADC();
}

void n64DS::_setWDTPrescaler(byte inTime) {
  byte _prescaler;
  switch (inTime) {
    case 8: _prescaler = 33; break;  // 8s (0b00100001 = (1<<WDP3)|(1<<WDP0))
    case 4: _prescaler = 32; break;  // 4s (0b00100000 = (1<<WDP3))
    case 2: _prescaler = 7; break;   // 2s (0b00000111)
    case 1: _prescaler = 6; break;   // 1s (0b00000110)
    case 16: _prescaler = 0; break;  // 16ms
    default: return;
  }
  cli();
  wdt_reset();
  WDTCSR = (24);
  WDTCSR = (_prescaler);
  WDTCSR |= (1 << 6);
  sei();
}

void n64DS::_sleep() {
  SMCR |= (1 << 2);
  SMCR |= 1;

  MCUCR |= (3 << 5);
  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6);
  __asm__ __volatile__("sleep");
}

void n64DS::_lightSleep() {
  uint8_t oldTIMSK0 = TIMSK0;
  uint8_t oldUCSR0B = UCSR0B;
  TIMSK0 = 0;
  UCSR0B &= ~((1 << RXCIE0) | (1 << TXCIE0) | (1 << UDRIE0));
  SMCR = (SMCR & ~0b111) | 0b001;
  SMCR |= (1 << SE);
  sei();
  __asm__ __volatile__("sleep");
  SMCR &= ~(1 << SE);
  TIMSK0 = oldTIMSK0;
  UCSR0B = oldUCSR0B;
}

void n64DS::enableWakeTimer(unsigned long inHours, unsigned long inMins, unsigned long inSecs, unsigned long inMs) {
  _wdt = true;
  uint32_t totalSecs = inSecs + 60UL * (inMins + 60UL * inHours);
  _eights = totalSecs / 8;
  totalSecs -= _eights * 8;
  _fours = totalSecs / 4;
  totalSecs -= _fours * 4;
  _twos = totalSecs / 2;
  totalSecs -= _twos * 2;
  _ones = totalSecs;

  _sixteenths = (inMs + 15) / 16;
}

void xInterrupt0() {
  byte pin = wakeInterruptPins[0];
  if (pin == 0) {
    wakeExternal = 0;
    wakeReasonCode = WAKE_NONE;
    wakeWDT = false;
    return;
  }
  byte pinState = digitalRead(pin);
  wakeExternal = pin;
  wakeReasonCode = (pinState == HIGH) ? WAKE_INT0_H : WAKE_INT0_L;
  if (pinState == LOW) {
    wakeExternalSawLow = true;
  }
  wakeWDT = false;
}

void xInterrupt1() {
  byte pin = wakeInterruptPins[1];
  if (pin == 0) {
    wakeExternal = 0;
    wakeReasonCode = WAKE_NONE;
    wakeWDT = false;
    return;
  }
  byte pinState = digitalRead(pin);
  wakeExternal = pin;
  wakeReasonCode = (pinState == HIGH) ? WAKE_INT1_H : WAKE_INT1_L;
  if (pinState == LOW) {
    wakeExternalSawLow = true;
  }
  wakeWDT = false;
}

void xInterrupt2() {
  byte pin = wakeInterruptPins[2];
  if (pin == 0) {
    wakeExternal = 0;
    wakeReasonCode = WAKE_NONE;
    wakeWDT = false;
    return;
  }
  byte pinState = digitalRead(pin);
  wakeExternal = pin;
  wakeReasonCode = (pinState == HIGH) ? WAKE_INT2_H : WAKE_INT2_L;
  if (pinState == LOW) {
    wakeExternalSawLow = true;
  }
  wakeWDT = false;
}

ISR(WDT_vect) {
  wakeReasonCode = WAKE_WDT;
  wakeWDT = true;
  wakeExternal = 0;
}
