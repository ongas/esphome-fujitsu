# Intro
This project uses Fujitsu's proprietary protocol to control a Fujitsu AC(heat pump) unit, interfacing with Home Assistant through ESPHome.

This project is entirely based on unreality [FujiHeatPump](https://github.com/unreality/FujiHeatPump) project.
Huge thanks to [unreality](https://github.com/unreality/) and [jaroslawprzybylowicz](https://github.com/jaroslawprzybylowicz/fuji-iot)!

# How to use:
## Hardware:

See [FujiHeatPump](https://github.com/unreality/FujiHeatPump)'s readme file, I use that exact circuit with a ESP32 development board.

Current wiring note:
- The LIN transceiver TX/RX are on the ESP32 Serial 2 line.
- Serial 1 is reserved for debugging.
- The 12V pull-up on pin 4 has been removed.
- A 10k resistor now connects pin 2 to pin 8.
- Pin 7 is wired to pin 2.

## ESPHome:


Copy all files under src into ESPHome config folder, next to the yaml file.
```text
- config_folder
|- fujitsu.yaml
|- FujiHeatPump.cpp
|- FujiHeatPump.h
|- FujitsuClimate.cpp
|- FujitsuClimate.h
```
See fujitsu.yaml for a sample config file.
Then run your esphome (tested with esphome version 2022.2.3) compile or upload command:
```bash
$ esphome compile fujitsu.yaml
```

## Configuration:

### Pins (Required)
```yaml
climate:
  - platform: fujitsu
    name: "Fujitsu AC"
    tx_pin: 1      # TX pin for serial communication
    rx_pin: 3      # RX pin for serial communication
```

### Debug Logging (Optional)
Enable debug logging to see incoming frames from the AC unit:
```yaml
climate:
  - platform: fujitsu
    name: "Fujitsu AC"
    tx_pin: 1
    rx_pin: 3
    debug: true    # Enable frame logging: onOff, temperature, mode, fan
```

When `debug: true`, frame data is logged. When `debug: false` (default), debug logging is disabled for normal operation.

## Default behaviour:

* The LIN transceiver (TLE8457C) is connected to Serial 2
* The controller is registered as secondary controller, see `void FujitsuClimate::setup()` in `FujitsuClimate.cpp`
* My AC works on 1 degree C step, 16-30 degree temperature range etc...
see `void climate::ClimateTraits FujitsuClimate::traits()` in `FujitsuClimate.cpp` for more details,
modify the function to suit your unit.

## SECONDARY controller registration (important):

The ESP32 registers as a SECONDARY controller (address 33) alongside an existing PRIMARY wired controller (address 32). Key things to know:

* **OTA updates** are seamless — the SECONDARY re-registers automatically after an OTA reboot without needing to power-cycle the AC unit.
* **TX timing is critical**: the SECONDARY response uses deferred TX with a 60ms delay + 50ms bus-idle check (`sendPendingFrame()`). Immediate TX (1ms after probe) does NOT work — the unit ignores responses that arrive too quickly after the probe.
* **Status display**: "Connected (Read-Write)" means the unit has accepted the SECONDARY registration (`cP=1` in incoming probes). If `cP` stays at 0, the unit is ignoring our responses and any changes from HA will revert.
* **Bus protocol**: 500 baud, 8E1, half-duplex via LIN transceiver. Each 8-byte frame takes 176ms to transmit. The bus cycle is ~850ms.

## Write retry mechanism:

When you change a setting in Home Assistant (temperature, mode, fan speed, etc.), the ESP holds the desired value in the UI for a 2-second grace period before checking whether the unit has accepted the change. If the unit hasn't confirmed the new value, it automatically retries up to 5 times (10 seconds total). This prevents the UI from "bouncing" back to the old value while the unit is processing the write.
