/* 
TODO:
- Figure out how to:
  - Make long connection interval and low latency when connecting or sending lots of data, Reduce when in normal running mode
  - Change advertising TX power and TX power while in a connection to increase battery life 
    Enable the LE Power Control feature if available
- Make a sleep function that for when device is inactive (if possible)
  - Make check in loop() that checks for 
- Set up ability to show battery percentage
*/

#include "bluetooth_setup.h"
#include "sensor_setup.h"

// Device pin constants 
#define ACCEL_MOUSE 33
#define GYRO_MOUSE 15
#define BUTTON_PIN 32

// Mouse button constants
#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BACK 8
#define MOUSE_FORWARD 16

// Gyroscope-related constants
#define ROT_X_BOUND PI / 4
#define ROT_Y_BOUND PI / 6
#define ROT_X_CORRECT 0.035f
#define ROT_Y_CORRECT 0.02f

// LSM6DSOX chip number 1
// Rename to "mouse", "Button", etc when finishing
AS_LSM6DSOX sox1;
AS_LSM6DSOX sox2;

signed char mouse_move_x = 0,
            mouse_move_y = 0,
            mouse_v_wheel = 0,
            mouse_h_wheel = 0;
uint8_t     mouse_button_press = 0;

float x_radians = 0.0f;
float y_radians = 0.0f;

int dt = 10;  // polling frequency in ms 

static inline void flush()
{
    if (can_communicate()) 
    {
        uint8_t mouse[5];
        mouse[0] = mouse_button_press;  // Button
        mouse[1] = mouse_move_x;        // Move in X
        mouse[2] = mouse_move_y;        // Move in Y
        mouse[3] = mouse_v_wheel;       // Vertical scroll
        mouse[4] = mouse_h_wheel;       // Horizontal scroll

        inputMouse->setValue(mouse, sizeof(mouse));
        inputMouse->notify();

        if (mouse_move_x != 0 || mouse_move_y != 0)
        {
            Serial.println("Moving mouse cursor:"); 
            Serial.println("X:"); 
            Serial.println(mouse_move_x);
            Serial.println("Y:"); 
            Serial.println(mouse_move_y);
        }
        if (mouse_button_press)
        {
            Serial.println("Performing mouse button press:");
            if (mouse_button_press == MOUSE_LEFT) Serial.println("Left click");
        }

        mouse_move_x = 0;
        mouse_move_y = 0;
        mouse_v_wheel = 0;
        mouse_h_wheel = 0;
        mouse_button_press = 0;
    }
}

static inline void use_button(uint8_t button) {
    mouse_button_press = button;
}

static inline void readGyro(AS_LSM6DSOX& sox, const int chipSelect) {
  if (can_communicate()) {
    // Select the chip
    digitalWrite(chipSelect, LOW);

    sensors_event_t accel;
    sensors_event_t gyro;

    sox.getEvent(&accel, &gyro);

    Serial.print("\t\tGyro: \n");
    Serial.println("X:");
    Serial.print(gyro.gyro.x);
    Serial.println(" rad/s ");

    float rot_x = gyro.gyro.x * dt / 1000;
    x_radians += rot_x;
    mouse_move_x = rot_x;

    if (x_radians >= ROT_X_BOUND - ROT_X_CORRECT) {
        mouse_move_x = 10;
    }
    if (x_radians <= -ROT_X_BOUND + ROT_X_CORRECT) {
        mouse_move_x = -10;
    }

    Serial.println(x_radians);

    Serial.println("Y:");
    Serial.print(gyro.gyro.y);
    Serial.println(" rad/s ");

    float rot_y = gyro.gyro.y * dt / 1000;
    y_radians += rot_y;
    mouse_move_y = rot_y;

    if (y_radians >= ROT_Y_BOUND - ROT_Y_CORRECT) {
        y_radians = ROT_Y_BOUND;
        mouse_move_y = 10;
    }
    if (y_radians <= -ROT_Y_BOUND + ROT_Y_CORRECT) {
        y_radians = -ROT_Y_BOUND;
        mouse_move_y = -10;
    }

    Serial.println(y_radians);
    
    Serial.println("Z: ");
    Serial.print(gyro.gyro.z);
    Serial.println(" rad/s ");
    Serial.println();

    // Deselect the chip
    digitalWrite(chipSelect, HIGH);
  }
}


static inline void readAccel(AS_LSM6DSOX& sox, const int chipSelect) {
  Serial.println("Accelerometer not yet set up");
}


void setup() {
  Serial.begin(115200);

  pinMode(GYRO_MOUSE, INPUT);
  pinMode(ACCEL_MOUSE, INPUT);
  pinMode(BUTTON_PIN, INPUT);   

  bluetooth_setup();
  sensor_setup(sox1);
}
  

void loop() {
  advertize();

  // Use gyroscope to move the mouse
  if (digitalRead(GYRO_MOUSE) == HIGH) {
    // Zero the gyroscope reading
    if (digitalRead(BUTTON_PIN) == HIGH) {
        x_radians = 0.0f;
        y_radians = 0.0f;
    }

    digitalWrite(LSM_CS2, HIGH);
    readGyro(sox1, LSM_CS1);
  }
  // Use accelerometer to move the mouse
  else if (digitalRead(ACCEL_MOUSE) == HIGH) {
    digitalWrite(LSM_CS2, HIGH);
    readAccel(sox1, LSM_CS1);
  }

  if (checkForDoubleTap(sox1)) use_button(MOUSE_LEFT);
  
  flush();
  delay(dt);
}
