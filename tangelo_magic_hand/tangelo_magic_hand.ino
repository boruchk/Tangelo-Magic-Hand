#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLEHIDDevice.h"
#include "BLESecurity.h"
#include "sdkconfig.h"

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
#define MOUSE_BACK 8
#define MOUSE_FORWARD 16


BLEHIDDevice* tangelo;
BLECharacteristic* inputMouse;
BLEAdvertising* advertising;

uint8_t buttonPress = 0;
bool deviceConnected = false;


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
  uint32_t onPassKeyRequest() {
    // Return the passkey (you can set it to a fixed value or dynamically)
    Serial.println("Passkey requested, returning key = 123456");
    return 123456;  // Return a fixed passkey for simplicity
  }

  void onPassKeyNotify(uint32_t pass_key) {
    Serial.println("onPassKeyNotify: ");
    Serial.println(pass_key);
  }

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
    BLE2902* desc = (BLE2902*)inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
    
    Serial.println("Device connected");
    advertising->stop();
    Serial.println("Tangelo stopped advertising");
  };

  void onDisconnect(BLEServer* pServer) {
    // start advertising in case it disconnected by mistake (or maybe wait for a bit to prevent going into connect/disconnect loop?)
    deviceConnected = false;
    BLE2902* desc = (BLE2902*)inputMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(false);

    Serial.println("Device Disconnected!");
    // Wait 2.5 seconds before advertising again to reconnect
    delay(2500);
    advertising->start();
    Serial.println("Tangelo is now advertising...");
  }
};


void move(signed char x, signed char y, signed char vWheel, signed char hWheel) {
  if (deviceConnected) {
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
  Serial.begin(115200);
  BLEDevice::init("Tangelo Magic Hand");

  // Set the security level
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new serverCallbacks());

  tangelo = new BLEHIDDevice(pServer);

  // Create an input report characteristic (for mouse movements: REPORT_ID = 0)
  inputMouse = tangelo->inputReport(0);

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

  // Start the HID service
  tangelo->startServices();

  // Start advertising as a BLE HID device
  BLEAdvertising* advertising = pServer->getAdvertising();
  advertising->setAppearance(HID_MOUSE);  // 0x0210 = generic HID device, 0x0201 = Mouse, 0x0200 = Keyboard
  advertising->addServiceUUID(tangelo->hidService()->getUUID());
  advertising->start();

  Serial.println("Tangelo is now advertising...");
}


signed char x=5, y=5, vWheel=0, hWheel=0;

void loop() {
  move(x, y, vWheel, hWheel);
  delay(100);
}