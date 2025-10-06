import smbus2
import Jetson.GPIO as GPIO
import time
import csv

# -------------------------
# Configuration Parameters
# -------------------------
I2C_ADDR = 0x08         # I2C address of the Teensy
I2C_BUS = 1             # Jetson Nano I2C bus number
START_PIN = 17          # GPIO pin for start trigger
END_PIN = 27            # GPIO pin for end trigger
MAX_SAMPLES = 5000      # Expected maximum number of samples
CSV_FILENAME = "data_log.csv"

# -------------------------
# Helper Functions
# -------------------------
def wait_for_event(pin, edge):
    """Wait for a GPIO edge event."""
    GPIO.wait_for_edge(pin, edge)

def i2c_write(cmd):
    """Send a single-byte command to the Teensy."""
    bus.write_byte(I2C_ADDR, ord(cmd))

def read_samples(num_samples):
    """Read samples from Teensy, each sample is 2 bytes."""
    data = []
    for _ in range(num_samples):
        try:
            raw = bus.read_i2c_block_data(I2C_ADDR, 0, 2)
            val = raw[0] | (raw[1] << 8)
            data.append(val)
        except OSError:
            # Stop when Teensy stops responding (end of buffer)
            break
    return data

# -------------------------
# Main Program
# -------------------------
def main():
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(START_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(END_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    print("Waiting for falling edge to start...")
    wait_for_event(START_PIN, GPIO.FALLING)
    print("Start trigger received")

    # Tell Teensy to start collecting data
    i2c_write('S')
    time.sleep(0.01)

    print("Collecting data... waiting for end trigger.")
    wait_for_event(END_PIN, GPIO.RISING)
    print("End trigger received")

    # Stop collection
    i2c_write('E')
    time.sleep(0.05)

    print("Reading data from Teensy...")
    samples = read_samples(MAX_SAMPLES)
    print(f"Received {len(samples)} samples.")

    # Save data to CSV
    with open(CSV_FILENAME, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["Sample Index", "ADC Value"])
        for i, val in enumerate(samples):
            writer.writerow([i, val])

    print(f"Data saved to {CSV_FILENAME}")

    GPIO.cleanup()

# -------------------------
# Entry Point
# -------------------------
if __name__ == "__main__":
    bus = smbus2.SMBus(I2C_BUS)
    try:
        main()
    finally:
        GPIO.cleanup()
        bus.close()
