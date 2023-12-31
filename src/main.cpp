#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <time.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>


const char *ssid = "";
const char *password = "";
const char *clientId = "";
const char *clientSecret = "";
const char *redirectUri = "http://esp8266.local/callback";
const char *oauthEndpoint = "https://sso.chaster.app/auth/realms/app/protocol/openid-connect/auth";
const char *tokenEndpoint = "https://sso.chaster.app/auth/realms/app/protocol/openid-connect/token";
const char *apiEndpoint = "https://api.chaster.app/locks";


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // Your GMT offset in seconds
const int daylightOffset_sec = 0; // Your daylight offset in seconds


String lockStatus = "locked";
String endDateStr;
String openedAtStr;
bool isFrozen;
bool locked;
bool extensionsAllowUnlocking;
bool canBeUnlocked;
bool isAllowedToViewTime;
bool jsonresponse = true;
bool apiresponded=true;
bool isAllowedToViewTimeNULL=true;

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

const char *apSSID = "ESP8266-Config";
const char *apPassword = "";
const int apChannel = 1;
const int apTimeout = 300;

Preferences preferences;
DNSServer dnsServer;
ESP32PWM pwm;
Servo servo1;
AsyncWebSocket webSocket("/ws"); 
AsyncWebServer server(80);
AsyncEventSource events("/events");


std::vector<String> availableSSIDs;

// LiquidCrystal_I2C lcd(0x27, 16, 2);

static const int servoPin = 13;


#define bufferSize 3072

String apiResponse;







const char* htmlTemplate = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chastity Timer</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #111;
            color: #fff;
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            text-align: center;
        }

        button {
            padding: 10px;
            font-size: 16px;
            background-color: #4CAF50;
            color: #fff;
            border: none;
            cursor: pointer;
            margin-top: 20px;
        }

        button:hover {
            background-color: #45a049;
        }

        .timer {
            font-size: 48px; /* Larger font size */
            margin-bottom: 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .timer span {
            color: #4CAF50; /* Highlighted characters color */
            font-weight: bold; /* Bold text for highlighted characters */
            font-size: 1.5em; /* Larger font size for highlighted characters */
            margin: 0 5px; /* Margin between each number */
        }

        .ready-to-unlock {
            color: #FFD700; /* Golden color for "Ready to Unlock" message */
            font-size: 1.5em; /* Larger font size */
            margin-top: 10px; /* Top margin */
        }

        .hidden {
            display: none;
        }

        .frozen {
            color: red;
        }
    </style>
</head>
<body>
    <div class="timer" id="timer">
        <span id="days">00</span>:
        <span id="hours">00</span>:
        <span id="minutes">00</span>:
        <span id="seconds">00</span>
    </div>
    <div class="ready-to-unlock hidden" id="readyToUnlock">Ready to Unlock</div>
<script>
    var socket = new WebSocket("ws://" + window.location.host + "/ws");
    function sendTimerUpdate(days, hours, minutes, seconds, isFrozen, isAllowedToViewTime, canBeUnlocked) {
        var timerElement = document.getElementById("timer");
        var daysElement = document.getElementById("days");
        var hoursElement = document.getElementById("hours");
        var minutesElement = document.getElementById("minutes");
        var secondsElement = document.getElementById("seconds");
        var readyToUnlockElement = document.getElementById("readyToUnlock");

        if (isAllowedToViewTime) {
            timerElement.innerHTML = "Hidden";
            readyToUnlockElement.classList.add("hidden");
        } else {
            // Update the HTML with highlighted characters
            daysElement.innerHTML = days;
            hoursElement.innerHTML = hours;
            minutesElement.innerHTML = minutes;
            secondsElement.innerHTML = seconds;

            // Show or hide "Ready to Unlock" message
            if (canBeUnlocked) {
                readyToUnlockElement.innerHTML = "Lock is Ready to Unlock";
                readyToUnlockElement.classList.remove("hidden");
            } else {
                readyToUnlockElement.classList.add("hidden");
            }
        }
    }

    socket.onmessage = function (event) {
        var data = JSON.parse(event.data);
        // Ensure data types are correct before processing
        if (typeof data.days === 'number' && typeof data.hours === 'number' &&
            typeof data.minutes === 'number' && typeof data.seconds === 'number') {
            // Process the received JSON data
            sendTimerUpdate(data.days, data.hours, data.minutes, data.seconds, data.isFrozen, data.isAllowedToViewTime, data.canBeUnlocked);
        } else {
            console.error("Invalid data received from WebSocket");
        }
    };
</script>
</body>
</html>
)=====";

void sendTimerUpdate(unsigned int days, unsigned int hours, unsigned int minutes, unsigned int seconds, bool isFrozen, bool isAllowedToViewTime, bool canBeUnlocked) {
    // Create a string with the time information
    String timeString;
    if (isAllowedToViewTime) {
        timeString = "hidden";
    } else {
        if (canBeUnlocked) {
            timeString = "Lock is Ready to Unlock";
        } else {
            if (days > 1) {
                timeString = String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
            } else {
                timeString = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
            }

            if (isFrozen) {
                timeString += " (frozen)";
            }
        }
    }

    // Use the WebSocket to send the time string to the web page
    String payload = "{\"time\": \"" + timeString + "\"}";
    webSocket.textAll(payload);
}


void sendTimerUpdate(unsigned int days, unsigned int hours, unsigned int minutes, unsigned int seconds, bool isFrozen, bool isAllowedToViewTime) {
  // Create a string with the time information
  String timeString;
  if (isAllowedToViewTime) {
    timeString = "hidden";
  } else {
    if (days > 1) {
      timeString = String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
    } else {
      timeString = String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
    }

    if (isFrozen) {
      timeString += " (frozen)";
    }
  }

  // Use the WebSocket to send the time string to the web page
  String payload = "{\"time\": \"" + timeString + "\"}";
  webSocket.textAll(payload);
}


void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        // Handle WebSocket data here
        Serial.println("WebSocket data received");
        // Parse and process the JSON data if applicable
    }
}






time_t getNTPTime() {
  const int NTP_PACKET_SIZE = 48;
  byte packetBuffer[NTP_PACKET_SIZE];

  WiFiUDP udp;
  udp.begin(2390); // Local port to listen for incoming NTP packets

  Serial.println("Transmit NTP Request");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  udp.beginPacket(ntpServer, 123); // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("No NTP packet yet");
    return 0;
  }

  Serial.print("NTP packet received, length=");
  Serial.println(cb);

  udp.read(packetBuffer, NTP_PACKET_SIZE);

  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  Serial.print("Seconds since Jan 1 1900 = ");
  Serial.println(secsSince1900);

  const unsigned long seventyYears = 2208988800UL;
  unsigned long epoch = secsSince1900 - seventyYears;

  Serial.print("Unix time = ");
  Serial.println(epoch);

  return epoch;
}

time_t parseDateTime(String dateTimeStr) {
  struct tm tm;
  const char *dateTimeCStr = dateTimeStr.c_str();
  strptime(dateTimeCStr, "%Y-%m-%dT%H:%M:%S.000Z", &tm);
  return mktime(&tm);
}










void decodejson(String apiResponse) {
  StaticJsonDocument<4096> doc;
  deserializeJson(doc, apiResponse);

  // Serial.println("Decoding JSON:");

  // Check if the array is not empty
  if (doc.size() > 0) {
    // Access the first element of the array
    JsonObject lockData = doc[0];
    Serial.println(apiResponse);

    // Print the entire JSON object
    // Serial.println("JSON Object:");
    // serializeJsonPretty(lockData, Serial);
    // Serial.println("JSON Object End");

    // Check if the key "extensionsAllowUnlocking" exists in the JSON object
    if (lockData.containsKey("extensionsAllowUnlocking")) {
      // Check the type of the value
      if (lockData["extensionsAllowUnlocking"].is<bool>()) {
        extensionsAllowUnlocking = lockData["extensionsAllowUnlocking"].as<bool>();
        Serial.println("extensionsAllowUnlocking is bool");
      } else if (lockData["extensionsAllowUnlocking"].is<int>()) {
        // Handle numeric representation (0 or 1)
        extensionsAllowUnlocking = lockData["extensionsAllowUnlocking"].as<int>() != 0;
        Serial.println("extensionsAllowUnlocking is int");
      }
    } else {
      Serial.println("extensionsAllowUnlocking not found");
    }

    // Modify other conditions in a similar way...
    
    canBeUnlocked = lockData["canBeUnlocked"].as<bool>();
    isAllowedToViewTime = lockData["isAllowedToViewTime"].as<bool>();
    isFrozen = lockData["isFrozen"].as<bool>();

    endDateStr = lockData["endDate"].as<String>();
    lockStatus = lockData["status"].as<String>();

    Serial.println("Extracting values:");
    Serial.print("extensionsAllowUnlocking: ");
    Serial.println(extensionsAllowUnlocking);
    Serial.print("canBeUnlocked: ");
    Serial.println(canBeUnlocked);
    Serial.print("endDate: ");
    Serial.println(endDateStr);
    Serial.print("isAllowedToViewTime: ");
    Serial.println(isAllowedToViewTime);
    Serial.print("isFrozen: ");
    Serial.println(isFrozen);
    Serial.print("status: ");
    Serial.println(lockStatus);
    currentTime = time(nullptr);
    time_t endDate = parseDateTime(endDateStr);
    if (endDate>currentTime) {
      durationLeft = endDate - currentTime;
    } else {
      durationLeft = 0;
    }
    if (endDate != true || endDate != false) {
      isAllowedToViewTimeNULL = true;
    } else {
      isAllowedToViewTimeNULL = false;
    }

    jsonresponse=true;
  } else {
    Serial.println("JSON array is empty.");
    jsonresponse=false;
  }
}




// String makeApiRequest(const String &accessToken) {
//   HTTPClient http;
//   http.begin(apiEndpoint);
//   http.addHeader("Authorization", "Bearer " + accessToken);

//   int httpResponseCode = http.sendRequest("GET");

//   String apiResponse;
//   if (httpResponseCode == HTTP_CODE_OK) {
//     apiResponse = http.getString();
//     Serial.println("apirespnse" + apiResponse);
//     decodejson(apiResponse);
//     apiresponded=false;
//   } else {
//     Serial.printf("[HTTP] GET... failed, error code: %d\n", httpResponseCode);
//     apiresponded=true;
//     String payload = http.getString();
//     Serial.printf("[HTTP] Response: %s\n", payload.c_str());
//     apiResponse = "API Request Failed";
//   }

//   http.end();

//   return apiResponse;
// }


void performApiCall() {
  HTTPClient httpApiClient;
  if (httpApiClient.begin(apiEndpoint)) {
    httpApiClient.addHeader("Authorization", "Bearer " + accessToken);

    int httpResponseCode = httpApiClient.GET();

    if (httpResponseCode == HTTP_CODE_OK) {
      String apiResponse = httpApiClient.getString();
      decodejson(apiResponse);
      if (((lockStatus == "locked" && extensionsAllowUnlocking) && (jsonresponse))) {
        servo1.write(10);
      } else {
        servo1.write(180);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error code: %d\n", httpResponseCode);
      servo1.write(10);
    }
    
    httpApiClient.end();
  } else {
    Serial.println("Failed to initialize HTTP client");
  }
}






bool requestAuthenticationUrl(String& authUrl) {
  HTTPClient http;

  http.begin(oauthEndpoint);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // Enable following redirects

  int httpResponseCode = http.GET();

  if (httpResponseCode == HTTP_CODE_OK) {
    // Get the redirected URL
    authUrl = http.getLocation();
    Serial.println("Authentication URL: " + authUrl);
    return true;
  } else {
    Serial.printf("[HTTP] GET... failed, error code: %d\n", httpResponseCode);
    String payload = http.getString();
    Serial.printf("[HTTP] Response: %s\n", payload.c_str());
    return false;
  }

  http.end();
}




bool refreshTokenIfNeeded() {
  if (millis() - lastTokenRefreshTime >= tokenRefreshInterval) {
    HTTPClient http;
    http.begin(tokenEndpoint);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String payload = "grant_type=refresh_token&refresh_token=" + String(refreshToken) +
                     "&client_id=" + String(clientId) + "&client_secret=" + String(clientSecret);

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode == HTTP_CODE_OK) {
      String response = http.getString();
      // Serial.println("Token refreshed successfully!");
      // Serial.println("Response: " + response);

      StaticJsonDocument<2048> doc;
      deserializeJson(doc, response);

      accessToken = doc["access_token"].as<String>();
      lastTokenRefreshTime = millis();

      return true;
    } else {
      Serial.println("Token refresh failed. HTTP error code: " + String(httpResponseCode));
      return false;
    }

    http.end();
  }

  return true;  // Token is still valid
}






void handleCallback(AsyncWebServerRequest *request) {
  String code = request->arg("code");
  String state = request->arg("state");

  // Serial.println("Authorization Code: " + code);
  // Serial.println("State: " + state);

  // Use the authorization code to obtain an access token
  String tokenUrl = String(tokenEndpoint);

  HTTPClient httpClient;
  httpClient.begin(tokenUrl);
  httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Build the request body
  String requestBody = "client_id=" + String(clientId) +
                       "&client_secret=" + String(clientSecret) +
                       "&code=" + code +
                       "&grant_type=authorization_code" +
                       "&redirect_uri=" + String(redirectUri);

  int httpCode = httpClient.POST(requestBody);

  if (httpCode == HTTP_CODE_OK) {
    // Parse the response and print the entire JSON document
    String payload = httpClient.getString();
    // Serial.println("JSON Response: " + payload);

    DynamicJsonDocument jsonDocument(bufferSize);
    DeserializationError error = deserializeJson(jsonDocument, payload);

    if (error) {
    Serial.print("Error parsing JSON: ");
    Serial.println(error.c_str());
    request->send(500, "text/plain", "Internal Server Error");
    return;
  }

    // Print the JSON document to Serial Monitor
    // serializeJsonPretty(jsonDocument, Serial);

    // Check if keys are present in the JSON structure
    if (jsonDocument.containsKey("refresh_token") && jsonDocument.containsKey("access_token")) {
      refreshToken = jsonDocument["refresh_token"].as<String>();
      accessToken = jsonDocument["access_token"].as<String>();

      // Serial.println("Refresh Token: " + refreshToken);
      // Serial.println("Access Token: " + accessToken);

      // Your logic to use the refresh token and access token

      // Now make a request to the API using the obtained access token
      performApiCall();

        request->send(200, "text/html", htmlTemplate);
    } else {
      Serial.println("Error: 'refresh_token' or 'access_token' not found in JSON response");
        request->send(200, "text/html", htmlTemplate);
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error code: %d\n", httpCode);
    String payload = httpClient.getString();
    Serial.printf("[HTTP] Response: %s\n", payload.c_str());
      request->send(200, "text/html", htmlTemplate);
  }

  httpClient.end();
  
}


void scanNetworks() {
  int networks = WiFi.scanNetworks();
  availableSSIDs.clear();

  for (int i = 0; i < networks; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() > 0 && ssid.length() < 32) {  // Ensure SSID is not empty and within valid length
      availableSSIDs.push_back(ssid);
    }
  }
}



void setupWiFiConfigAP() {
  if (WiFi.status() != WL_CONNECTED) {
  WiFi.softAP(apSSID, apPassword, apChannel);
  dnsServer.start(53, "*", WiFi.softAPIP());
  if (MDNS.begin("esp8266")) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up mDNS");
  }
  scanNetworks();
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><style>";
    html += "body { background-color: #1a1a1a; color: #ffffff; font-family: Arial, sans-serif; }";
    html += "h1 { color: #61dafb; text-align: center; }";
    html += "form { max-width: 400px; margin: 0 auto; padding: 20px; background-color: #2a2a2a; border-radius: 10px; }";
    html += "select, input { width: 100%; padding: 10px; margin-bottom: 10px; border: 1px solid #404040; border-radius: 5px; background-color: #1a1a1a; color: #ffffff; }";
    html += "input[type='submit'] { background-color: #61dafb; color: #000000; cursor: pointer; }";
    // Add your dark theme styles here
    html += "</style></head><body>";
    html += "<h1>WiFi Configuration</h1>";
    html += "<form method='POST' action='/save'>";
    
    html += "SSID: <select name='ssid'>";
    // Populate SSID options from the global variable
    for (const auto& ssid : availableSSIDs) {
      html += "<option value='" + ssid + "'>" + ssid + "</option>";
    }
    html += "</select><br>";
    
    html += "Password: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Connect'>";
    html += "</form></body></html>";
    request->send(200, "text/html", html);
  });


 server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
  String ssid;
  String password;

  // Check if there are parameters in the request
  if (request->params() > 0) {
    for (size_t i = 0; i < request->params(); i++) {
      AsyncWebParameter* param = request->getParam(i);
      if (param->name() == "ssid") {
        ssid = param->value();
      } else if (param->name() == "password") {
        password = param->value();
      }
    }
  }

  // Check if both SSID and password are present
  if (ssid.length() > 0 && password.length() > 0) {
    Serial.println("Received SSID: " + ssid);
    Serial.println("Received Password: " + password);

    // Save SSID and password to Preferences
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Attempt to connect to the Wi-Fi network
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi. Press <a href='/restart'>here</a> to restart manually.");
      request->send(200, "text/html", "Connected to WiFi. restarting");
      request->redirect("esp8266.local");
      delay(500);
      ESP.restart();
      
    } else {
      Serial.println("\nFailed to connect to WiFi.");
      request->send(200, "text/html", "Failed to connect to WiFi. Please check your credentials.");
    }
  } else {
    Serial.println("Missing SSID or password parameters");
    request->send(400, "text/plain", "Bad Request: Missing SSID or password parameters");
  }
});

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request){
    // Manual restart link
    request->send(200, "text/html", "Restarting...");
    delay(500);
    ESP.restart();
  });

  server.begin();
  }
}








void handleMain(AsyncWebServerRequest *request) {

  // Read ESP32 chip ID
  uint32_t chipId = ESP.getEfuseMac();

  // Convert chip ID to char string
  char chipIdStr[9];  // Assuming a 32-bit ID, which is 8 characters plus null terminator
  sprintf(chipIdStr, "%08X", chipId);


  // Redirect to the OAuth2 provider's authentication page
  String authUrl = String(oauthEndpoint) +
                   "?client_id=" + clientId +
                   "&redirect_uri=" + redirectUri +
                   "&response_type=code" +
                   "&scope=locks" + 
                   "&state=" + String(chipIdStr);

  request->redirect(authUrl);
}

void setup() {
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
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
  if (MDNS.begin("esp8266")) {
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
  servo1.setPeriodHertz(50); 
  servo1.attach(servoPin, 1000, 2000);
  // lcd.begin(16, 2);  // initialize the lcd
  // lcd.clear();       // clear the LCD screen
  servo1.write(10);
  server.addHandler(&webSocket);
  webSocket.onEvent(onWebSocketEvent);

  server.on("/", HTTP_GET, handleMain);
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

}




