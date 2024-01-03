#include "img.h"
#include "wifiincludes.h"
time_t millsivar;

bool imgUploadFlag = false;
int pinState1;
int pinState2;

void pinChangeInterrupt() {
  // This function will be called when the input pin changes state (low to high or high to low)
  pinState1 = digitalRead(pin1);
  pinState2 = digitalRead(pin2);
  
  if (millis() - millsivar > 1000) {
    imgUploadFlag = true;
  }     
  
}





    // if (pinState1 == HIGH && pinState2 == HIGH) {
    //     imgupload("/00.png");
    // } else if (pinState1 == HIGH && pinState2 == LOW) {
    //     imgupload("/01.png");
    // } else if (pinState1 == LOW && pinState2 == HIGH) {
    //     imgupload("/10.png");
    // } else if (pinState1 == LOW && pinState2 == HIGH) {
    //     imgupload("/11.png");
    // } 


void checkupdate() {
    if (imgUploadFlag) {
        millsivar = millis();
        imgUploadFlag = false;
        if (pinState1 == HIGH && pinState2 == HIGH) {
        imgupload("/00.png");
        } else if (pinState1 == HIGH && pinState2 == LOW) {
            imgupload("/01.png");
        } else if (pinState1 == LOW && pinState2 == HIGH) {
            imgupload("/10.png");
        } else if (pinState1 == LOW && pinState2 == LOW) {
            imgupload("/11.png");
        } 
        
    }
}


#if !defined(PSRAM_AVAILABLE)


String verificationId;

String verificationEndpoint = "https://api.chaster.app/extensions/verification-picture/";


String readImageFromFlash(const char* filename) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Error opening file");
        return "";
    }

    Serial.println("File size: " + String(file.size()) + " bytes");

    String imageData = "";
    while (file.available()) {
        imageData += char(file.read());
    }

    file.close();
    return imageData;
}


void beginverification() {
    String verificationReqEndpoint = "https://api.chaster.app/locks/" + lockID + "/extensions/" + verificationId + "/action";
    
    HTTPClient http;
    http.begin(verificationReqEndpoint);
    // Set headers before making the request
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + accessToken);
    
    // Make the POST request with the payload
    int httpResponseCode = http.POST(String("{\"action\":\"createVerificationRequest\",\"payload\":{}}"));

    // Check the response code
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        // Log the response data for debugging
        String response = http.getString();
        Serial.println("Response: " + response);
    } else {
        Serial.println("HTTP POST request failed");
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    }

    // End the HTTP session
    http.end();
}

void imgupload(const char* filename) {
    String imageData = readImageFromFlash(filename);

    if (imageData.length() == 0) {
        Serial.println("Image data is empty");
        return;
    }
    
    beginverification();

    String apiUrl = verificationEndpoint + lockID + "/submit/";
    Serial.println("Sending image data to " + apiUrl);

    // Allocate memory in PSRAM for the multipart/form-data request body




  String boundary = "c1a91b32-3f47-464a-94d2-600b9a6de217";
  
  // Construct the multipart/form-data request body
    String requestBodyBuffer = "--" + boundary + "\r\n" +
                                "Content-Disposition: form-data; name=\"file\"; filename=test.png; filename*=utf-8''test.png\r\n\r\n" +
                                imageData + "\r\n" +
                                "--" + boundary + "\r\n" +
                                "Content-Type: text/plain; charset=utf-8\r\n" +
                                "Content-Disposition: form-data; name=\"enableVerificationCode\"\r\n\r\n" +
                                "true\r\n" +
                                "--" + boundary + "--\r\n";

    // Serial.println(requestBodyBuffer);

    // Make sure to set the Content-Type header to multipart/form-data with the specified boundary
    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Authorization", "Bearer " + accessToken);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    // Make the POST request with the constructed body in the PSRAM buffer
    int httpResponseCode = http.POST(requestBodyBuffer);

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        // Log the response data for debugging
        String response = http.getString();
        Serial.println("Response: " + response);
    } else {
        Serial.println("HTTP POST request failed");
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    }

    http.end();


}


#else



String verificationId;

String verificationEndpoint = "https://api.chaster.app/extensions/verification-picture/";


String readImageFromFlash(const char* filename) {
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Error opening file");
        return "";
    }

    Serial.println("File size: " + String(file.size()) + " bytes");

    String imageData = "";
    while (file.available()) {
        imageData += char(file.read());
    }

    file.close();
    return imageData;
}


void beginverification() {
    String verificationReqEndpoint = "https://api.chaster.app/locks/" + lockID + "/extensions/" + verificationId + "/action";
    
    HTTPClient http;
    http.begin(verificationReqEndpoint);
    // Set headers before making the request
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + accessToken);
    
    // Make the POST request with the payload
    int httpResponseCode = http.POST(String("{\"action\":\"createVerificationRequest\",\"payload\":{}}"));

    // Check the response code
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        // Log the response data for debugging
        String response = http.getString();
        Serial.println("Response: " + response);
    } else {
        Serial.println("HTTP POST request failed");
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    }

    // End the HTTP session
    http.end();
}


void imgupload(const char* filename) {
    String imageData = readImageFromFlash(filename);

    if (imageData.length() == 0) {
        Serial.println("Image data is empty");
        return;
    }

    String apiUrl = verificationEndpoint + lockID + "/submit/";
    Serial.println("Sending image data to " + apiUrl);
    beginverification();
    // Allocate memory in PSRAM for the multipart/form-data request body
    char* requestBodyBuffer = (char*)ps_malloc(imageData.length() + 500);  // Adjust the buffer size as needed

    if (requestBodyBuffer == NULL) {
        Serial.println("Failed to allocate PSRAM for requestBodyBuffer");
        return;
    }



  String boundary = "c1a91b32-3f47-464a-94d2-600b9a6de217";
  
  // Construct the multipart/form-data request body
    sprintf(requestBodyBuffer, "--%s\r\n"
                                "Content-Disposition: form-data; name=\"file\"; filename=test.jpeg; filename*=utf-8''test.jpeg\r\n\r\n"
                                "%s\r\n"
                                "--%s\r\n"
                                "Content-Type: text/plain; charset=utf-8\r\n"
                                "Content-Disposition: form-data; name=\"enableVerificationCode\"\r\n\r\n"
                                "true\r\n"
                                "--%s--\r\n", boundary.c_str(), imageData.c_str(), boundary.c_str(), boundary.c_str());
    // Serial.println(requestBodyBuffer);

    // Make sure to set the Content-Type header to multipart/form-data with the specified boundary
    HTTPClient http;
    http.begin(apiUrl);
    http.addHeader("Authorization", "Bearer " + accessToken);
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    // Make the POST request with the constructed body in the PSRAM buffer
    int httpResponseCode = http.POST(requestBodyBuffer);

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        // Log the response data for debugging
        String response = http.getString();
        Serial.println("Response: " + response);
    } else {
        Serial.println("HTTP POST request failed");
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    // Free the allocated PSRAM buffer
    free(requestBodyBuffer);
}
#endif