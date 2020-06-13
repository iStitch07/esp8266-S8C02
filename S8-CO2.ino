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

byte cmd_s8[7]    = {0xFE, 0x44, 0x00, 0x08, 0x02, 0x9F, 0x25};
byte response_s8[] = {0,0,0,0,0,0,0};

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

void s8Request(byte packet[], int num) { 
  s8Serial.begin(9600);
  while(!s8Serial.available()) {
    s8Serial.write(cmd_s8,num); 
    delay(50); 
  }
  int timeout=0;
  while(s8Serial.available() < num ) {
    timeout++; 
    if(timeout > 10) {
      while(s8Serial.available()) {
        s8Serial.read(); 
        break;
      }
    } 
    delay(50); 
  } 
  for (int i=0; i < num; i++) { 
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
  s8Request(cmd_s8, 7);
  s8_co2 = s8Replay(response_s8);
  
  if (!s8_co2_mean) s8_co2_mean = s8_co2;
  s8_co2_mean = s8_co2_mean - smoothing_factor*(s8_co2_mean - s8_co2);
  
  if (!s8_co2_mean2) s8_co2_mean2 = s8_co2;
  s8_co2_mean2 = s8_co2_mean2 - smoothing_factor2*(s8_co2_mean2 - s8_co2);

  Serial.printf("CO2 value: %d, M1Value: %d, M2Value: %d\n", s8_co2, s8_co2_mean, s8_co2_mean2);
  return;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  if(WiFi.status() != WL_CONNECTED) {
    wifi_reconnect();
  }
}

void loop() {
  co2_measure();
  delay(10 * 1000L);
}
