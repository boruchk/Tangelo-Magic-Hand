// Basic demo for accelerometer & gyro readings from Adafruit
// LSM6DSOX sensor

#include <Adafruit_LSM6DSOX.h>

// For SPI mode, we need a CS pin
#define LSM_CS 23
#define LSM_CS0 21
// For software-SPI mode we need SCK/MOSI/MISO pins
#define LSM_SCK 32 //SCL
#define LSM_MISO 22 //DO
#define LSM_MOSI 14 //SDA

Adafruit_LSM6DSOX sox;
Adafruit_LSM6DSOX sox0;
void setup(void) {
    Serial.begin(115200);
    while (!Serial)
        delay(10); // will pause Zero, Leonardo, etc until serial console opens

    Serial.println("Adafruit LSM6DSOX test!");

    // if (!sox.begin_I2C()) {
    // if (!sox.begin_SPI(LSM_CS)) {
    if (!sox.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
        Serial.println("Failed to find first LSM6DSOX chip");
        while (1) {
            delay(10);
        }

        if (!sox0.begin_SPI(LSM_CS0, LSM_SCK, LSM_MISO, LSM_MOSI)) {
            Serial.println("Failed to find second LSM6DSOX chip");
            while (1) {
                delay(10);
            }

        }

        Serial.println("double LSM6DSOX Found!");

        // sox.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);
        Serial.print("Accelerometer range set to: ");
        switch (sox.getAccelRange()) {
            case LSM6DS_ACCEL_RANGE_2_G:
                Serial.println("+-2G");
                break;
            case LSM6DS_ACCEL_RANGE_4_G:
                Serial.println("+-4G");
                break;
            case LSM6DS_ACCEL_RANGE_8_G:
                Serial.println("+-8G");
                break;
            case LSM6DS_ACCEL_RANGE_16_G:
                Serial.println("+-16G");
                break;
        }

        // sox.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS );
        Serial.print("Gyro range set to: ");
        switch (sox.getGyroRange()) {
            case LSM6DS_GYRO_RANGE_125_DPS:
                Serial.println("125 degrees/s");
                break;
            case LSM6DS_GYRO_RANGE_250_DPS:
                Serial.println("250 degrees/s");
                break;
            case LSM6DS_GYRO_RANGE_500_DPS:
                Serial.println("500 degrees/s");
                break;
            case LSM6DS_GYRO_RANGE_1000_DPS:
                Serial.println("1000 degrees/s");
                break;
            case LSM6DS_GYRO_RANGE_2000_DPS:
                Serial.println("2000 degrees/s");
                break;
            case ISM330DHCX_GYRO_RANGE_4000_DPS:
                break; // unsupported range for the DSOX
        }

        // sox.setAccelDataRate(LSM6DS_RATE_12_5_HZ);
        Serial.print("Accelerometer data rate set to: ");
        switch (sox.getAccelDataRate()) {
            case LSM6DS_RATE_SHUTDOWN:
                Serial.println("0 Hz");
                break;
            case LSM6DS_RATE_12_5_HZ:
                Serial.println("12.5 Hz");
                break;
            case LSM6DS_RATE_26_HZ:
                Serial.println("26 Hz");
                break;
            case LSM6DS_RATE_52_HZ:
                Serial.println("52 Hz");
                break;
            case LSM6DS_RATE_104_HZ:
                Serial.println("104 Hz");
                break;
            case LSM6DS_RATE_208_HZ:
                Serial.println("208 Hz");
                break;
            case LSM6DS_RATE_416_HZ:
                Serial.println("416 Hz");
                break;
            case LSM6DS_RATE_833_HZ:
                Serial.println("833 Hz");
                break;
            case LSM6DS_RATE_1_66K_HZ:
                Serial.println("1.66 KHz");
                break;
            case LSM6DS_RATE_3_33K_HZ:
                Serial.println("3.33 KHz");
                break;
            case LSM6DS_RATE_6_66K_HZ:
                Serial.println("6.66 KHz");
                break;
        }

        // sox.setGyroDataRate(LSM6DS_RATE_12_5_HZ);
        Serial.print("Gyro data rate set to: ");
        switch (sox.getGyroDataRate()) {
            case LSM6DS_RATE_SHUTDOWN:
                Serial.println("0 Hz");
                break;
            case LSM6DS_RATE_12_5_HZ:
                Serial.println("12.5 Hz");
                break;
            case LSM6DS_RATE_26_HZ:
                Serial.println("26 Hz");
                break;
            case LSM6DS_RATE_52_HZ:
                Serial.println("52 Hz");
                break;
            case LSM6DS_RATE_104_HZ:
                Serial.println("104 Hz");
                break;
            case LSM6DS_RATE_208_HZ:
                Serial.println("208 Hz");
                break;
            case LSM6DS_RATE_416_HZ:
                Serial.println("416 Hz");
                break;
            case LSM6DS_RATE_833_HZ:
                Serial.println("833 Hz");
                break;
            case LSM6DS_RATE_1_66K_HZ:
                Serial.println("1.66 KHz");
                break;
            case LSM6DS_RATE_3_33K_HZ:
                Serial.println("3.33 KHz");
                break;
            case LSM6DS_RATE_6_66K_HZ:
                Serial.println("6.66 KHz");
                break;
        }
    }
}
void loop() {

    //  /* Get a new normalized sensor event */

    digitalWrite(LSM_CS, LOW);
    digitalWrite(LSM_CS0, HIGH);  // Deselect sensor 2

    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    sox.getEvent(&accel, &gyro, &temp);



    digitalWrite(LSM_CS, HIGH);
    digitalWrite(LSM_CS0, LOW);  // Deselect sensor 1


    sensors_event_t accel0;
    sensors_event_t gyro0;
    sensors_event_t temp0;
    sox0.getEvent(&accel0, &gyro0, &temp0);


    Serial.print("\t\tTemperature ");
    Serial.print(temp.temperature);
    Serial.print("\t\tSecond Temperature ");
    Serial.print(temp0.temperature);
    Serial.println(" deg C");

    /* Display the results (acceleration is measured in m/s^2) */
    Serial.print("\t\tAccel X: ");
    Serial.print(accel.acceleration.x);
    Serial.print(" \tY: ");
    Serial.print(accel.acceleration.y);
    Serial.print(" \tZ: ");
    Serial.print(accel.acceleration.z);
    Serial.println(" m/s^2 ");

    /* Display the results (rotation is measured in rad/s) */
    Serial.print("\t\tGyro X: ");
    Serial.print(gyro.gyro.x);

    Serial.print("\t\tSecond Gyro X: ");
    Serial.print(gyro0.gyro.x);

    Serial.print(" \tY: ");
    Serial.print(gyro.gyro.y);
    Serial.print(" \tZ: ");
    Serial.print(gyro.gyro.z);
    Serial.println(" radians/s ");
    Serial.println();

    delay(100);

    // serial plotter friendly format

    Serial.print(temp.temperature);
    Serial.print(",");

    Serial.print(accel.acceleration.x);
    Serial.print(","); Serial.print(accel.acceleration.y);
    Serial.print(","); Serial.print(accel.acceleration.z);
    Serial.print(",");

    Serial.print(gyro.gyro.x);
    Serial.print(","); Serial.print(gyro.gyro.y);
    Serial.print(","); Serial.print(gyro.gyro.z);
    Serial.println();
    delayMicroseconds(10000);
}