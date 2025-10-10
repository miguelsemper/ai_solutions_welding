import smbus2
import Jetson.GPIO as GPIO
import time
import csv
from datetime import datetime

# -------------------------
# Configuration Parameters
# -------------------------
I2C_ADDR = 0x08          # I2C address of the Teensy
I2C_BUS = 7              # Jetson Nano I2C bus number
START_PIN = 12           # GPIO pin for start trigger
END_PIN = 12             # GPIO pin for end trigger
MAX_SAMPLES = 5000       # Expected maximum number of samples
CSV_FILENAME = "/home/etm3/Documents/welding/data_log.csv"  # Change path as needed

# -------------------------
# Helper Functions
# -------------------------
def wait_for_event(pin, edge):
    GPIO.wait_for_edge(12, edge)

def i2c_write(cmd):
    bus.write_byte(I2C_ADDR, ord(cmd))

def read_samples(num_samples):
    data = []
    for _ in range(num_samples):
        try:
            raw = bus.read_i2c_block_data(I2C_ADDR, 0, 2)
            val = raw[0] | (raw[1] << 8)
            data.append(val)
        except OSError:
            break
    return data

# -------------------------
# Main Program
# -------------------------
def main():
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(START_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
    GPIO.setup(END_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    print("Ready and waiting for triggers... (Press Ctrl+C to stop)\n")

    # Create CSV if it doesn't exist and add header
    with open(CSV_FILENAME, "a", newline="") as f:
        writer = csv.writer(f)
        if f.tell() == 0:  # only write header if file is empty
            writer.writerow(["Timestamp", "Samples..."])

    while True:
        print("Waiting for falling edge on START pin...")
        wait_for_event(START_PIN, GPIO.FALLING)
        print("Start trigger received")

        # Tell Teensy to start collecting data
        i2c_write('S')
        time.sleep(0.01)

        print("Collecting data... waiting for END trigger.")
        wait_for_event(END_PIN, GPIO.RISING)
        print("End trigger received")

        # Stop collection
        i2c_write('E')
        time.sleep(0.05)

        print("Reading data from Teensy...")
        samples = read_samples(MAX_SAMPLES)
        print(f"Received {len(samples)} samples.")

        # Save all samples as one row (timestamp + data)
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open(CSV_FILENAME, "a", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([timestamp] + samples)

        print(f"Data saved. Waiting for next trigger...\n")

# -------------------------
# Entry Point
# -------------------------
if __name__ == "__main__":
    bus = smbus2.SMBus(I2C_BUS)
    try:
        main()
    except KeyboardInterrupt:
        print("Program terminated by user.")
    finally:
        GPIO.cleanup()
        bus.close()
