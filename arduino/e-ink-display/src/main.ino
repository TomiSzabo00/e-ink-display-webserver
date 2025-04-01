#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "Tamás iPhone-ja";
const char* password = "VizualTabla";
const char* serverUrl = "http://80.98.23.213:5002/image/processed";

#define IMAGE_WIDTH 296
#define IMAGE_HEIGHT 128
uint8_t black_bitmap[IMAGE_WIDTH * IMAGE_HEIGHT / 8];
uint8_t red_bitmap[IMAGE_WIDTH * IMAGE_HEIGHT / 8];

GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> display(GxEPD2_290_C90c(/*CS=5*/ 5, /*DC=*/ 17, /*RES=*/ 16, /*BUSY=*/ 4));

void setup()
{
  display.init(115200, true, 50, false);

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

  delay(2000);

  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  downloadImage();

  display.hibernate();
}

void loop() {
  // put your main code here, to run repeatedly:
}

void drawImage(uint8_t *black, uint8_t *red) {
  display.setRotation(1);
  display.firstPage();
  do {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(0, 0, black, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_BLACK);
      display.drawBitmap(0, 0, red, IMAGE_WIDTH, IMAGE_HEIGHT, GxEPD_RED);
  } while (display.nextPage());

  Serial.println("Image drawn");
  display.hibernate();
}

void downloadImage() {
  HTTPClient http;
  http.begin(serverUrl);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
      WiFiClient* stream = http.getStreamPtr();
      size_t index = 0;
      size_t totalSize = sizeof(black_bitmap) + sizeof(red_bitmap);

      Serial.println("Received data:");

      while (http.connected() && stream->available() && index < totalSize) {
          uint8_t byte = stream->read();
          
          // Print the received byte in HEX format
          Serial.print(byte, HEX);
          Serial.print(" ");

          if (index < sizeof(black_bitmap)) {
              black_bitmap[index] = byte;
          } else {
              red_bitmap[index - sizeof(black_bitmap)] = byte;
          }
          index++;
      }

      Serial.println("\nFinished receiving data.");
      drawImage(black_bitmap, red_bitmap);
  } else {
      Serial.println("Failed to download image!");
      Serial.println(http.errorToString(httpCode).c_str());
      Serial.println("HTTP Code: " + String(httpCode));
      Serial.println("Response: " + String(http.getString().c_str()));
  }

  http.end();
}