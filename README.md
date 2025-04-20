# Tangelo-Magic-Hand

## Development Set-Up

- Download this repo to your local machine
- It is recommended to use Arduino for the ESP32 Feather Based on recommendations from [Adafruit website](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide)
- You can use the instructions from the [Espressif website](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)

## Compilation

- Partition Scheme
  - It's recommended to use one of the `No OTA` options (under `Tools`) to avoid having libraries bottlenecked by programmable storage space; statistics are provided below.
    - Using `No OTA (2MB APP/2MB SPIFFS)`, 54% of programmable storage space.
    - Using `Default 4MB with spiffs (1.2MB APP/1.5 SPIFFS)`, 87% of programmable space.

## BLE Technical information (some adopted from [ESP BLE Guide](https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf))

While our project implements the [arduino-esp32](https://github.com/espressif/arduino-esp32) library with high level code, it is based on the more low level [ESP-idf repo](https://github.com/espressif/esp-idf).

We optimized the BLE header files by cutting down on the features that are not used in our device including (...)

### Put optimized files in the corresponding folders in the "libraries" directory

We create the server, we create the service, create a haracteristic upon the service,\\
set a value for the characteristic and then ask the service to start responding to incoming requests.

When a BLE Server is running, what must happen next is that peer devices (clients) must be able to\\
locate it. This is made possible through the notion of advertising. The BLE Server can broadcast its\\
existence along with sufficient information to allow a client to know what services it can provide.

Once we have started the BLE Server, we can ask it for an object (BLEAdvertising) that owns the\\
advertisements that the server produces:

```c++
BLEAdvertising* pAdvertising = pServer->getAdverstising();
pAdvertising->start();
```

Once performed, the server can be dynamically found by the clients. Of course a server doesn't need to\\
advertise. If a client should otherwise be informed (or remember) the address of the BLE server, it can\\
request a connection at any time.

Notify is used to signal (or notify) to the client that the characteristic's value has changed.\\
The client will receive an indication event to let it know that the change has occurred.\\
(indicate() receives a confirmation while a notify() does not receive a confirmation.)

```c++
pCharacteristic->setValue("HighTemp");
pCharacteristic->notify();
```

Associated with the idea of indications/notifications, is the architected BLE Descriptor called "Client
Characteristic Configuration" which has UUID 0x2902. This contains (among other things) two distinct
bit fields that can be on or off. One bit field governs Notifications while the other governs Indications.
If the corresponding bit is on, then the server can/may send the corresponding push.

For example if the Notifications bit is on, then the server can/may send notifications. The primary purpose of the
descriptor is to allow a partner (a client) to request that the server actually send notifications or
indications.

## Communication Protocol

We chose to use the SPI protocol over I2C as it offers higher-speed speed and lower-latency communication, ideal for our application.

We optimized the SPI header files by cutting down on the features that are not used in our device including tempurature sensing and the pedometer.

## Technical Docs

[adafruit-huzzah32-esp32-feather](https://cdn-learn.adafruit.com/downloads/pdf/adafruit-huzzah32-esp32-feather.pdf)

[esp32_technical_reference_manual_en](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

[lsm6dsox-and-ism330dhc-6-dof-imu](https://cdn-learn.adafruit.com/downloads/pdf/lsm6dsox-and-ism330dhc-6-dof-imu.pdf)

[lsm6dsox Datasheet](https://www.st.com/resource/en/datasheet/lsm6dsox.pdf)

[ESP BLE Guide](https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf)

[GATT Security Server Walkthrough](https://github.com/espressif/esp-idf/blob/master/examples/bluetooth/bluedroid/ble/gatt_security_server/tutorial/Gatt_Security_Server_Example_Walkthrough.md)
