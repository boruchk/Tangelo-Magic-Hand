// Basic demo for accelerometer & gyro readings from Adafruit
// LSM6DSOX sensor

#include <Adafruit_LSM6DSOX.h>
#include <Wire.h>                   //additional include for double tap

// For SPI mode, we need a CS pin
#define LSM_CS 23
#define LSM_CS0 21
// For software-SPI mode we need SCK/MOSI/MISO pins
#define LSM_SCK 19 //SCL
#define LSM_MOSI 16 //SDA
#define LSM_MISO 17 //DO





//below is code taken for double tap experiment
class AS_LSM6DSOX : public Adafruit_LSM6DSOX
{
  public:
    AS_LSM6DSOX();
    uint8_t getRegister(uint8_t theRegister);
    void writeRegister(uint8_t theRegister, uint8_t value);
    uint8_t setupTaps();
};



AS_LSM6DSOX::AS_LSM6DSOX(void) {}

uint8_t AS_LSM6DSOX::getRegister(uint8_t theRegister)
{
  Adafruit_BusIO_Register readRegister = Adafruit_BusIO_Register(i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, theRegister);
  return readRegister.read();
}

void AS_LSM6DSOX::writeRegister(uint8_t theRegister, uint8_t value)
{
  Adafruit_BusIO_Register writeRegister = Adafruit_BusIO_Register(i2c_dev, spi_dev, ADDRBIT8_HIGH_TOREAD, theRegister);
  Adafruit_BusIO_RegisterBits registerBits = Adafruit_BusIO_RegisterBits(&writeRegister, 8, 0);   // write all 8 bits
  registerBits.write(value);
}

uint8_t AS_LSM6DSOX::setupTaps(void)
{
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
AS_LSM6DSOX sox;
Adafruit_LSM6DSOX sox0; //dummy for reverting version
//above is modified macro init

//below is function that uses double tap. Can be polled
bool checkForDoubleTap()
{
  static unsigned long  tapWait;                  // decay value so double taps wont refire right away, keep this uninitialized.
  uint8_t               reg;

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

//above is function that can be polled for doubletap


//above is code taken for double tap experiment




void setup(void) {
  Serial.begin(115200);
  while (!Serial)
  {    delay(10); // will pause Zero, Leonardo, etc until serial console opens
  }

  Serial.println("Adafruit LSM6DSOX test!");

  // if (!sox.begin_I2C()) {
    // if (!sox.begin_SPI(LSM_CS)) {
    if (!sox.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) 
  {
    Serial.println("Failed to find first LSM6DSOX chip");
    while (1) 
    {
      delay(10);
    }
  }

  Serial.println("rught before extra setup!");

  //below additional doubletap code setup call
  sox.reset();
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);                        // library = Adafruit_LSM6DS.h transitive inherit from Adafruit_LSM6DSOX.h
  sox.setAccelDataRate(LSM6DS_RATE_833_HZ);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  sox.setGyroDataRate(LSM6DS_RATE_1_66K_HZ);
  sox.setupTaps();
  //above doubletap setup call

 
}

void loop() {

  //  /* Get a new normalized sensor event */

  digitalWrite(LSM_CS, LOW);
  digitalWrite(LSM_CS0, HIGH);  // Deselect sensor 2

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  sox.getEvent(&accel, &gyro, &temp);



  // digitalWrite(LSM_CS, HIGH);
  // digitalWrite(LSM_CS0, LOW);  // Deselect sensor 1


// sensors_event_t accel0;
//   sensors_event_t gyro0;
//   sensors_event_t temp0;
  // sox0.getEvent(&accel0, &gyro0, &temp0);


  //  Serial.write(27);       // ESC character
  // Serial.print("[2J");    // Clear screen
  // Serial.write(27);
  checkForDoubleTap();  //test call
  // Serial.print("[H");     // Cursor to home position

  // Display temperatures
  // Serial.print("Temperature: ");
  // Serial.print(temp.temperature);
  // Serial.print(" °C\t");

  // Serial.print("Second Temperature: ");
  // Serial.print(temp0.temperature);
  // Serial.println(" °C");

  // // Display acceleration
  // Serial.print("Accel X: ");
  // Serial.print(accel.acceleration.x);
  // Serial.print("\tY: ");
  // Serial.print(accel.acceleration.y);
  // Serial.print("\tZ: ");
  // Serial.print(accel.acceleration.z);
  // Serial.println(" m/s^2");

  // // Display gyroscope
  // Serial.print("Gyro X: ");
  // Serial.print(gyro.gyro.x);
  // Serial.print("\tSecond Gyro X: ");
  // Serial.print(gyro0.gyro.x);
  // Serial.print("\tY: ");
  // Serial.print(gyro.gyro.y);
  // Serial.print("\tZ: ");
  // Serial.print(gyro.gyro.z);
  // Serial.println(" rad/s");

  // // Add spacing or a separator
  // Serial.println("--------------------------------------------------");

  // // Delay
  // delay(100);

  // // Serial Plotter friendly format
  // Serial.print(temp.temperature);
  // Serial.print(",");
  // Serial.print(accel.acceleration.x);
  // Serial.print(",");
  // Serial.print(accel.acceleration.y);
  // Serial.print(",");
  // Serial.print(accel.acceleration.z);
  // Serial.print(",");
  // Serial.print(gyro.gyro.x);
  // Serial.print(",");
  // Serial.print(gyro.gyro.y);
  // Serial.print(",");
  // Serial.println(gyro.gyro.z);

  delayMicroseconds(1);
}
