import Jetson.GPIO as GPIO
import time

PIN = 7  # Physical pin number (BOARD mode)
READ_INTERVAL = 0.5  # seconds

GPIO.setmode(GPIO.BOARD)
GPIO.setup(PIN, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)  # Default LOW if unconnected

print(f"Reading state of pin {PIN}. Press Ctrl+C to exit.")

try:
    while True:
        state = GPIO.input(PIN)
        if state:
            print("Pin is HIGH")
        else:
            print("Pin is LOW")
        time.sleep(READ_INTERVAL)

except KeyboardInterrupt:
    print("Exiting...")

finally:
    GPIO.cleanup()
