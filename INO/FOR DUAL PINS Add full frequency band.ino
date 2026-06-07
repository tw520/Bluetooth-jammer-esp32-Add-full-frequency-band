#include "RF24.h"
#include <SPI.h>
#include <ezButton.h>
#include "esp_bt.h"
#include "esp_wifi.h"

SPIClass *sp = nullptr;
SPIClass *hp = nullptr;

// 設定 SPI 速度為 16MHz (nRF24L01 硬體極限為 10MHz，但部分模組可超頻，若通訊不穩請降回 10000000)
RF24 radio(16, 15, 16000000);   // HSPI
RF24 radio1(22, 21, 16000000);  // VSPI

unsigned int flag = 0;   // HSPI 掃頻方向旗標
unsigned int flagv = 0;  // VSPI 掃頻方向旗標
int ch = 0;    // HSPI 當前頻道 (0~125)
int ch1 = 0;   // VSPI 當前頻道 (0~125)

ezButton toggleSwitch(33);

// 模式二：線性掃頻模式 (覆蓋 0~125 全頻段)
void two() {
  // VSPI 頻道掃頻邏輯
  if (flagv == 0) {
    ch1 += 4;
  } else {
    ch1 -= 4;
  }

  // VSPI 邊界檢查 (0 到 125)
  if (ch1 >= 125 && flagv == 0) {
    flagv = 1; 
  } else if (ch1 <= 0 && flagv == 1) {
    flagv = 0; 
  }

  // HSPI 頻道掃頻邏輯
  if (flag == 0) {
    ch += 2;
  } else {
    ch -= 2;
  }

  // HSPI 邊界檢查 (0 到 125)
  if (ch >= 125 && flag == 0) {
    flag = 1; 
  } else if (ch <= 0 && flag == 1) {
    flag = 0; 
  }

  // 應用頻道設定
  radio.setChannel(ch);
  radio1.setChannel(ch1);
}

// 模式一：隨機跳頻模式 (覆蓋 0~125 全頻段)
void one() {
  // random(126) 會產生 0 到 125 的隨機數
  radio1.setChannel(random(126));
  radio.setChannel(random(126));
  delayMicroseconds(random(60)); 
}

void setup() {
  Serial.begin(115200);
  
  // 關閉 ESP32 內建無線，避免干擾
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
  
  toggleSwitch.setDebounceTime(50);

  initHP();
  initSP();
}

void initSP() {
  sp = new SPIClass(VSPI);
  sp->begin();
  if (radio1.begin(sp)) {
    Serial.println("SP Started !!!");
    radio1.setAutoAck(false);
    radio1.stopListening();
    radio1.setRetries(0, 0);
    radio1.setPALevel(RF24_PA_MAX, true);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setCRCLength(RF24_CRC_DISABLED);
    radio1.printPrettyDetails();
    radio1.startConstCarrier(RF24_PA_MAX, ch1);
  } else {
    Serial.println("SP couldn't start !!!");
  }
}

void initHP() {
  hp = new SPIClass(HSPI);
  hp->begin();
  if (radio.begin(hp)) {
    Serial.println("HP Started !!!");
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, ch);
  } else {
    Serial.println("HP couldn't start !!!");
  }
}

void loop() {
  toggleSwitch.loop(); 
  int state = toggleSwitch.getState();

  if (state == HIGH) {
    two();
  } else {
    one();
  }
}
