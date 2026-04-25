
#include "7Semi_MCP4728.h"

MCP4728_7Semi::MCP4728_7Semi() {}

bool MCP4728_7Semi::begin(uint8_t addr, TwoWire &i2cPort, uint32_t i2cSpeed)
{
  i2c_address = addr;
  i2c = &i2cPort;

  i2c->begin();
  i2c->setClock(i2cSpeed);

  // Check device ACK on bus
  i2c->beginTransmission(i2c_address);
  if (i2c->endTransmission() != 0)
    return false;

  return true;
}

void MCP4728_7Semi::setI2CSpeed(uint32_t clock)
{
  if (i2c)
    i2c->setClock(clock);
}

bool MCP4728_7Semi::setChannelEEPROM(uint8_t channel,
                                     uint16_t value,
                                     uint8_t vref,
                                     uint8_t gain,
                                     uint8_t pd)
{
  if (!i2c || channel > 3 || value > 0x0FFF)
    return false;

  if (vref > 1 || gain > 1 || pd > 3)
    return false;

  uint8_t data[3];

  // Build MSB with config + upper DAC bits
  uint8_t msb =
      ((vref & 0x01) << 7) |
      ((pd & 0x03) << 5) |
      ((gain & 0x01) << 4) |
      ((value >> 8) & 0x0F);

  // Command byte (Single Write + channel select)
  data[0] = MCP4728_CMD_SINGLE_WRITE | (channel << 1);
  data[1] = msb;
  data[2] = value & 0xFF;

  if (!writeCommand(data, 3))
    return false;

  // EEPROM write delay
  delay(30);

  return true;
}

bool MCP4728_7Semi::analogWrite(uint8_t channel, uint16_t value)
{
  if (!i2c || channel > 3 || value > 0x0FFF)
    return false;

  uint8_t data[3];

  data[0] = MCP4728_CMD_MULTI_WRITE | (channel << 1); // DAC only (fast)
  data[1] = (value >> 8) & 0x0F;
  data[2] = value & 0xFF;

  return writeCommand(data, 3);
}

bool MCP4728_7Semi::fastWrite(uint16_t values[4])
{
  if (!i2c || !values)
    return false;

  uint8_t data[8];
  uint8_t idx = 0;

  for (uint8_t ch = 0; ch < 4; ++ch)
  {
    uint16_t v = values[ch] & 0x0FFF;

    uint8_t pd = 0; // normal mode

    data[idx++] =
        ((ch & 0x03) << 6) | // channel select (C1 C0)
        ((pd & 0x03) << 4) | // power-down bits
        ((v >> 8) & 0x0F);   // upper 4 bits of DAC value

    data[idx++] = v & 0xFF;
  }

  return writeCommand(data, idx);
}

bool MCP4728_7Semi::multiWrite(uint16_t values[4], bool writeEEPROM)
{
  if (!i2c || !values)
    return false;

  uint8_t data[12];
  uint8_t idx = 0;

  if (!writeEEPROM)
  {
    // Multi-write (DAC only)
    for (uint8_t ch = 0; ch < 4; ++ch)
    {
      uint16_t v = values[ch] & 0x0FFF;

      data[idx++] = MCP4728_CMD_MULTI_WRITE | (ch << 1);
      data[idx++] = (v >> 8) & 0x0F;
      data[idx++] = v & 0xFF;
    }
  }
  else
  {
    // Sequential write (DAC + EEPROM)
    data[idx++] = MCP4728_CMD_SEQ_WRITE;

    for (uint8_t ch = 0; ch < 4; ++ch)
    {
      uint16_t v = values[ch] & 0x0FFF;

      uint8_t msb =
          (0 << 7) | // VREF (0 = VDD)
          (0 << 5) | // PD (normal)
          (0 << 4) | // GAIN (1x)
          ((v >> 8) & 0x0F);

      data[idx++] = msb;
      data[idx++] = v & 0xFF;
    }
  }

  bool ok = writeCommand(data, idx);

  // EEPROM write delay (required)
  if (writeEEPROM)
    delay(30);

  return ok;
}

bool MCP4728_7Semi::sequentialWrite(uint8_t startChannel,
                                    uint16_t values[],
                                    uint8_t len)
{
  if (!i2c || !values || startChannel > 3 || len == 0 || (startChannel + len) > 4)
    return false;

  uint8_t data[20];
  uint8_t idx = 0;

  uint8_t cmd = MCP4728_CMD_SEQ_WRITE;
  cmd |= (startChannel << 1);

  data[idx++] = cmd;

  for (uint8_t i = 0; i < len; ++i)
  {
    uint16_t v = values[i] & 0x0FFF;

    uint8_t msb =
        (0 << 7) | // VREF (0 = VDD)
        (0 << 5) | // PD (normal)
        (0 << 4) | // GAIN (1x)
        ((v >> 8) & 0x0F);

    data[idx++] = msb;
    data[idx++] = v & 0xFF;
  }

  return writeCommand(data, idx);
}

bool MCP4728_7Semi::updateAllOutputs()
{
  return writeCommand(MCP4728_CMD_GENERAL_CALL_UPDATE);
}

bool MCP4728_7Semi::readDAC(uint16_t values[4])
{
  if (!values)
    return false;

  uint8_t data[24];

  if (!readData(data, 24))
    return false;

  for (uint8_t ch = 0; ch < 4; ++ch)
  {
    uint8_t msb = data[ch * 6 + 4];
    uint8_t lsb = data[ch * 6 + 5];

    values[ch] = ((msb & 0x0F) << 8) | lsb;
  }

  return true;
}

bool MCP4728_7Semi::readAll(uint16_t ADCVals[4],
                            uint16_t eepromVals[4])
{
  if (!i2c || !ADCVals || !eepromVals)
    return false;

  uint8_t data[24];

  if (!readData(data, 24))
    return false;

  for (uint8_t ch = 0; ch < 4; ++ch)
  {
    uint8_t base = ch * 6;

    // DAC register
    uint8_t dac_msb = data[base + 1];
    uint8_t dac_lsb = data[base + 2];

    // EEPROM register
    uint8_t eeprom_msb = data[base + 4];
    uint8_t eeprom_lsb = data[base + 5];

    ADCVals[ch] = ((dac_msb & 0x0F) << 8) | dac_lsb;
    eepromVals[ch] = ((eeprom_msb & 0x0F) << 8) | eeprom_lsb;
  }

  return true;
}

bool MCP4728_7Semi::setVref(uint8_t chA, uint8_t chB, uint8_t chC, uint8_t chD)
{
  uint8_t cmd = MCP4728_CMD_VREF;

  cmd |= (chA & 0x01) << 3;
  cmd |= (chB & 0x01) << 2;
  cmd |= (chC & 0x01) << 1;
  cmd |= (chD & 0x01) << 0;

  return writeCommand(cmd);
}

bool MCP4728_7Semi::setGain(uint8_t gainCH_A,
                            uint8_t gainCH_B,
                            uint8_t gainCH_C,
                            uint8_t gainCH_D)
{
  uint8_t data = MCP4728_CMD_GAIN;

  data |= (gainCH_A & 0x01) << 3;
  data |= (gainCH_B & 0x01) << 2;
  data |= (gainCH_C & 0x01) << 1;
  data |= (gainCH_D & 0x01) << 0;

  return writeCommand(data);
}

bool MCP4728_7Semi::powerDown(uint8_t pdA,
                              uint8_t pdB,
                              uint8_t pdC,
                              uint8_t pdD)
{
  uint8_t data[2];

  // First byte → command + CH A/B
  data[0] = MCP4728_CMD_POWERDOWN |
            ((pdA & 0x03) << 2) |
            ((pdB & 0x03));

  // Second byte → CH C/D
  data[1] = ((pdC & 0x03) << 6) |
            ((pdD & 0x03) << 4);

  return writeCommand(data, 2);
}

bool MCP4728_7Semi::writeCommand(uint8_t *data, uint8_t len)
{
  if (!i2c)
    return false;

  i2c->beginTransmission(i2c_address);
  i2c->write(data, len);
  return (i2c->endTransmission() == 0);
}

bool MCP4728_7Semi::writeCommand(uint8_t commandByte)
{
  if (!i2c)
    return false;

  i2c->beginTransmission(i2c_address);
  i2c->write(commandByte);

  uint8_t result = i2c->endTransmission();
  return (result == 0);
}

bool MCP4728_7Semi::setChannel(uint8_t channel, uint16_t value)
{
  if (!i2c || channel > 3)
    return false;

  value &= 0x0FFF;

  uint8_t data[3];

  data[0] = MCP4728_CMD_MULTI_WRITE | (channel << 1);
  data[1] = (value >> 8) & 0x0F; // upper 4 bits
  data[2] = value & 0xFF;        // lower 8 bits

  return writeCommand(data, 3);
}

bool MCP4728_7Semi::readData(uint8_t *buf, size_t len)
{
  if (!i2c)
  {
    status = NOT_INITIALIZED;
    return false;
  }

  if (!buf)
  {
    status = NULL_PTR;
    return false;
  }

  if (len == 0)
  {
    status = SHORT_READ;
    return false;
  }

  if (i2c->requestFrom((int)i2c_address, (int)len) != len)
  {
    status = SHORT_READ;
    return false;
  }

  for (size_t i = 0; i < len; ++i)
    buf[i] = i2c->read();

  // Serial.print("Data: 0x");
  // for (size_t i = 0; i < len; ++i)
  // {
  //   Serial.print(buf[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println(" ");

  status = OK;
  return true;
}
/**

* burstRead
  */
bool MCP4728_7Semi::burstRead(uint8_t reg, uint8_t *buf, size_t len)
{
  if (!i2c)
  {
    status = NOT_INITIALIZED;
    return false;
  }

  if (!buf)
  {
    status = NULL_PTR;
    return false;
  }

  if (len == 0)
  {
    status = SHORT_READ;
    return false;
  }

  i2c->beginTransmission(i2c_address);
  i2c->write(reg);

  if (i2c->endTransmission(false) != 0)
  {
    status = OTHER_ERROR;
    return false;
  }

  if (i2c->requestFrom((int)i2c_address, (int)len) != len)
  {
    status = SHORT_READ;
    return false;
  }

  for (size_t i = 0; i < len; ++i)
    buf[i] = i2c->read();

  // Serial.print("Data: 0x");
  // for (size_t i = 0; i < len; ++i)
  // {
  //   Serial.print(buf[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println(" ");
  status = OK;
  return true;
}
