#include "wifimanager.h"


const char *apSSID = "esplockbox-Config";
const char *apPassword = "";
const int apChannel = 1;
const int apTimeout = 300;


std::vector<String> availableSSIDs;


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
  if (MDNS.begin("esplockbox")) {
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
      request->redirect("esplockbox.local");
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