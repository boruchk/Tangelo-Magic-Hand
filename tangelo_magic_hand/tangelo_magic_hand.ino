/* 
TODO:
- Figure out how to:
  - Make long connection interval and low latency when connecting or sending lots of data, Reduce when in normal running mode
  - Change advertising TX power and TX power while in a connection to increase battery life 
    Enable the LE Power Control feature if available
- Make a sleep function that for when device is inactive (if possible)
  - Make check in loop() that checks for 
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLEHIDDevice.h"
#include "BLESecurity.h"


#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BACK 8
#define MOUSE_FORWARD 16


BLEHIDDevice* tangelo;
BLECharacteristic* inputMouse;
BLE2902* notificationDescriptor;
BLEAdvertising* advertising;


uint8_t buttonPress = 0;
bool deviceConnected = false;
bool shouldStartAdvertising = false;


/* 
Reports sent by the esp32 follow this structure:
  Byte      Field             Description
  0         Buttons           5 bits for Left, Right, Middle, Back, Forward. Plus 3-bits for padding
  1         X Movement        -127 to +127
  2         Y Movement        -127 to +127
  3         Vertical scroll   -127 to +127
  4         Horizontal scroll -127 to +127
*/
uint8_t reportMap[] = {
  USAGE_PAGE(1),       0x01, // USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x02, // USAGE (Mouse)
  COLLECTION(1),       0x01, // COLLECTION (Application)
  USAGE(1),            0x01, //   USAGE (Pointer)
  COLLECTION(1),       0x00, //   COLLECTION (Physical)
  // ------------------------------------------------- Buttons (Left, Right, Middle, Back, Forward)
  // BYTE 0
  USAGE_PAGE(1),       0x09, //     USAGE_PAGE (Button)
  USAGE_MINIMUM(1),    0x01, //     USAGE_MINIMUM (Button 1)
  USAGE_MAXIMUM(1),    0x05, //     USAGE_MAXIMUM (Button 5)
  LOGICAL_MINIMUM(1),  0x00, //     LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1),  0x01, //     LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),      0x01, //     REPORT_SIZE (1)
  REPORT_COUNT(1),     0x05, //     REPORT_COUNT (5)
  HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;5 button bits
  // ------------------------------------------------- Padding
  REPORT_SIZE(1),      0x03, //     REPORT_SIZE (3)
  REPORT_COUNT(1),     0x01, //     REPORT_COUNT (1)
  HIDINPUT(1),         0x03, //     INPUT (Constant, Variable, Absolute) ;3 bit padding
  // ------------------------------------------------- X/Y position, Wheel
  // BYTE 1, 2, 3
  USAGE_PAGE(1),       0x01, //     USAGE_PAGE (Generic Desktop)
  USAGE(1),            0x30, //     USAGE (X)
  USAGE(1),            0x31, //     USAGE (Y)
  USAGE(1),            0x38, //     USAGE (Wheel)
  LOGICAL_MINIMUM(1),  0x81, //     LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f, //     LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08, //     REPORT_SIZE (8)
  REPORT_COUNT(1),     0x03, //     REPORT_COUNT (3)
  HIDINPUT(1),         0x06, //     INPUT (Data, Variable, Relative) ;3 bytes (X,Y,Wheel)
  // ------------------------------------------------- Horizontal wheel
  // BYTE 4
  USAGE_PAGE(1),       0x0c, //     USAGE PAGE (Consumer Devices)
  USAGE(2),      0x38, 0x02, //     USAGE (AC Pan)
  LOGICAL_MINIMUM(1),  0x81, //     LOGICAL_MINIMUM (-127)
  LOGICAL_MAXIMUM(1),  0x7f, //     LOGICAL_MAXIMUM (127)
  REPORT_SIZE(1),      0x08, //     REPORT_SIZE (8)
  REPORT_COUNT(1),     0x01, //     REPORT_COUNT (1)
  HIDINPUT(1),         0x06, //     INPUT (Data, Var, Rel)
  END_COLLECTION(0),         //   END_COLLECTION
  END_COLLECTION(0)          // END_COLLECTION
};


class MySecurityCallbacks: public BLESecurityCallbacks {
  bool onSecurityRequest() {
    Serial.println("onSecurityRequest");
    deviceConnected = true;
    return deviceConnected;
  };

  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
    if (auth_cmpl.success) {
      Serial.println("Pairing successful!");
    } else {
      Serial.println("Pairing failed!");
    }
  }

  // Following methods would only be used if we implement higher security that
  //  requires the user to enter a pin on the client side. I doubt we will 
  //  implement that (maybe if we install a screen on the device?)
  void onPassKeyNotify(uint32_t pass_key) {
    Serial.println("onPassKeyNotify: ");
    Serial.println(pass_key);
  }

  uint32_t onPassKeyRequest() {
    // Return the passkey (you can set it to a fixed value or dynamically)
    Serial.println("Passkey requested, returning key = 123456");
    return 123456;  // Return a fixed passkey for simplicity
  }

  bool onConfirmPIN(uint32_t pin) {
    Serial.println("Confirming pin");
    if (pin == 123456) {
      return true;
    }
  }
};

// Callback function that is called whenever a client is connected or disconnected
class serverCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;    
    // Stop advertising once we are connected to a client
    shouldStartAdvertising = false;
    advertising->stop();
    Serial.println("Tangelo stopped advertising");
    Serial.println("Device connected");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;    
    // Wait 2.5 seconds before advertising again to reconnect
    // Start advertising in case it disconnected by mistake
    // delay(2500);
    notificationDescriptor->setNotifications(false);
    shouldStartAdvertising = true;
    Serial.println("Device Disconnected!");
  }
};

void advertize() {
  if (shouldStartAdvertising) {
    shouldStartAdvertising = false;
    advertising->start();
    Serial.println("Tangelo is now advertising...");
  }
}


void move(signed char x, signed char y, signed char vWheel, signed char hWheel) {
  // Check if the client is connected and it wants to receive messages from us.
  // This also ensures that we only send data when the devices are paired and not just connected
  // We check if the client is ready with getNotifications()
  if (deviceConnected && notificationDescriptor->getNotifications()) {
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

void click(uint8_t button) {
  buttonPress = button;
  move(0,0,0,0);
  buttonPress = 0;
  move(0,0,0,0);
}


void setup() {
  Serial.begin(921600);

  // Initilize the BLE environment
  BLEDevice::init("Tangelo Magic Hand");
  // Set the security callbacks
  static MySecurityCallbacks secCB;
  BLEDevice::setSecurityCallbacks(&secCB);

  // Set the security level
  static BLESecurity bleSec;
  BLESecurity *pSecurity = &bleSec;
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);  // set to ESP_IO_CAP_OUT if we want to make users enter a unique pin to pair
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  // Use this insead of the above 3 lines if we want the users to enter a static pin
  // pSecurity->setStaticPIN(123456);
  

  // Create the BLE server
  BLEServer* pServer = BLEDevice::createServer();
  // Set the callbacks for when its connected/disconnected
  static serverCallbacks servCB;
  pServer->setCallbacks(&servCB);


  // Set up
  static BLEHIDDevice bleHidDev(pServer);
  tangelo = &bleHidDev;

  // Create an input report characteristic (for mouse movements: REPORT_ID = 0)
  // TODO change this for keyboard inputs?
  inputMouse = tangelo->inputReport(0);
  // Set up the notification descriptor (BLE2902*) for the inputMouse Charactaristic
  notificationDescriptor = (BLE2902*)inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));

  // Set up the HID descriptor for a mouse
  tangelo->manufacturer()->setValue("Adafruit-Espressif");
  tangelo->pnp(
    0x02,   // device_id = HIDdevice
    0x1234, // vendor_id = generic vendor ID-maybe apply for our own but costs $5000, 
    0x0001, // product_id = number 1 for first product?
    0x0100  // version = 1.0
  );
  tangelo->hidInfo(
    0x00, // country = 0x01, Country code for the United States (for North American keyboard layout)
    0x01  // flags = 0x01, can be used as a bootable HID device
  );
  tangelo->reportMap((uint8_t*)reportMap, sizeof(reportMap));

  // Start the BLEHID services (ask the services to start responding to incoming requests - handled by BLE____.h header files)
  tangelo->startServices();

  // Start advertising as a BLE HID device
  advertising = pServer->getAdvertising();
  advertising->setAppearance(HID_MOUSE);  // 0x0210 = generic HID device, 0x0201 = Mouse, 0x0200 = Keyboard
  advertising->addServiceUUID(tangelo->hidService()->getUUID());

  advertising->setMinInterval(0x20);  // Minimum connection interval (in units of 1.25ms) to 25ms
  advertising->setMaxInterval(0x40);  // Maximum connection interval (in units of 1.25ms) to 50ms

  shouldStartAdvertising = true;
}


signed char x=5, y=5, vWheel=0, hWheel=0;

void loop() {
  advertize();
  move(x, y, vWheel, hWheel);
  delay(100);
}