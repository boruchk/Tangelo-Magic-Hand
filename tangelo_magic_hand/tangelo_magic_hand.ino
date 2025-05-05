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


#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BACK 8
#define MOUSE_FORWARD 16

#define ACCEL_MOUSE 33
#define GYRO_MOUSE 15
#define BUTTON_PIN 32


// LSM6DSOX chip number 1
// Rename to "mouse", "Button", etc when finishing
AS_LSM6DSOX sox1;
AS_LSM6DSOX sox2;


signed char MOVE_x=0, MOVE_y=0, vWheel=0, hWheel=0;
uint8_t buttonPress = 0;

float x_degrees = 0.0f;
float y_degrees = 0.0f;

float previous_time = 0.0f;
float cur_time = 0.0f;

int dt = 10;  // polling frequency in ms 


static inline void move(signed char x, signed char y, signed char vWheel, signed char hWheel) {
  // Check if the client is connected and it wants to receive messages from us.
  // This also ensures that we only send data when the devices are paired and not just connected
  // We check if the client is ready with getNotifications()
  if (can_communicate()) {
    uint8_t mouseMove[5];
    mouseMove[0] = buttonPress;  // Button
    mouseMove[1] = x;            // Move in X
    mouseMove[2] = y;            // Move in Y
    mouseMove[3] = vWheel;       // Vertical scroll
    mouseMove[4] = hWheel;       // Horizontal scroll

    inputMouse->setValue(mouseMove, sizeof(mouseMove));
    inputMouse->notify();

    Serial.println("Moving mouse...");
    Serial.print("X = ");
    Serial.println(mouseMove[1]);
    Serial.print("Y = ");
    Serial.println(mouseMove[2]);
  }
}

static inline void click(uint8_t button) {
  buttonPress = button;
  move(0,0,0,0);
  buttonPress = 0;
  move(0,0,0,0);
}


static inline void readGyro(AS_LSM6DSOX& sox, const int chipSelect) {
  if (can_communicate()) {
    // Select the chip
    digitalWrite(chipSelect, LOW);

    sensors_event_t accel;
    sensors_event_t gyro;

    sox.getEvent(&accel, &gyro);

    float time = (float) millis() / 1000;
    float delta_time = time - previous_time;
    previous_time = time;


    Serial.print("x_degrees");
    Serial.print(x_degrees);

    Serial.print("\t\tGyro: \n");
    Serial.print("X: ");
    Serial.print(gyro.gyro.x);

    char temp_x = gyro.gyro.x;
    x_degrees += gyro.gyro.x * delta_time;
    MOVE_x = temp_x;

    float x_bound = PI / 4;
    float y_bound = PI / 6;
    float x_corrective_factor = 0.035;
    float y_corrective_factor = 0.02;

    if (x_degrees >= x_bound - x_corrective_factor) {
        x_degrees = x_bound;
        MOVE_x=10;
    }
    if (x_degrees <= -x_bound + x_corrective_factor) {
        x_degrees = -x_bound;
        MOVE_x=-10;
    }


    Serial.print(" Y: ");
    Serial.print(gyro.gyro.y);
    char temp_y = gyro.gyro.y;
    y_degrees += gyro.gyro.y * delta_time;
    MOVE_y = temp_y;

    if (y_degrees >= y_bound - y_corrective_factor) {
        y_degrees = y_bound;
        MOVE_y=10;
    }
    if (y_degrees <= -y_bound + y_corrective_factor) {
        y_degrees = -y_bound;
        MOVE_y=-10;
    }

    Serial.print(" ");
    Serial.print(y_degrees);

    
    Serial.print(" Z: ");
    Serial.print(gyro.gyro.z);
    Serial.println(" radians/s ");
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
        x_degrees = 0.0f;
        y_degrees = 0.0f;
    }

    digitalWrite(LSM_CS2, HIGH);
    readGyro(sox1, LSM_CS1);
  }
  // Use accelerometer to move the mouse
  else if (digitalRead(ACCEL_MOUSE) == HIGH) {
    digitalWrite(LSM_CS2, HIGH);
    readAccel(sox1, LSM_CS1);
  }

  
  move(MOVE_x, MOVE_y, vWheel, hWheel);
  MOVE_x = 0;
  MOVE_y = 0;


  if (checkForDoubleTap(sox1)) click(MOUSE_LEFT);

  delay(10);
}