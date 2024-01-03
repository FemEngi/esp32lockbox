#ifndef wifiincludes_H_
#define wifiincludes_H_

#include <Preferences.h>
#if defined(ESP32)
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
// #include <WiFiClient.h>
// WiFiClient wifiClient;
extern ESP32PWM pwm;
#define servoPin 13
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>
#include <WiFiClient.h>
extern WiFiClient wifiClient;
#define servoPin D1
#endif

extern String lockID;
extern String lockStatus;
extern String endDateStr;
extern String openedAtStr;
extern bool isFrozen;
extern bool locked;
extern bool extensionsAllowUnlocking;
extern bool canBeUnlocked;
extern bool isAllowedToViewTime;
extern bool jsonresponse;
extern bool apiresponded;
extern bool isAllowedToViewTimeNULL;

extern time_t timeInSeconds;
extern time_t durationLeft;
extern time_t currentTime;
extern time_t lastupdate;
extern time_t lastApiCallTime;
extern time_t lastApiCallTime1;

extern const char *ssid;
extern const char *password;
extern const char *clientId;
extern const char *clientSecret;
extern const char *redirectUri;
extern const char *oauthEndpoint;
extern const char *tokenEndpoint;
extern const char *apiEndpoint;
extern String verificationEndpoint;
extern String verificationId;


extern const char* ntpServer;
extern const long gmtOffset_sec;
extern const int daylightOffset_sec;

extern String refreshToken;
extern String accessToken;

extern const unsigned long tokenRefreshInterval;
extern unsigned long lastTokenRefreshTime;
extern unsigned long apiCallInterval;

extern DNSServer dnsServer;
extern Preferences preferences;
extern Servo servo1;
extern AsyncWebSocket webSocket;
extern AsyncWebServer server;
extern AsyncEventSource events;

extern const char pinsolenoid;

extern const int bufferSize;

#endif