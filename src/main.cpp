#include <Wire.h>
#include <ArduinoJson.h>

#include <time.h>
#include <WiFiUdp.h>

#include "oauthwebserver.h"
#include "wifimanager.h"
#include "wifiincludes.h"
#include "img.h"

#if defined(ESP32)
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#include <WiFiClientSecure.h>
WiFiClient wifiClient;
#define servoPin 13
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>
#include <WiFiClientSecure.h>
WiFiClient wifiClient;
#define servoPin D1
#endif


const char pin1 = 12;
const char pin2 = 13;
const char pinsolenoid = 14;

const char *ssid = "";
const char *password = "";
const char *clientId = "locky-558750";
const char *clientSecret = "e3fd7301-3b8b-42f1-ac89-7d4d3d6bd9fb";
const char *redirectUri = "http://esplockbox.local/callback";
const char *oauthEndpoint = "https://sso.chaster.app/auth/realms/app/protocol/openid-connect/auth";
const char *tokenEndpoint = "https://sso.chaster.app/auth/realms/app/protocol/openid-connect/token";
const char *apiEndpoint = "https://api.chaster.app/locks";


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // Your GMT offset in seconds
const int daylightOffset_sec = 0; // Your daylight offset in seconds

const int bufferSize = 3072;

String lockStatus = "locked";
String lockID = "";
String endDateStr;
String openedAtStr;
bool isFrozen;
bool locked;
bool extensionsAllowUnlocking;
bool canBeUnlocked;
bool isAllowedToViewTime;
// bool jsonresponse = true;
// bool apiresponded=true;
// bool isAllowedToViewTimeNULL=true;

String imageData;

String refreshToken;
String accessToken;

const unsigned long tokenRefreshInterval = 300000;  // 300 seconds
unsigned long lastTokenRefreshTime = 0;
unsigned long apiCallInterval = 5000;

time_t timeInSeconds;
time_t durationLeft;
time_t currentTime;
time_t lastupdate;
time_t lastApiCallTime;
time_t lastApiCallTime1;
time_t lastApiCallTime2;

DNSServer dnsServer;
Preferences preferences;
Servo servo1;


AsyncWebSocket webSocket("/ws"); 
AsyncWebServer server(80);
AsyncEventSource events("/events");
HTTPClient httpClient;


time_t unlocksolenoid = 0;
bool jsonresponse = true;
bool apiresponded=true;
bool isAllowedToViewTimeNULL=true;

// LiquidCrystal_I2C lcd(0x27, 16, 2);




String apiResponse;







void setup() {
  #if defined(ESP32)
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
  #endif
  preferences.begin("wifi", true);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();

  Serial.begin(115200);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  uint8_t count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 5) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    count = count+1;
  }
  setupWiFiConfigAP();
  while (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  }

  Serial.println("Connected to WiFi");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // Set up mDNS
  if (MDNS.begin("esplockbox")) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS");
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/callback", HTTP_GET, handleCallback);
  server.on("/", HTTP_GET, handleMain);

  Serial.println("Starting server");
  server.begin();

  Serial.println("HTTP server started");

  getNTPTime();
  #if defined(ESP32)
  servo1.setPeriodHertz(50);
  #endif 
  servo1.attach(servoPin, 1000, 2000);
  // lcd.begin(16, 2);  // initialize the lcd
  // lcd.clear();       // clear the LCD screen
  servo1.write(10);
  server.addHandler(&webSocket);
  webSocket.onEvent(onWebSocketEvent);

  server.on("/", HTTP_GET, handleMain);
  MDNS.addService("http", "tcp", 80);



  if (!SPIFFS.begin(true)) {
    Serial.println("Error mounting SPIFFS");
    return;
  }



  pinMode(pin1, INPUT_PULLUP);
  pinMode(pin2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pin1), pinChangeInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin2), pinChangeInterrupt, CHANGE);
  pinChangeInterrupt();
  pinMode(pinsolenoid, OUTPUT);
  digitalWrite(pinsolenoid, HIGH);

}







void loop() {
  // Refresh token if needed
  

  
  // Check if it's time to perform the API call
  if (millis() - lastApiCallTime >= apiCallInterval) {
    if (refreshTokenIfNeeded()) {
    lastApiCallTime = millis();
    performApiCall();  // This will update timeInSeconds based on the API response
  }
  }
  // Move the servo

  // Optional: Print or handle time-related logic here

  unsigned int days = durationLeft / 86400;  // 86400 seconds in a day
  unsigned int hours = (durationLeft % 86400) / 3600;
  unsigned int minutes = (durationLeft % 3600) / 60;
  unsigned int seconds = durationLeft % 60;

  // Create a string to display
  if (millis() - lastApiCallTime1 >= 1000) {
    if (!isFrozen && durationLeft>0) {
        durationLeft = durationLeft - 1;
        Serial.println(durationLeft);
        checkupdate();
    }
    sendTimerUpdate(days, hours, minutes, seconds, isFrozen, !isAllowedToViewTime, canBeUnlocked);
    // Send the timer data via WebSocket
    String payload = "{\"days\": " + String(days) +
                    ", \"hours\": " + String(hours) +
                    ", \"minutes\": " + String(minutes) +
                    ", \"seconds\": " + String(seconds) +
                    ", \"isFrozen\": " + String(isFrozen ? "true" : "false") +
                    ", \"isAllowedToViewTime\": " + String(!isAllowedToViewTime ? "true" : "false") +
                    "}";
    webSocket.textAll(payload);

    lastApiCallTime1 = millis();
    Serial.println("Time left = " + String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s" + "    timer visible is set to " + String(!isAllowedToViewTime) + "       lock is  " + String(lockStatus));
  }

  if (millis() - unlocksolenoid >= 15000) {
    digitalWrite(pinsolenoid, HIGH);
  }
  // #if defined(ESP8266)
  // MDNS.update();
  // #endif
  // if (millis() - lastApiCallTime2 >= 10000) {
  //   lastApiCallTime2 = millis();
  //   imgupload("/000.png");
  //   Serial.print("upload");
  // }
}



