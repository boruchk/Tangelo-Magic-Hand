#ifndef BLUETOOTH_SETUP_H
#define BLUETOOTH_SETUP_H

#include "BLEDevice.h"    // 
#include "BLEServer.h"    // 
#include "BLEHIDDevice.h" //
#include "BLESecurity.h"  // small enough, no need to cut down


extern BLEHIDDevice* tangelo;
extern BLECharacteristic* inputMouse;
extern BLE2902* notificationDescriptor;
extern BLEAdvertising* advertising;


void advertize();
bool can_communicate();
void bluetooth_setup();


#endif