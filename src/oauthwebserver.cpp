#include "oauthwebserver.h"
#include "wifiincludes.h"

const char htmlTemplate[] PROGMEM = R"=====(
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

     <button id="unlockButton" onclick="unlockButtonPressed()" style="display: {{showUnlockButton ? 'block' : 'none'}};">Unlock</button>

<script>
    var socket = new WebSocket("ws://" + window.location.host + "/ws");

    function unlockButtonPressed() {
            // Send a message to the server indicating the button is pressed
            socket.send("unlockButtonPressed");
    }
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

    function updateUnlockButtonVisibility(visibility) {
      var unlockButton = document.getElementById("unlockButton");
      if (unlockButton) {
        unlockButton.style.display = visibility ? 'block' : 'none';
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
        if (data.unlockButtonVisibility !== undefined) {
          updateUnlockButtonVisibility(data.unlockButtonVisibility);
        }
    };
</script>
</body>
</html>
)=====";





void unlockDevice() {
    // Add your logic to unlock the device here
    Serial.println("Device unlocked!");
    if (!canBeUnlocked) {
    digitalWrite(pinsolenoid, LOW);
    
    unlocksolenoid = millis();
    }
    // digitalWrite(pinsolenoid, HIGH);
    
}


// void handleWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
//     if (type == WS_EVT_DATA) {
//         String message = String((char*)data);
//         if (message == "unlockButtonPressed") {
//             // Call your C++ function here
//             unlockDevice();
//         }
//     }
// }





void sendTimerUpdate(unsigned int days, unsigned int hours, unsigned int minutes, unsigned int seconds, bool isFrozen, bool isAllowedToViewTime, bool canBeUnlocked) {
    // Create a string with the time information
    String timeString;
    if (isAllowedToViewTime) {
        timeString = "hidden";
    } else {
        if (!canBeUnlocked) {
            timeString = "Lock is Ready to Unlock";

            webSocket.textAll("unlockButtonVisibility:true");
        } else {
            webSocket.textAll("unlockButtonVisibility:false");
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
        String message = String((char*)data);
        if (message == "unlockButtonPressed") {
            // Call your C++ function here
            unlockDevice();
        }
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
    JsonArray extensions = lockData["extensions"].as<JsonArray>();
  
    // Iterate through each element in the "extensions" array
       for (JsonObject extension : extensions) {
      // Check if the "slug" is "verification-picture"
      if (extension["slug"] == "verification-picture") {
        // Access the "_id" field
        const char* verificationIdChar = extension["_id"];
        verificationId = String(verificationIdChar);
        
        // Print or use the verification ID as needed
        Serial.print("Verification Picture ID: ");
        Serial.println(verificationId);
      }
    }
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
    
    // canBeUnlocked = lockData["canBeUnlocked"].as<bool>();
    isAllowedToViewTime = lockData["isAllowedToViewTime"].as<bool>();
    isFrozen = lockData["isFrozen"].as<bool>();

    endDateStr = lockData["endDate"].as<String>();
    lockStatus = lockData["status"].as<String>();
    lockID = lockData["_id"].as<String>();

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
    Serial.print("id: ");
    Serial.println(lockID);
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
  #if defined(ESP32)
  bool begun = httpApiClient.begin(apiEndpoint);
  #else
  bool begun = httpApiClient.begin(wifiClient ,apiEndpoint);
  #endif
  if (begun) {
    httpApiClient.addHeader("Authorization", "Bearer " + accessToken);

    int httpResponseCode = httpApiClient.GET();

    if (httpResponseCode == HTTP_CODE_OK) {
      String apiResponse = httpApiClient.getString();
      decodejson(apiResponse);
      if (((lockStatus == "locked" && extensionsAllowUnlocking) && (jsonresponse))) {
        canBeUnlocked = false;
      } else {
        canBeUnlocked = true;
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error code: %d\n", httpResponseCode);
      canBeUnlocked = true;
    }
    
    httpApiClient.end();
  } else {
    Serial.println("Failed to initialize HTTP client");
  }
}






bool requestAuthenticationUrl(String& authUrl) {
  HTTPClient http;
  #if defined(ESP32)
  http.begin(oauthEndpoint);
  #else
  http.begin(wifiClient ,oauthEndpoint);
  #endif
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
    #if defined(ESP32)
  http.begin(tokenEndpoint);
  #else
  http.begin(wifiClient ,tokenEndpoint);
  #endif

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
  #if defined(ESP32)
  httpClient.begin(tokenUrl);
  #else
  httpClient.begin(wifiClient ,tokenUrl);
  #endif
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

      performApiCall();
      request->send(200, "text/html", htmlTemplate);
    } else {
      Serial.println("Error: 'refresh_token' or 'access_token' not found in JSON response");
      performApiCall();
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











void handleMain(AsyncWebServerRequest *request) {

  // Read ESP32 chip ID
  #if defined(ESP32)
  uint32_t chipId = ESP.getEfuseMac();
  #else
  uint32_t chipId = ESP.getChipId();
  #endif

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