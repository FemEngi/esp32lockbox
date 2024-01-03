#ifndef OAUTHWEBSERVER_H
#define OAUTHWEBSERVER_H
#include "wifiincludes.h"

#define ARDUINOJSON_USE_ARDUINO_STRING 1

#include <ArduinoJson.h>


extern time_t unlocksolenoid;

time_t getNTPTime();
void handleMain(AsyncWebServerRequest *request);
void handleCallback(AsyncWebServerRequest *request);
void performApiCall();
bool refreshTokenIfNeeded();
void sendTimerUpdate(unsigned int days, unsigned int hours, unsigned int minutes, unsigned int seconds, bool isFrozen, bool isAllowedToViewTime, bool canBeUnlocked);

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
#endif