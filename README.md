# nano64DeepSleep Library

Centralized Deep Sleep, Watchdog Timer (WDT), and Wake Interrupt Management Library for ATmega644PA LoRaFarmNet field nodes.

## Features

- **Watchdog Timer (WDT) Sleep Cycles**: Decomposes total sleep duration into 8s, 4s, 2s, 1s, and 16ms watchdog sleep intervals.
- **Power Down & Light Sleep Modes**: Supports both `POWER_DOWN` deep sleep (`sleep()`) and `IDLE` light sleep (`lightSleep()`).
- **Wake Interrupt Management**: Configurable external interrupts (`INT0`, `INT1`, `INT2`) with optional internal pull-up control (`usePullup`) and software debouncing (`cooldownMs`).
- **EIFR Interrupt Flag Clearing**: Prevents immediate false wake-up triggers during interrupt setup.
- **Packaging Reed Switch Support**: Software debouncing for magnetic reed switch toggles (`recordPackagingToggle`).

## Installation

Add to `platformio.ini`:
```ini
lib_deps =
    https://github.com/toogooda/nano64DeepSleep.git
```
