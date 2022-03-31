/*
   ESP32 based BLE beacon for liquid sensors
   See https://github.com/oh2mp/esp32_watersensor
   set CPU to 80Mhz!!!
*/

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "driver/adc.h"
#include "MAX17043.h"

#define SENS1 36 //36 for plant1 
#define SENS2 39
#define SENS3 35 //skipping 34 bcs unreliable readings
#define volt1 12
#define volt2 4
#define volt3 16
#define uS_TO_S_FACTOR 1000000ULL   /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3600        /* Time ESP32 will go to sleep (in seconds) */
#define MIN1 950 //completely in water //plant2 =1500 //plant1=1000
#define MAX1 2500 //completely dry //plant2 = 3600 //plant1 = 3100
#define MIN2 950
#define MAX2 2500
#define MIN3 950
#define MAX3 2500

int sensorbuff1;
int sensorbuff2;
int sensorbuff3;

byte sensor1;
byte sensor2;
byte sensor3;
byte battery;

char datastr[16];
char convtable[255];

BLEAdvertising *advertising;
std::string mfdata = "";

/* ----------------------------------------------------------------------------------
   Set up data packet for advertising
*/
void set_beacon() {
  BLEBeacon beacon = BLEBeacon();
  BLEAdvertisementData advdata = BLEAdvertisementData();
  BLEAdvertisementData scanresponse = BLEAdvertisementData();

  advdata.setFlags(0x06); // BR_EDR_NOT_SUPPORTED 0x04 & LE General discoverable 0x02

  mfdata = "";
  mfdata += (char)0xE5; mfdata += (char)0x02;  // Espressif Incorporated Vendor ID = 0x02E5
  mfdata += (char)(sensor1);         
  mfdata += (char)(sensor2);         
  mfdata += (char)(sensor3);                    // Raw (calculated) value from the sensor
  mfdata += (char)(battery);                   // Battery value if any
  mfdata += (char)0xBE; mfdata += (char)0xEF;  // Beef is always good nutriment

  advdata.setManufacturerData(mfdata);
  advertising->setAdvertisementData(advdata);
  advertising->setScanResponseData(scanresponse);
}


void setup() {

  pinMode(SENS1, INPUT);
  pinMode(SENS2, INPUT);
  pinMode(SENS3, INPUT);
  pinMode(volt1, OUTPUT);
  pinMode(volt2, OUTPUT);
  pinMode(volt3, OUTPUT);
  FuelGauge.begin();
  digitalWrite(SDA, 0); //disable double resistors https://github.com/porrey/max1704x/issues/1
  digitalWrite(SCL, 0);
  FuelGauge.quickstart();
  //Serial.begin(115200);
  BLEDevice::init("ESP32_plantmonitor");
  advertising = BLEDevice::getAdvertising();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}

/* ---------------------------------------------------------------------------------- */
void loop() {
  // Read sensor and calculate value
  digitalWrite(volt1, HIGH); //give power to sensor
  digitalWrite(volt2, HIGH);
  digitalWrite(volt3, HIGH);
  delay(500);
  sensorbuff1 = analogRead(SENS1);
  sensorbuff2 = analogRead(SENS2);
  sensorbuff3 = analogRead(SENS3);
  battery = round(FuelGauge.percent());
  digitalWrite(volt1, LOW); //cut power
  digitalWrite(volt2, LOW); //cut power
  digitalWrite(volt3, LOW); //cut power
  sensor1 = map(sensorbuff1, MIN1, MAX1, 99, 0); //map min and max values to 0-99
  sensor2 = map(sensorbuff2, MIN2, MAX2, 99, 0); //map min and max values to 0-99
  sensor3 = map(sensorbuff3, MIN3, MAX3, 99, 0); //map min and max values to 0-99
  //Serial.println(sensorbuff);
  //Serial.println(sensor);
  set_beacon();  //put variables into the beacon
  advertising->start();
  delay(5000); //advertise for 10sec
  advertising->stop();
  //esp_bluedroid_disable();
  //esp_bluedroid_deinit();
  btStop();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  adc_power_off(); //turn everything off
  esp_deep_sleep_start();
}

/* ---------------------------------------------------------------------------------- */
