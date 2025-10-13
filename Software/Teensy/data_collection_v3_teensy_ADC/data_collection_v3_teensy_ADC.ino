#include <Wire.h>
#include <vector>

#define JETSON_ADDR 0x08
#define ANALOG_PIN A0  // Teensy pin where your analog signal is connected

std::vector<int16_t> samples;
const unsigned long sample_interval_us = 1000;  // 1 kHz sampling (1 ms per sample)
bool collecting = false;
bool sending = false;
size_t send_index = 0;
uint16_t total_samples = 0;
bool header_sent = false;

void setup() {
  Serial.begin(115200);

  // --- I2C for Jetson communication ---
  Wire.begin(JETSON_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  // --- ADC setup ---
  analogReadResolution(12);   // 12-bit ADC (0–4095)
  analogReadAveraging(1);     // no averaging for maximum speed

  Serial.println("Teensy ready: I2C0→Jetson, internal ADC on A0");
}

void loop() {
  static unsigned long lastSampleTime = 0;

  if (collecting) {
    unsigned long now = micros();
    if (now - lastSampleTime >= sample_interval_us) {
      lastSampleTime = now;

      // Read the internal ADC
      int16_t adc_val = analogRead(ANALOG_PIN);
      samples.push_back(adc_val);
    }
  }
}

// --- I2C event handlers ---
void receiveEvent(int howMany) {
  while (Wire.available()) {
    char cmd = Wire.read();

    if (cmd == 'S') {  // Start collection
      collecting = true;
      sending = false;
      samples.clear();
      header_sent = false;
      Serial.println("Started data collection");
    }

    else if (cmd == 'E') {  // Stop collection
      collecting = false;
      sending = true;
      send_index = 0;
      total_samples = samples.size();
      header_sent = false;
      Serial.print("Stopped data collection. Total samples: ");
      Serial.println(total_samples);
    }
  }
}

void requestEvent() {
  if (!sending) {
    // Idle: send zeros when not sending data
    uint16_t zero = 0;
    Wire.write((uint8_t*)&zero, 2);
    return;
  }

  // --- First send the header (number of samples) ---
  if (!header_sent) {
    Wire.write((uint8_t*)&total_samples, 2);
    header_sent = true;
    Serial.print("Header sent (samples = ");
    Serial.print(total_samples);
    Serial.println(")");
    return;
  }

  // --- Then send samples ---
  if (send_index < samples.size()) {
    int16_t value = samples[send_index++];
    Wire.write((uint8_t*)&value, 2);

    // When last sample sent, clear buffer
    if (send_index >= samples.size()) {
      Serial.println("All data sent to Jetson. Clearing buffer...");
      samples.clear();
      sending = false;
    }
  }
}