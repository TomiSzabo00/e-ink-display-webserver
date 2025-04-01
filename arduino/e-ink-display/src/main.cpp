#include <WiFi.h>
#include <HTTPClient.h>
#include <GxEPD2_3C.h>

#define IMAGE_WIDTH 296
#define IMAGE_HEIGHT 128

const char* ssid = "";
const char* password = "";

// Display initialization
GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT / 8> display(GxEPD2_290_C90c(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

const char* serverUrl = "http://80.98.23.213:5002/image/processed";

uint8_t black_bitmap[IMAGE_WIDTH * IMAGE_HEIGHT / 8]; // Buffer for black pixels
uint8_t red_bitmap[IMAGE_WIDTH * IMAGE_HEIGHT / 8];   // Buffer for red pixels

void drawImage(uint8_t *black, uint8_t *red) {
    display.setRotation(0);
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);
        display.drawBitmap(0, 0, black, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_BLACK);
        display.drawBitmap(0, 0, red, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_RED);
    } while (display.nextPage());

    display.hibernate();
}

void downloadImage() {
    HTTPClient http;
    http.begin(serverUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        size_t index = 0;
        
        while (http.connected() && stream->available() && index < sizeof(black_bitmap)) {
            black_bitmap[index] = stream->read();
            index++;
        }

        drawImage(black_bitmap, red_bitmap);
    } else {
        Serial.println("Failed to download image!");
        Serial.println(http.errorToString(httpCode).c_str());
        Serial.println("HTTP Code: " + String(httpCode));
        Serial.println("Response: " + String(http.getString().c_str()));
    }

    http.end();
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  
  unsigned long startMillis = millis();
  while( !WiFi.isConnected() ) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
      
      // Timeout after 10 seconds
      if (millis() - startMillis > 10000) {
          Serial.println("Failed to connect to WiFi");
          return;
      }
  }

  delay(5000);

  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  display.init();
  downloadImage();
}

void loop() {
    // Nothing here, as the ESP will deep sleep after execution
}