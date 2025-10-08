#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <vector>

Adafruit_ADS1115 ads;
std::vector<int16_t> samples;

const int I2C_ADDR = 0x08;
const unsigned long sample_interval_us = 1000; // 1ms sampling interval
bool collecting = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  ads.begin();
  ads.setGain(GAIN_ONE); // ±4.096V range
  Serial.println("Teensy I2C + ADS1115 ready");
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

void receiveEvent(int howMany) {
  while (Wire.available()) {
    char c = Wire.read();

    if (c == 'S') {  // Start collection
      collecting = true;
      samples.clear(); // clear any old data
      Serial.println("Started data collection");
    } 
    else if (c == 'E') {  // End collection and prepare to send
      collecting = false;
      Serial.println("Stopped data collection");
      Serial.print("Collected ");
      Serial.print(samples.size());
      Serial.println(" samples");
    }
  }
}

void requestEvent() {
  // Send the data length first (2 bytes)
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
    Wire.write((uint8_t*)0, 2); // send zeros when done
    sending = false;

    // Clear buffer after sending
    if (samples.size() > 0) {
      Serial.println("Data sent to Jetson, clearing buffer...");
      samples.clear();
    }
  }
}
