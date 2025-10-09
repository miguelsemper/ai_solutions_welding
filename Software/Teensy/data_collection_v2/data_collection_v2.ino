#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <vector>

// --- ADS1115 on I2C1 ---
Adafruit_ADS1115 ads;

// --- I2C0 (Wire) for Jetson communication ---
#define JETSON_ADDR 0x08

std::vector<int16_t> samples;
const unsigned long sample_interval_us = 1000; // 1 ms sampling
bool collecting = false;

void setup() {
  Serial.begin(115200);

  // Initialize I2C0 (Jetson)
  Wire.begin(JETSON_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  // Initialize I2C1 (ADS1115)
  Wire1.begin();
  ads.begin(0x48, &Wire1);   // Pass Wire1 here
  ads.setGain(GAIN_ONE);     // ±4.096V input range

  Serial.println("Teensy ready: I2C0 -> Jetson, I2C1 -> ADS1115");
}

void loop() {
  static unsigned long lastSampleTime = 0;

  if (collecting) {
    unsigned long now = micros();
    if (now - lastSampleTime >= sample_interval_us) {
      lastSampleTime = now;
      int16_t adc0 = ads.readADC_SingleEnded(0);
      samples.push_back(adc0);
    }
  }
}

// --- I2C communication with Jetson ---

void receiveEvent(int howMany) {
  while (Wire.available()) {
    char c = Wire.read();

    if (c == 'S') {  // Start collection
      collecting = true;
      samples.clear();
      Serial.println("Started data collection");
    } 
    else if (c == 'E') {  // End collection
      collecting = false;
      Serial.print("Stopped data collection. Total samples: ");
      Serial.println(samples.size());
    }
  }
}

void requestEvent() {
  static size_t index = 0;
  static bool sending = false;

  if (!sending && samples.size() > 0) {
    index = 0;
    sending = true;
  }

  if (sending && index < samples.size()) {
    int16_t value = samples[index++];
    Wire.write((uint8_t*)&value, 2);
  } else {
    Wire.write((uint8_t*)0, 2); // Send zeros when done
    sending = false;

    // Clear buffer after transmission
    if (!samples.empty()) {
      Serial.println("Data sent to Jetson. Clearing buffer...");
      samples.clear();
    }
  }
}