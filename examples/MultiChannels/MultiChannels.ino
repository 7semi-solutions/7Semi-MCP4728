/***************************************************************************************************

  7Semi MCP4728  – 4 Channel Basic Output


  Description:


  Initializes MCP4728 DAC


  Writes values to all 4 channels


  Prints corresponding output voltages


  Hardware Connection:


  MCP4728 → MCU

  VCC → 3.3V / 5V
  GND → GND
  SDA → SDA (UNO: A4, ESP32: GPIO21)
  SCL → SCL (UNO: A5, ESP32: GPIO22)


  Notes:


  Default I2C address = 0x60


  DAC resolution = 12-bit (0–4095)


  Output voltage depends on VREF & GAIN
***************************************************************************************************/
#include <7Semi_MCP4728.h>

#define MCP4728_ADDR 0x60
#define VREF_VOLTAGE 3.30 // Correct for internal Vref + Gain x2
#define ADC_MAX 1023.0    // Change to 4095 for ESP32

MCP4728_7Semi dac;

/** DAC values */
uint16_t values[4] = {0, 1024, 2048, 3072};

/** Analog input pins */
uint8_t pins[4] = {A0, A1, A2, A3};

void setup()
{
  Serial.begin(115200);
  Wire.begin();

  if (!dac.begin(MCP4728_ADDR, Wire, 400000))
  {
    Serial.println("ERROR: MCP4728 init failed");
    while (1)
      ;
  }

  Serial.println("MCP4728 Ready");

  dac.setChannelEEPROM(0, 0, 0, 0, 0);
  delay(500);
  dac.setChannelEEPROM(1, 0, 0, 0, 0);
  delay(500);
  dac.setChannelEEPROM(2, 0, 0, 0, 0);
  delay(500);
  dac.setChannelEEPROM(3, 0, 0, 0, 0);
  delay(500);

  dac.setVref(0, 0, 0, 0);
  dac.setGain(0, 0, 0, 0);
}

void loop()
{
  static bool increasing = true;

  dac.fastWrite(values);

  Serial.print("SET: ");
  for (int i = 0; i < 4; i++)
  {
    float v = values[i] * VREF_VOLTAGE / 4095.0;
    Serial.print(v, 3);
    Serial.print(", ");
  }
  Serial.println();
  delay(500);

  Serial.print("OUT: ");
  for (int i = 0; i < 4; i++)
  {
    uint16_t adc = analogRead(pins[i]);
    float v = adc * VREF_VOLTAGE / ADC_MAX;
    Serial.print(v, 3);
    Serial.print(", ");
  }
  Serial.println("\n");

  for (int i = 0; i < 4; i++)
  {
    if (increasing)
    {
      values[i] += 100;
      if (values[i] >= 4095)
      {
        values[i] = 4095;
        increasing = false;
      }
    }
    else
    {
      if (values[i] > 100)
        values[i] -= 100;
      else
      {
        values[i] = 0;
        increasing = true;
      }
    }
  }
}
