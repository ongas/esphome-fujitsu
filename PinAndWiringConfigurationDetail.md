# LIN Transceiver (TLE8457-C) PIN Configuration is:
1. Vs
2. EN
3. GND
4. BUS
5. RxD
6. TxD
7. NRST
8. Vcc

# FireBeetle Left side Pin Configuration is:
- IO3/RXD：D0(Arduino), the RX of Serial, connects to IO3 of ESP32
- IO1/TXD：D1(Arduino), the TX of Serial, connects to IO1 of ESP32
- IO25/D2：D2 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO25 of ESP32.
- IO26/D3：D3 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO26 of ESP32.
- IO27/D4：D4 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO27 of ESP32.
- IO9/ D5: D5 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO9 of ESP32.
- IO10/D6：D6 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO9 of ESP32.
- IO13/D7：D7 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO13 of ESP32.
- IO5/D8：D8 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO5 of ESP32.
- IO2/D9：D9 (Arduino) of GPIO digital input and output port and PWM output pin, connect to IO2 of ESP32.
- CLK：Clock pin to IO6 of ESP32
- SD0：SD0 port to IO7 of ESP32
- SD1：SD1 port to IO7 of ESP32
- CMD：CMD port to IO11 of ESP32
- GND：Power line ground
- AREF：input voltage for reference, here connect to NC.
- 3V3：3.3V Vo, can provide 600mA current out at most.
- VCC：The Vi/Vo is 5V charged by USB and 3.7V charged by lithium battery.

FireBeetle Right side Pin Configuration is:
- IO36/A0：A0 (Arduino), analog input, connect to IO36 of ESP32.
- IO39/A1：A1 (Arduino), analog input, connect to IO39 of ESP32.
- IO34/A2：A2 (Arduino), analog input, connect to IO34 of ESP32.
- IO35/A3：A3 (Arduino), analog input, connect to IO35 of ESP32.
- IO15/A4：A4 (Arduino), analog input, connect to IO15 of ESP32.
- NC：Not connected
- IO0：Digital interface, connect to IO0 of ESP32.
- SCK：Clock pin of SPI, connect to IO18 of ESP32.
- MOSI：SPI’s MOSI data-wire, connect to IO23 of ESP32.
- MISO：SPI’s MOSO data-wire, connect to IO19 of ESP32.
- SDA：I2C’s data-wire, connect to IO21 of ESP32.
- SCL：Clock pin of I2C, connect to IO22 of ESP32.
- BCLK：Clock pin of I2S, connect to IO14 of ESP32.
- MCLK：Clock pin of I2S, connect to IO12 of ESP32.
- DO：DO data-wire of I2S, connect to IO4 of ESP32.
- DI：DI data-wire of I2S, connect to IO16 of ESP32.
- LRCK：LRCK data-wire of I2S, connect to IO17 of ESP32.
- RST：Low level reset port.
For master control board of the FireBeetle, we have reserved external DC ports for more
complex projects, supporting other peripheral chargers such as wireless charging module and
solar charging module.
Caution: + should be connected to the positive pole of the external charger;
 - should be connected to the negative pole of the external charger.
 The input voltage is from 4.7V to 6V

# AR-WDD1E terminal configuration is:
- RED +12v -> Buck Converter +12v In
- BLK GND -> Buck Converter GND
- WHT BUS -> LIN Pin4


# LIN Pins:
- Pin1 Power Supply +12v AND 1 leg of 100nf cap AND positive electrolytic #1
- Pin3 Power Supply -12v AND negative electrolytic #1 AND other leg of 100nf cap
- Pin4 Bus AND 10kohm resistor -> +12v

# Buck Converter:
- AC +12v -> +12v In
- AC GND -> GND In

# FireBeetle (DFR0478):
- GPIO27/D4 → LIN Pin6 (TxD)
- GPIO13/D7 → LIN Pin5 (RxD)
- VCC -> Buck +5v Out
- GND -> Power Supply GND

# TLE8457C LIN Transceiver Direct Wiring:
- Pin1 (VS): +12V from AC unit (via capacitor network)
- Pin2 (EN): Connected to Pin8 (VCC) - enables normal operation mode
- Pin3 (GND): Ground
- Pin4 (BUS): LIN bus to AC unit
- Pin5 (RxD): 3.3V logic from Firebeetle GPIO13
- Pin6 (TxD): 3.3V logic to Firebeetle GPIO27
- Pin7 (NRST): Leave unconnected (has internal pull-up to VCC, open-drain output for reset signaling)
- Pin8 (VCC): Regulated 5V or 3.3V output (internal LDO - see variant marking)
  - Requires 1µF ceramic decoupling capacitor to GND
  - Supplies Firebeetle and peripherals up to 70mA
