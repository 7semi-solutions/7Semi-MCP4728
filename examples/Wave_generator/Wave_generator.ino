/***************************************************************************************************
* 7Semi MCP4728 – 4 Channel Sine Wave Generator
*
* * Generates smooth sine waves on all 4 channels
* * Each channel is phase shifted (0°, 90°, 180°, 270°)
* * Uses fastWrite (no EEPROM)
*
* Notes:
* * Output range: 0 → VDD (3.3V)
* * Uses lookup table for performance
***************************************************************************************************/
#include <7Semi_MCP4728.h>

#define MCP4728_ADDR 0x60
#define VREF_VOLTAGE 3.30
#define SAMPLES 64  // higher = smoother wave

MCP4728_7Semi dac;

/** Sine lookup table */
uint16_t sineTable[SAMPLES];

/** DAC values */
uint16_t values[4];

void generateSineTable() {
  for (int i = 0; i < SAMPLES; i++) {
    float angle = (2 * PI * i) / SAMPLES;
    float s = (sin(angle) + 1.0) / 2.0;  // normalize 0–1
    sineTable[i] = (uint16_t)(s * 4095);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!dac.begin(MCP4728_ADDR, Wire, 400000)) {
    Serial.println("ERROR: MCP4728 init failed");
    while (1)
      ;
  }

  Serial.println("MCP4728 Ready");

  generateSineTable();
}

void loop() {
  static uint16_t idx = 0;

  /**

* Phase shift offsets
  */
  uint16_t offset = SAMPLES / 4;

  values[0] = sineTable[(idx) % SAMPLES];               // 0°
  values[1] = sineTable[(idx + offset) % SAMPLES];      // 90°
  values[2] = sineTable[(idx + 2 * offset) % SAMPLES];  // 180°
  values[3] = sineTable[(idx + 3 * offset) % SAMPLES];  // 270°

  dac.fastWrite(values);

  for (int i = 0; i < 4; i++) {
    float v = values[i] * VREF_VOLTAGE / 4095.0;
    Serial.print(v, 3);
    Serial.print(", ");
  }
  Serial.println();

  idx++;
  if (idx >= SAMPLES)
    idx = 0;

  delay(10); 
}
