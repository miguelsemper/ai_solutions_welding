#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;
#define I2C_SLAVE_ADDR 0x08
#define MAX_SAMPLES 5000   // adjust as needed
#define SAMPLE_INTERVAL_MS 1

uint16_t samples[MAX_SAMPLES];
volatile int sampleCount = 0;
bool collecting = false;
unsigned long lastSampleTime = 0;

void onReceive(int numBytes);
void onRequest();

void setup() {
  Wire.begin(I2C_SLAVE_ADDR);   // Initialize as I2C slave
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  
  Serial.begin(115200);
  ads.begin();

  Serial.println("Teensy I2C ADC collector ready");
}

void loop() {
  if (collecting) {
    unsigned long now = millis();
    if (now - lastSampleTime >= SAMPLE_INTERVAL_MS && sampleCount < MAX_SAMPLES) {
      lastSampleTime = now;
      int16_t adc0 = ads.readADC_SingleEnded(0);
      samples[sampleCount++] = adc0;
    }
  }
}

void onReceive(int numBytes) {
  if (numBytes < 1) return;
  char cmd = Wire.read();

  if (cmd == 'S') {  // Start collection
    collecting = true;
    sampleCount = 0;
    lastSampleTime = millis();
  } 
  else if (cmd == 'E') {  // End collection
    collecting = false;
  } 
  else if (cmd == 'C') {  // Clear buffer
    sampleCount = 0;
  }
}

void onRequest() {
  // Send all samples sequentially
  // Each sample is 2 bytes
  static int sendIndex = 0;
  if (sendIndex < sampleCount) {
    uint16_t value = samples[sendIndex++];
    Wire.write((uint8_t *)&value, 2);
  } else {
    sendIndex = 0; // reset after sending all
  }
}
