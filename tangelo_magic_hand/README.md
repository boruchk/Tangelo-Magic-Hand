## note: DO NOT use pin 12 and 13, there is firmware bug when uploading code

### connection instruction: this mapping is very neatly in-order, no wire should cross with any other wire
 - add additional CS pin connection for additional LSM sensors, must use digital pin on ESP
 - shared 5-channel SPI standard wiring across all LSM sensors
 - SPI favored over I2C for bandwidth and low latency