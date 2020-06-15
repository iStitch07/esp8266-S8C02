#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#include <arduino_secrets.h>
#include "arduino_secrets.h"

const char* ssid          = SECRET_SMARTHOME_WIFI_SSID;
const char* password      = SECRET_SMARTHOME_WIFI_PASSWORD;

#define D7 (13)
#define D8 (15)

SoftwareSerial s8Serial(D7, D8);

int s8_co2;
int s8_co2_mean;
int s8_co2_mean2;

float smoothing_factor = 0.5;
float smoothing_factor2 = 0.15;

byte cmd_s8[]       = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};
byte abc_s8[]       = {0xFE, 0x03, 0x00, 0x1F, 0x00, 0x01, 0xA1, 0xC3};
byte response_s8[7] = {0, 0, 0, 0, 0, 0, 0};

const int r_len = 7;
const int c_len = 8;

boolean wifi_reconnect() {
  Serial.printf("\nConnecting to %s ", ssid);
  WiFi.hostname("TestUnit");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\nConnected to the WiFi network: %s\n", ssid);
  return true;
}

void s8Request(byte cmd[]) {
  s8Serial.begin(9600);
  while(!s8Serial.available()) {
    s8Serial.write(cmd, c_len); 
    delay(50); 
  }
  int timeout=0;
  while(s8Serial.available() < r_len ) {
    timeout++; 
    if(timeout > 10) {
      while(s8Serial.available()) {
        s8Serial.read();
        break;
      }
    } 
    delay(50); 
  } 
  for (int i=0; i < r_len; i++) { 
    response_s8[i] = s8Serial.read(); 
  }
  
  s8Serial.end();
}                     

unsigned long s8Replay(byte rc_data[]) { 
  int high = rc_data[3];
  int low = rc_data[4];
  unsigned long val = high*256 + low;
  return val; 
}

void co2_measure() {
  s8Request(cmd_s8);
  s8_co2 = s8Replay(response_s8);
  
  if (!s8_co2_mean) s8_co2_mean = s8_co2;
  s8_co2_mean = s8_co2_mean - smoothing_factor*(s8_co2_mean - s8_co2);
  
  if (!s8_co2_mean2) s8_co2_mean2 = s8_co2;
  s8_co2_mean2 = s8_co2_mean2 - smoothing_factor2*(s8_co2_mean2 - s8_co2);

  Serial.printf("CO2 value: %d, M1Value: %d, M2Value: %d\n", s8_co2, s8_co2_mean, s8_co2_mean2);
  return;
}

void get_abc() {
  int abc_s8_time;
  s8Request(abc_s8);
  abc_s8_time = s8Replay(response_s8);

  Serial.printf("ABC Time is: %d hours\n", abc_s8_time);
  return;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  if(WiFi.status() != WL_CONNECTED) {
    wifi_reconnect();
  }

  get_abc();
}

void loop() {
  co2_measure();
  delay(10 * 1000L);
}
