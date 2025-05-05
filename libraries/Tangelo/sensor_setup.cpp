#include "sensor_setup.h"


// below is code taken for double tap experiment

// try to comment out when testing
AS_LSM6DSOX::AS_LSM6DSOX(void) {}


uint8_t AS_LSM6DSOX::getRegister(uint8_t theRegister) {
  Adafruit_BusIO_Register readRegister = Adafruit_BusIO_Register(spi_dev, theRegister, ADDRBIT8_HIGH_TOREAD);
  return readRegister.read();
}


void AS_LSM6DSOX::writeRegister(uint8_t theRegister, uint8_t value) {
  Adafruit_BusIO_Register writeRegister = Adafruit_BusIO_Register(spi_dev, theRegister, ADDRBIT8_HIGH_TOREAD);
  Adafruit_BusIO_RegisterBits registerBits = Adafruit_BusIO_RegisterBits(&writeRegister, 8, 0);   // write all 8 bits
  registerBits.write(value);
}


uint8_t AS_LSM6DSOX::setupTaps(void) {
  uint8_t theRegister;
 
  // disable I3C interface - the Adafruit constructor does this initially - this is how STM does it
  
  theRegister = getRegister(0x18);                      // CTRL9_XL (0x18h)
  writeRegister(0x18, theRegister | B00000010);
  
  // the I3C_BUS_AVB register inits as 0, write the entire bit field at once i3c_bus_avb_sel bits = 11

  writeRegister(0x62, B00011000);                       // I3C_BUS_AVB (62h)
  
  // chip inits with 0x70 = 833hz - set to 417hz and 2G - Full Scale
  
  writeRegister(0x10, B01100000);                       // CTRL1_XL (10h)

  // read the TAP_CFG0 and get the currently set bits, enable taps on X, Y, & Z - inits with 0x0E

  theRegister = getRegister(0x56);                      // TAP_CFG0 (56h))
  writeRegister(0x56, theRegister | B00001110);

  // set X, Y, & Z tap threshold (5 LSBs) to B1000 (0x08) - sets the tap threshold is 500 mg (= 12 * FS_XL / 32 )
  
  theRegister = getRegister(0x57);                      // X = TAP_CFG1 (57h)
  writeRegister(0x57, theRegister | B00001000);
  
  theRegister = getRegister(0x58);                      // Y = TAP_CFG2 (58h)
  writeRegister(0x58, theRegister | B00001000);
                
  theRegister = getRegister(0x59);                      // Z = TAP_CFG3 (59h)
  writeRegister(0x59, theRegister | B00001000);
                              
  // configure single and double tap parameters
  
  writeRegister(0x5A, B01111111);                       // INT_DUR2 (5Ah)

  // enable single and double tap detection
  
  theRegister = getRegister(0x5B);                      // WAKE_UP_THS (5Bh)
  writeRegister(0x5B, theRegister | B10000000);

  // TAP_CFG2 (58h) interrupts enable
  
  theRegister = getRegister(0x58);                      // TAP_CFG2 (58h)
  writeRegister(0x58, theRegister | B10000000);
  
  // 0x5E & 0x5F routes hardware interrupts
 
  return 0;
}


// below is function that uses double tap. Can be polled
bool checkForDoubleTap(AS_LSM6DSOX& sox) {
  static unsigned long tapWait;                  // decay value so double taps wont refire right away, keep this uninitialized.
  uint8_t reg;

  if (tapWait < millis()) {
    reg = sox.getRegister(0x1c);
    if (((reg & 0x40) == 0x40) && ((reg & 0x10) == 0x10)) {     // double tap
      Serial.print("double tap ");
      Serial.println(millis());
      tapWait = millis() + 1500;
      return true;
      }
    }

  return false;
}


void sensor_setup(AS_LSM6DSOX& sox) {
  // Set up SPI functionality  
  pinMode(LSM_CS1, OUTPUT);
  pinMode(LSM_CS2, OUTPUT);
  pinMode(LSM_SCK, OUTPUT);
  pinMode(LSM_MISO, INPUT);
  pinMode(LSM_MOSI, OUTPUT);


  if (!sox.begin_SPI(LSM_CS1, LSM_SCK, LSM_MISO, LSM_MOSI, LSM6DSOX_CHIP_ID)) {
    Serial.println("");
    Serial.println("Failed to find LSM6DSOX_1 chip");
  }
  else {
    Serial.println("LSM6DSOX_1 Found!");

    // Enable accelerometer with 104 Hz data rate, 4G
    // Enable gyro with 104 Hz data rate, 2000 dps

    // sox.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);   // 2_G, 4_G, 8_G, 16_G
    Serial.print("Accelerometer range set to: ");
    Serial.println(sox.getAccelRange());

    // sox.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);  // 125_DPS, 250_DPS, 500_DPS, 1000_DPS, 2000_DPS, 4000_DPS, 
    Serial.print("Gyro range set to: ");
    Serial.println(sox.getGyroRange());

    // sox.setAccelDataRate(LSM6DS_RATE_12_5_HZ);    // SHUTDOWN = 0Hz, 12_5_HZ, 26_HZ, 52_HZ, 104_HZ, 208_HZ, 416_HZ, 833_HZ, 1_66K_HZ, 3_33K_HZ, 6_66K_HZ 
    Serial.print("Accelerometer data rate set to: ");
    Serial.println(sox.getAccelDataRate());

    // sox.setGyroDataRate(LSM6DS_RATE_12_5_HZ);     // SHUTDOWN = 0Hz, 12_5_HZ, 26_HZ, 52_HZ, 104_HZ, 208_HZ, 416_HZ, 833_HZ, 1_66K_HZ, 3_33K_HZ, 6_66K_HZ
    Serial.print("Gyro data rate set to: ");
    Serial.println(sox.getGyroDataRate());
  }


  sox.reset();
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);                        // library = Adafruit_LSM6DS.h transitive inherit from Adafruit_LSM6DSOX.h
  sox.setAccelDataRate(LSM6DS_RATE_833_HZ);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  sox.setGyroDataRate(LSM6DS_RATE_1_66K_HZ);
  sox.setupTaps();

}