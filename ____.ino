#include <LiquidCrystal.h>  //lcd
#include <analogWrite.h>    //esp32 pwn
#include <pitches.h>
#include <Tone32.h>
#include <ESP32_Servo.h>
#include <BLEDevice.h> 
#include <String.h>

#include <BLEServer.h> 
#include <BLEUtils.h> 
#include <BLE2902.h> 
#include <SPI.h>
#include <MFRC522.h> //rc522

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t txValue = 0;
long lastMsg = 0;
String rxload;
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
  };
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        rxload="";
        for (int i = 0; i < rxValue.length(); i++){
          rxload +=(char)rxValue[i];
        }
      }
    }
  };

#define SS_PIN 5
#define RST_PIN 0
const int ipaddress[4] = {103, 97, 67, 25};
byte nuidPICC[4] = {0, 0, 0, 0};
MFRC522::MIFARE_Key key;
MFRC522 rfid = MFRC522(SS_PIN, RST_PIN);
const int TONE_PWM_CHANNEL = 0; 
LiquidCrystal lcd(27, 26, 25, 33, 32 , 22);
void setupBLE(String BLEName){
  const char *ble_name=BLEName.c_str();
  BLEDevice::init(ble_name);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID); 
  pCharacteristic= pService->createCharacteristic(CHARACTERISTIC_UUID_TX,BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks()); 
  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  }
  
void setup() {
  Serial.begin(115200);
  setupBLE("RFID-RC522");
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_DumpVersionToSerial();
  ledcAttachPin(4, TONE_PWM_CHANNEL);
  lcd.begin(16, 2);          // 設定LCD字幕為 16*2
  lcd.home();             // 將游標移至左上角
  lcd.print("identifying.....");  // 顯示字串
}

void loop() {
  analogWrite(13, 0);
  analogWrite(12, 0);
  analogWrite(14, 0);
  readRFID();
  long now = millis();
  if (now - lastMsg > 100) {
    if (deviceConnected&&rxload.length()>0) {
      Serial.println(rxload);
      if (rxload=="64"){
        ledcWriteTone(TONE_PWM_CHANNEL, 350);
        analogWrite(13, 50);
        analogWrite(12, 255);
        analogWrite(14, 0);
        lcd.setCursor(0,1);
        lcd.print(rxload);
        lcd.print(" pass   ");
        delay(500);
      }
      else if (rxload!="64"){
        ledcWriteTone(TONE_PWM_CHANNEL, 1000);
        delay(340);
        ledcWriteTone(TONE_PWM_CHANNEL, 1000);
        delay(340);
        analogWrite(13, 255);
        analogWrite(12, 0);
        analogWrite(14, 0);
        lcd.setCursor(0,1);
        lcd.print(rxload);         // 移動游標至第0行第1列
        lcd.print(" fail     ");
        delay(500);
      }
      rxload="";
    }
    if(Serial.available()>0){
      String str=Serial.readString();
      const char *newValue=str.c_str();
      pCharacteristic->setValue(newValue);
      pCharacteristic->notify();
    }
    lastMsg = now;
  }
}

void readRFID(void ) { 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if (  !rfid.PICC_ReadCardSerial())
    return;
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  printDec(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void printDec(byte *buffer, byte bufferSize) {
  byte i;
  if (buffer[i]==64){
    Serial.println(rxload);
    ledcWriteTone(TONE_PWM_CHANNEL, 350);
    analogWrite(13, 50);
    analogWrite(12, 255);
    analogWrite(14, 0);
    lcd.setCursor(0,1); 
    lcd.print(buffer[i]);// 移動游標至第0行第1列
    lcd.print(" pass");
    delay(500);
  }
  else if (buffer[i]!=64){
    ledcWriteTone(TONE_PWM_CHANNEL, 1000);
    delay(340);
    ledcWriteTone(TONE_PWM_CHANNEL, 1000);
    delay(340);
    analogWrite(13, 255);
    analogWrite(12, 0);
    analogWrite(14, 0);
    lcd.setCursor(0,1); 
    lcd.print(buffer[i]);// 移動游標至第0行第1列
    lcd.print(" fail");
    delay(500);
  }
}
