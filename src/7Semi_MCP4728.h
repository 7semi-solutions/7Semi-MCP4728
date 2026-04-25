/***************************************************************************************************
 7semi_MCP4728.h - Header file for the MCP4728 4-channel 12-bit DAC with EEPROM
 Written for the 7semi hardware platform

 This library provides high-level and low-level functions to control the MCP4728 DAC,
 including setting DAC values, gain, voltage reference, power-down modes,
 and EEPROM storage. Both volatile (RAM) and non-volatile (EEPROM) configurations
 are supported.

 Features:
   - Fast write to individual channels
   - Write all channels at once
   - Sequential writes starting from any channel
   - EEPROM write and read
   - General call update for simultaneous output changes
   - Readback of input registers and EEPROM values

 Author : 7semi
 License: MIT
*****************************************************************************************************/

#ifndef _7SEMI_MCP4728_H_
#define _7SEMI_MCP4728_H_

#include <Arduino.h>
#include <Wire.h>

// Default I2C address for MCP4728
#define MCP4728_I2C_ADDR 0x60

#define MCP4728_CMD_FAST_WRITE           0x00  // Fast write (implicit, no command byte)

#define MCP4728_CMD_MULTI_WRITE          0x40  // Write DAC registers (no EEPROM)
#define MCP4728_CMD_SEQ_WRITE            0x50  // Sequential write (DAC + EEPROM)
#define MCP4728_CMD_SINGLE_WRITE         0x58  // Single channel write (DAC + EEPROM)

#define MCP4728_CMD_VREF                 0x80  // Set VREF for all channels
#define MCP4728_CMD_POWERDOWN            0xA0  // Power-down configuration
#define MCP4728_CMD_GAIN                 0xC0  // Set gain for all channels

#define MCP4728_CMD_GENERAL_CALL_RESET   0x06  // General call reset
#define MCP4728_CMD_GENERAL_CALL_WAKEUP  0x09  // General call wake-up
#define MCP4728_CMD_GENERAL_CALL_UPDATE  0x08  // LDAC update all outputs

enum Channel
{
  DAC_A = 0,
  DAC_B = 1,
  DAC_C = 2,
  DAC_D = 3
};

enum Vref
{
  VREF_EXTERNAL = 0, // External reference (Vdd)
  VREF_2_048V = 1  // Internal 2.048V reference
};

enum Gain
{
  GAIN_1X = 0, // Vout = Vref * (value / 4096)
  GAIN_2X = 1  // Vout = 2 * Vref * (value / 4096)
};

enum PowerDown
{
  PWR_NORMAL = 0,  // Normal operation
  PWR_1KOHM = 1,   // Power-down with 1 kΩ resistor to GND
  PWR_100KOHM = 2, // Power-down with 100 kΩ resistor to GND
  PWR_500KOHM = 3  // Power-down with 500 kΩ resistor to GND
};

enum MCP4728_I2CStatus : uint8_t
{
  OK = 0,
  DATA_TOO_LONG = 1,
  NACK_ON_ADDRESS = 2,
  NACK_ON_DATA = 3,
  OTHER_ERROR = 4,

  NULL_PTR = 10,
  TIMEOUT = 11,
  NOT_INITIALIZED = 12,
  SHORT_READ = 13
};

class MCP4728_7Semi
{
public:
  MCP4728_7Semi();

  /**
   * Initialize MCP4728 and verify communication
   *
   * - Stores I2C address and Wire instance
   * - Starts I2C bus and sets clock speed
   * - Checks device presence using I2C ACK
   *
   * Returns:
   * - true  → Device detected and responding
   * - false → Device not found or communication failed
   *
   * Notes:
   * - Must be called before any DAC operation
   * - Typical I2C speeds:
   *   - 100000  → standard mode
   *   - 400000  → fast mode (recommended)
   */
  bool begin(uint8_t i2cAddress = 0x60, TwoWire &i2cPort = Wire, uint32_t i2cSpeed = 400000);

  /**
   * Update I2C clock speed
   *
   * - Changes Wire bus frequency
   * - Useful when switching between devices with different speed limits
   *
   * Notes:
   * - Safe to call anytime after begin()
   * - Ignored if I2C is not initialized
   */
  void setI2CSpeed(uint32_t clock);

  /**
   * Write one channel (DAC + EEPROM)
   *
   * - Uses Single Write command (0x58)
   * - Updates DAC output and stores value in EEPROM
   *
   * Notes:
   * - Includes VREF / GAIN / PD in MSB
   * - EEPROM write takes ~25–30 ms
   * - Do NOT use in fast loop (limited write cycles)
   */
  bool setChannelEEPROM(uint8_t channel,
                        uint16_t value,
                        uint8_t vref,
                        uint8_t gain,
                        uint8_t pd);

  /**
   * analogWrite: Set DAC value for a single channel
   *
   * - Writes 12-bit value to selected channel
   * - Updates DAC register only (no EEPROM)
   *
   * Notes:
   * - Channel: 0–3
   * - Value: 0–4095
   * - Fast and safe for runtime updates
   */
  bool analogWrite(uint8_t channel, uint16_t value);

  /**
   * fastWrite: Write all 4 DAC channels quickly (no EEPROM)
   *
   * - Uses fast write command (no command byte per channel)
   * - Updates DAC output registers only
   * - No EEPROM write → very fast operation
   *
   * Notes:
   * - Does not modify VREF / GAIN / PD (default = 0 used)
   * - Output updates immediately
   * - Best for real-time waveform / frequent updates
   */
  bool fastWrite(uint16_t values[4]);

  /**
   * multiWrite: Write all 4 channels (DAC or EEPROM)
   *
   * - writeEEPROM = false → multi-write (0x40), fast DAC update
   * - writeEEPROM = true  → sequential write (0x50), saves to EEPROM
   *
   * Notes:
   * - EEPROM write takes ~25–30ms (blocking delay used)
   * - VREF / GAIN / PD are currently set to 0 (default)
   * - DAC values are masked to 12-bit
   */
  bool multiWrite(uint16_t values[4], bool writeEEPROM = false);

  /**
   * sequentialWrite: Write multiple channels starting from index
   *
   * - Writes DAC registers and EEPROM
   * - Starts from startChannel and writes 'len' channels
   *
   * Notes:
   * - Uses command 0x50 (sequential write)
   * - Includes VREF / GAIN / PD bits (currently default = 0)
   * - Does not include delay → caller should handle if needed
   */
  bool sequentialWrite(uint8_t startChannel, uint16_t values[], uint8_t len);

  /**
   * updateAllOutputs: Apply pending DAC updates
   *
   * - Uses General Call command (0x08)
   * - Forces all channels to update simultaneously
   *
   * Notes:
   * - Useful when LDAC pin is controlled externally
   * - Safe to call anytime
   */
  bool updateAllOutputs();

  /**
   * readDAC: Read current DAC output values
   *
   * - Reads 24 bytes from device
   * - Extracts DAC register values (not EEPROM)
   *
   * Notes:
   * - Returns 12-bit values per channel
   * - Ignores config bits (VREF / GAIN / PD)
   */
  bool readDAC(uint16_t values[4]);

  /**
   * readAll: Read DAC + EEPROM values
   *
   * - Reads full device memory (24 bytes)
   * - Extracts both DAC register and EEPROM values
   *
   * Outputs:
   * - ADCVals  → current DAC outputs
   * - eepromVals → stored EEPROM values
   *
   * Notes:
   * - Useful for debugging and verification
   * - Values are masked to 12-bit
   */
  bool readAll(uint16_t ADCVals[4], uint16_t eepromVals[4]);

  /**
   * setVref: Set voltage reference for all channels
   *
   * - Uses VREF command (100xxxxx)
   * - Updates DAC input registers only (no EEPROM)
   *
   * Notes:
   * - chA–chD: 0 = VDD, 1 = Internal 2.048V
   * - Temporary setting → will be overwritten by DAC write commands
   */
  bool setVref(uint8_t chA, uint8_t chB, uint8_t chC, uint8_t chD);

  /**
   * setGain: Set gain for all channels
   *
   * - Uses Gain command (110xxxxx)
   * - Updates DAC input registers immediately
   *
   * Notes:
   * - 0 = Gain x1
   * - 1 = Gain x2
   * - Does not affect EEPROM
   */
  bool setGain(uint8_t gainCH_A,
               uint8_t gainCH_B,
               uint8_t gainCH_C,
               uint8_t gainCH_D);

  /**
   * powerDown: Configure power-down mode for all channels
   *
   * - Uses Power-Down command (101xxxxx)
   * - Controls output state (normal / resistor to GND)
   *
   * Notes:
   * - pd values:
   *   0 → normal mode
   *   1 → 1kΩ to GND
   *   2 → 100kΩ to GND
   *   3 → 500kΩ to GND
   */
  bool powerDown(uint8_t pdA,
                 uint8_t pdB,
                 uint8_t pdC,
                 uint8_t pdD);

  /**
   * setChannel: Write DAC value to single channel
   *
   * - Updates DAC register only (no EEPROM)
   *
   * Notes:
   * - Channel: 0–3
   * - Value is masked to 12-bit
   * - Uses multi-write style command
   */
  bool setChannel(uint8_t channel, uint16_t value);

private:
  uint8_t i2c_address;
  TwoWire *i2c;

  uint8_t status = 0;

  bool writeCommand(uint8_t *data, uint8_t len);

  bool writeCommand(uint8_t commandByte);

  bool sendCommand(uint8_t cmd);

  bool writeReg(uint8_t reg, uint16_t value);

  bool readData(uint8_t *buf, size_t len);

  bool readReg(uint8_t reg, uint16_t &value);

  bool burstRead(uint8_t reg, uint8_t *buf, size_t len);
};

#endif
