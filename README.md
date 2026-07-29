# nano64DeepSleep Library

Centralized Deep Sleep, Watchdog Timer (WDT), and Wake Interrupt Management Library for ATmega644PA LoRaFarmNet field nodes.

## Features

- **Watchdog Timer (WDT) Sleep Cycles**: Decomposes total sleep duration into 8s, 4s, 2s, 1s, and 16ms watchdog sleep intervals.
- **Power Down & Light Sleep Modes**: Supports both `POWER_DOWN` deep sleep (`sleep()`) and `IDLE` light sleep (`lightSleep()`).
- **Wake Interrupt Management**: Configurable external interrupts (`INT0`, `INT1`, `INT2`) with optional internal pull-up control (`usePullup`) and software debouncing (`cooldownMs`).
- **EIFR Interrupt Flag Clearing**: Prevents immediate false wake-up triggers during interrupt setup.
- **Packaging Reed Switch Support**: Software debouncing for magnetic reed switch toggles (`recordPackagingToggle`).

## Usage

In `platformio.ini`:
```ini
lib_deps =
    symlink://../../Libraries/nano64DeepSleep
```

In `main.cpp`:
```cpp
#include <nano64DeepSleep.h>

n64DS DS;

void setup() {
    DS.enableWakeExternal(2, 50, true); // Pin D2, 50ms cooldown, use internal pullup
    DS.enableWakeTimer(0, 15, 0);       // Sleep for 15 minutes
}

void loop() {
    DS.sleep();
    if (wakeWDT) {
        // Woke from timer
    } else if (wakeExternal != 0) {
        // Woke from external interrupt
    }
}
```
