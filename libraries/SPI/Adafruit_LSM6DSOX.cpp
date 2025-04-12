
/*!
 *  @file Adafruit_LSM6DSOX.cpp
 *  Adafruit LSM6DSOX 6-DoF Accelerometer and Gyroscope library
 *  Optimized from https://github.com/adafruit/Adafruit_LSM6DS/blob/master/Adafruit_LSM6DSOX.cpp
 *
 *  Adapted from:
 *  Bryan Siepert for Adafruit Industries
 * 	BSD (see license.txt)
 */

#include "Adafruit_LSM6DSOX.h"

/*!
 *    @brief  Instantiates a new LSM6DSOX class
 */
Adafruit_LSM6DSOX::Adafruit_LSM6DSOX(void) {}

bool Adafruit_LSM6DSOX::_init(int32_t sensor_id) {
  Adafruit_BusIO_Register chip_id = Adafruit_BusIO_Register(
      spi_dev, LSM6DS_WHOAMI, ADDRBIT8_HIGH_TOREAD);

  // make sure we're talking to the right chip
  if (chip_id.read() != LSM6DSOX_CHIP_ID) {
    Serial.println(chip_id.read(), HEX);
    Serial.println("Adafruit_LSM6DSOX::_init() -> chip_id.read() != LSM6DSOX_CHIP_ID");
    return false;
  }
  _sensorid_accel = sensor_id;
  _sensorid_gyro = sensor_id + 1;

  reset();

  // Block Data Update
  // this prevents MSB/LSB data registers from being updated until both are read
  Adafruit_BusIO_Register ctrl3 = Adafruit_BusIO_Register(
      spi_dev, LSM6DSOX_CTRL3_C, ADDRBIT8_HIGH_TOREAD);
  Adafruit_BusIO_RegisterBits bdu = Adafruit_BusIO_RegisterBits(&ctrl3, 1, 6);
  bdu.write(true);

  // Disable I3C
  Adafruit_BusIO_Register ctrl_9 = Adafruit_BusIO_Register(
      spi_dev, LSM6DSOX_CTRL9_XL, ADDRBIT8_HIGH_TOREAD);
  Adafruit_BusIO_RegisterBits i3c_disable_bit =
      Adafruit_BusIO_RegisterBits(&ctrl_9, 1, 1);

  i3c_disable_bit.write(true);

  // call base class _init()
  Adafruit_LSM6DS::_init(sensor_id);

  return true;
}
