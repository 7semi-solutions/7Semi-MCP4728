# 7Semi MCP4728 Arduino Library

Arduino driver for the Microchip MCP4728 4-channel 12-bit DAC.

The MCP4728 provides four independent high-resolution analog voltage outputs using a digital I²C interface, making it ideal for multi-channel signal generation, calibration, waveform synthesis, and analog control systems.

## Features

- 4-channel 12-bit DAC (0–4095 per channel)

- Fast write (simultaneous update of all channels)

- EEPROM write support (non-volatile storage)

- Individual channel configuration (VREF, Gain, Power-down)

- Sequential and multi-write modes

- Readback of DAC and EEPROM values

---

## Connections / Wiring

The MCP4728 communicates using **I²C interface**.

## I²C Connection

| MCP4728 Pin | MCU Pin   | Notes                          |
| ----------- | --------- | ------------------------------ |
| VCC         | 3.3V / 5V | Supply voltage                 |
| GND         | GND       | Common ground                  |
| SDA         | SDA       | I²C data                       |
| SCL         | SCL       | I²C clock                      |
| LDAC        | GND       | Auto update outputs (optional) |
| RDY         | NC        | Optional status pin            |

---

### I²C Notes

- Default I²C address: `0x60`

- Supported bus speeds:
  - 100 kHz
  - 400 kHz (recommended)

---

## Installation

- Arduino Library Manager

  1. Open Arduino IDE  
  2. Go to Library Manager  
  3. Search for **7Semi MCP4728**  
  4. Click Install  

- Manual Installation

  1. Download this repository as ZIP  
  2. Arduino IDE → Sketch → Include Library → Add .ZIP Library  

---

## Library Overview

### Initialize DAC

```cpp
dac.begin(0x60);
```

- Initializes MCP4728 with default I²C settings

### Set Single Channel (Fast)

```cpp
dac.analogWrite(DAC_A, 2048);
```

- Writes value to one channel (no EEPROM)
- Range: 0 → 4095

### Set All Channels (Fast Write)

```cpp
uint16_t values[4] = {1000, 2000, 3000, 4000};
dac.fastWrite(values);
```

- Updates all 4 channels simultaneously
- No EEPROM write → fast operation

### Multi Write (Optional EEPROM)

```cpp
dac.multiWrite(values, true);
```

- false → DAC only (fast)
- true → DAC + EEPROM (persistent)

### Sequential Write

```cpp
uint16_t values[4] = {1000, 2000, 3000, 4000};
dac.sequentialWrite(1, values, 3);
```

- Writes starting from channel index
- Supports partial updates

### Set Gain

```cpp
dac.setGain(GAIN_1X, GAIN_1X, GAIN_2X, GAIN_2X);
```

- 0 → Gain x1
- 1 → Gain x2

### Set VREF

```cpp
dac.setVref(VREF_EXTERNAL, VREF_EXTERNAL, VREF_2_048V, VREF_2_048V);
```

- 0 → VDD reference
- 1 → Internal 2.048V reference

### Power-down Modes

```cpp
dac.powerDown(PWR_NORMAL, PWR_1KOHM, PWR_100KOHM, PWR_500KOHM);
```

- 0 → Normal operation
- 1 → 1kΩ pull-down
- 2 → 100kΩ pull-down
- 3 → 500kΩ pull-down

### Read DAC Values

```cpp
uint16_t values[4];
dac.readDAC(values);
```

- Reads current DAC output values

### Read DAC + EEPROM

```cpp
uint16_t dacVals[4];
uint16_t eepromVals[4];
uint8_t status;

dac.readAll(dacVals, eepromVals, status);
```

**Notes:**

- DAC resolution = 12-bit (0–4095)
- Output voltage depends on:
  - VREF selection
  - Gain setting
- Avoid frequent EEPROM writes (limited write cycles)
- Use fastWrite() for real-time applications

---

## Typical Use Cases

- Multi-channel waveform generation
- Analog control systems
- Sensor calibration
- Signal synthesis (sine, ramp, etc.)
