/*
https://embeddedcentric.com/lesson-2-ble-profiles-services-characteristics-device-roles-and-network-topology/
https://punchthrough.com/manage-ble-connection/

https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLEServer.h

  BLE sends data as messages called a charactaristic (BLECharacteristic)
  Each charactaristic optionally has a descriptor (BLE2902) with more info about the charactaristic
  
  Breakdown of BLE Hierarchy:
    Server = this device, which contains (allows for discovery) one or more services
    Service = Contains related characteristics.
      HID Service (0x1812)
    Characteristic = A data point which contains declaration and value attributes.
    Value = The actual data
    Declaration = Metadata about the actual data:
    - Characteristic value UUID, UUID = type of charactaristic could be either a standard Bluetooth-SIG defined UUID (16-bits) or a vendor specific UUID (128-bits)
    - Characteristic value handle = client needs this to grab the data (like a data address)
    - Characteristic properties = Lists the GATT operation(s) allowed on the characteristic value:
      ~ Broadcast
      ~ Read = client can read
      ~ Write with no responce = No acknowledgment is sent from the server (saves power but maybe not reliable)
      ~ Write = expects ack
      ~ Notify = server sends data (no ack)
      ~ Indicate = server sends data (yes ack)
      ~ Authenticated Signed Write = allows the server to verify the authenticity “signature” of the data and confirm its source (requires paring and bonding)
    Descriptor = Metadata that provides extra information about the characteristic.
      The second use for the descriptor, which is the most commonly used is to use it as a switch to turn ON/OFF indication and notification (server-initiated updates). This type of descriptors is known as Client Characteristic Configuration Descriptor(CCCD).

  Device sends advertisements to PC (probably with UUID) up to 64 bytes total
  Device listens for PC Scan or Connection Request
  Device responds to a Scan Request with a Scan Response (another advert)


  Advertisement:
  - advert interval: use a fast interval (under a few hundred milliseconds) for a short duration (e.g. 30 seconds) at Peripheral power-up, 
    upon disconnection, or upon a user event. After 30 or more seconds, reduce the advertising interval
  - peripheral latency: 
  - supervision timeout: 
  Most stacks use a default connection interval of under 100 milliseconds and a peripheral latency of 0
  Make long connection interval and low latency when connecting or sending lots of data, Reduce when in normal running mode

  Transmit power: Change advertising TX power and TX power while in a connection to increase battery life
    Enable the LE Power Control feature if available.

  If sleeping is not be viable, use longer advertising intervals and/or connection parameters

  Use LE Privacy in which the Peripheral’s advertisement address changes at a configurable interval

  Bonding or application-level authentication can prevent unauthenticated devices from exchanging data with your device

  Filter Accept List: The Peripheral adds the Central to its Filter Accept List after making a connection for the first time
    prevents unauthorized centrals to connect

*/


#include <BLEDevice.h>
#include <BLEHIDDevice.h>


BLEHIDDevice* tangelo;
BLECharacteristic* inputReport;
BLEAdvertising* advertising;

bool deviceConnected = false;

/* 
See report structure documentation here:
https://usb.org/document-library/hid-usage-tables-16

Reports sent by the esp32 follow this structure:
  Byte      Field         Description
  0         Report ID     0x01 (Identifies Mouse)
  1         Buttons       3 bits for Left, Right, Middle, 5-bit padding
  2         X Movement    -127 to +127
  3         Y Movement    -127 to +127
  4         Scroll Wheel  -127 to +127 (+1 to +127 = Scroll up, -1 to -127 = Scroll down)
*/
uint8_t reportMap[] = {
  // **Mouse Report**
  USAGE_PAGE(1), 0x01,        // Generic Desktop Controls
  USAGE(1), 0x02,             // Mouse
  COLLECTION(1), 0x01,        // The following Application Collection is for the above USAGE (mouse)
    REPORT_ID(1), 0x01,       // Report ID 1: Mouse
    USAGE(1), 0x01,           // Pointer (as in a mouse pointer)
    COLLECTION(1), 0x00,      // Physical Collection of input data
      // Button Page //
      USAGE_PAGE(1), 0x09,    
      USAGE_MINIMUM(1), 0x01, // Minimum button index = 1 (First button).
      USAGE_MAXIMUM(1), 0x03, // Maximum button index = 3 (Left, Right, Middle buttons).
      LOGICAL_MINIMUM(1), 0x00, // Button states can be 0 (not pressed) or 1 (pressed).
      LOGICAL_MAXIMUM(1), 0x01, // Maximum state = 1 (pressed)
      REPORT_SIZE(1), 0x01,   // Each button state is 1 bit
      REPORT_COUNT(1), 0x03,  // Three buttons (Left, Right, Middle)
      // INPUT(1), 0x02,         // Buttons: Data, Variable, Absolute
      0x81, 0x02,
      REPORT_SIZE(1), 0x05,   // Padding size is 5 bits (since data is only 3 bits)
      REPORT_COUNT(1), 0x01,  // Only 1 padding field.
      // INPUT(1), 0x01,         // Padding
      0x81, 0x01,
      // Movement Page //
      USAGE_PAGE(1), 0x01,    
      USAGE(1), 0x30,         // X Axis
      USAGE(1), 0x31,         // Y Axis
      LOGICAL_MINIMUM(1), 0x81, // -127
      LOGICAL_MAXIMUM(1), 0x7F, // 127
      REPORT_SIZE(1), 0x08,
      REPORT_COUNT(1), 0x02,
      // INPUT(1), 0x06,         // Data, Variable, Relative
      0x81, 0x06,
      // Scroll Wheel Page //
      USAGE(1), 0x38,         
      LOGICAL_MINIMUM(1), 0x81, // -127
      LOGICAL_MAXIMUM(1), 0x7F, // 127
      REPORT_SIZE(1), 0x08,
      REPORT_COUNT(1), 0x01,
      // INPUT(1), 0x06,         // Data, Variable, Relative
      0x81, 0x06,
    END_COLLECTION(0),
  END_COLLECTION(0),

  // **Keyboard Report**
  USAGE_PAGE(1), 0x01,        // Generic Desktop
  USAGE(1), 0x06,             // Keyboard
  COLLECTION(1), 0x01,        // Application Collection
    REPORT_ID(1), 0x02,     // Report ID 2: Keyboard
    USAGE_PAGE(1), 0x07,    // Keyboard Page
    USAGE_MINIMUM(1), 0xE0, // Left Control
    USAGE_MAXIMUM(1), 0xE7, // Right GUI
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x01,
    REPORT_SIZE(1), 0x01,
    REPORT_COUNT(1), 0x08,  // 8 modifier keys (Ctrl, Shift, Alt, GUI)
    // INPUT(1), 0x02,         // Data, Variable, Absolute
    0x81, 0x02,
    REPORT_SIZE(1), 0x08,
    REPORT_COUNT(1), 0x01,
    // INPUT(1), 0x01,         // Padding
    0x81, 0x01,
    REPORT_SIZE(1), 0x08,
    REPORT_COUNT(1), 0x06,  // 6 Keycodes
    LOGICAL_MINIMUM(1), 0x00,
    LOGICAL_MAXIMUM(1), 0x65, // Max keycode (standard HID table)
    USAGE_MINIMUM(1), 0x00,
    USAGE_MAXIMUM(1), 0x65,
    // INPUT(1), 0x00,         // Data, Array
    0x81, 0x00,
  END_COLLECTION(0)
};


class MySecurityCallbacks: public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() {
    // Return the passkey (you can set it to a fixed value or dynamically)
    Serial.println("Passkey requested, returning key = 123456");
    return 123456;  // Return a fixed passkey for simplicity
  }

  void onPassKeyNotify(uint32_t pass_key) {}

  bool onSecurityRequest() {
    deviceConnected = true;
    return false;
  };

  void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) {
    if (auth_cmpl.success) {
      Serial.println("Pairing successful!");
    } else {
      Serial.println("Pairing failed!");
    }
  }

  bool onConfirmPIN(uint32_t pin) {
    if (pin == 1223456) return true;
  }

};

// Callback function that is called whenever a client is connected or disconnected
class serverCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    // maybe stop advertising after some time
    // do something with the accelerometer?
    deviceConnected = true;
    advertising->stop();
  };

  void onDisconnect(BLEServer* pServer) {
    // start advertising in case it disconnected by mistake (or maybe wait for a bit to prevent going into connect/disconnect loop?)
    // wait and go to sleep?
    deviceConnected = false;
    delay(2500);
    advertising->start();
  }
};


void setup() {
  Serial.begin(921600);
  BLEDevice::init("Tangelo Magic Hand");

  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new serverCallbacks());

  // Set the security level (optional, but you may want to set it to something like MITM protection)
  BLESecurity *pSecurity = new BLESecurity();
  // Require bonding (+ man-in-the-middle security) and pairing so that devices remember each other and won't require re-pairing each time they reconnect as well as encrypted communication
  // pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND | ESP_LE_AUTH_MITM);
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setKeySize(16);  // Set encryption key size
  pSecurity->setInitEncryptionKey(true);  // Automatically start encryption


  tangelo = new BLEHIDDevice(pServer);

  // Create an input report characteristic (for mouse movements and keyboard macros)
  inputReport = tangelo->inputReport(*reportMap);

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
  tangelo->reportMap(reportMap, sizeof(reportMap));

  // Start the HID service
  tangelo->startServices();

  // Start advertising as a BLE HID device
  advertising = BLEDevice::getAdvertising();
  advertising->setAppearance(0x0201);  // 0x0210 = generic HID device, 0x0201 = Mouse, 0x0200 = Keyboard
  advertising->addServiceUUID(tangelo->hidService()->getUUID());
  advertising->start();

  Serial.println("Tangelo is now advertising...");
}

void loop() {
  if (deviceConnected) {
    Serial.println("Moving mouse...");
    uint8_t mouseMove[] = {0x00, 10, 0};  // Move cursor right by 10 pixels
    inputReport->setValue(mouseMove, sizeof(mouseMove));
    inputReport->notify();
    delay(1000);
  }
}