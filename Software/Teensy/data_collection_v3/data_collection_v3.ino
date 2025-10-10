#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <vector>

Adafruit_ADS1115 ads; // ADS1115 on Wire1

#define JETSON_ADDR 0x08

std::vector<int16_t> samples;                 // dynamic buffer for channel 0
const unsigned long sample_interval_us = 1000; // 1ms per sample
bool collecting = false;
bool sending = false;
size_t send_index = 0;
uint16_t total_samples = 0;
bool header_sent = false;                     // to track if the header has been sent

void setup() {
  Serial.begin(115200);

  // --- ADS1115 on I2C1 ---
  Wire1.begin();
  ads.begin(0x48, &Wire1);   // ADS1115 on Wire1
  ads.setGain(GAIN_ONE);     // ±4.096V range

  // --- I2C0 for Jetson ---
  Wire.begin(JETSON_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  Serial.println("Teensy ready: I2C0->Jetson, I2C1->ADS1115, Channel 0 only");
}

void loop() {
  static unsigned long lastSampleTime = 0;
  if (collecting) {
    unsigned long now = micros();
    if (now - lastSampleTime >= sample_interval_us) {
      lastSampleTime = now;
      int16_t adc0 = ads.readADC_SingleEnded(0); // read channel 0 only
      samples.push_back(adc0);
    }
  }
}

// --- I2C event handlers ---
void receiveEvent(int howMany) {
  while (Wire.available()) {
    char cmd = Wire.read();
    if (cmd == 'S') {
      collecting = true;
      sending = false;
      samples.clear();
      header_sent = false;
      Serial.println("Started data collection");
    }
    else if (cmd == 'E') {
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
    if (send_index == (samples.size()-1)){
      Serial.println("All data sent to Jetson. Clearing buffer...");
      samples.clear();
      sending = false;
    }
  } else {
  }
}
