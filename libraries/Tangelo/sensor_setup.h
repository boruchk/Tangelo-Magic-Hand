#ifndef SENSOR_SETUP_H
#define SENSOR_SETUP_H

#include "Adafruit_LSM6DSOX.h"


#define LSM_CS1 23  // esp32 pin SDA - chip select for LSM6DSOX #1
#define LSM_CS2 21  // esp32 pin D21 - chip select for LSM6DSOX #2
#define LSM_SCK 19  // esp32 pin MISO
#define LSM_MOSI 16 // esp32 pin D16/RX
#define LSM_MISO 17 // esp32 pin D17/TX


class AS_LSM6DSOX : public Adafruit_LSM6DSOX {
public:
  AS_LSM6DSOX();
  uint8_t getRegister(uint8_t theRegister);
  void writeRegister(uint8_t theRegister, uint8_t value);
  uint8_t setupTaps();
};

bool checkForDoubleTap(AS_LSM6DSOX& sox);

void sensor_setup(AS_LSM6DSOX& sox);

#endif