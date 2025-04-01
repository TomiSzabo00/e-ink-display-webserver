#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "Tamás iPhone-ja";
const char* password = "VizualTabla";
const char* serverUrl = "http://80.98.23.213:5002/image/buffers";

#define IMAGE_WIDTH 296
#define IMAGE_HEIGHT 128

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

void downloadImage() {
  HTTPClient http;
  http.begin(serverUrl);
  http.setTimeout(10000);
  http.setConnectTimeout(10000);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    const int bufferSize = IMAGE_WIDTH * IMAGE_HEIGHT / 8; // 296 * 128 / 8 = 4736 bytes

    // Allocate buffers on the HEAP (not stack)
    uint8_t* blackBuffer = (uint8_t*) malloc(bufferSize);
    uint8_t* redBuffer = (uint8_t*) malloc(bufferSize);
    memset(blackBuffer, 0, bufferSize);
    memset(redBuffer, 0, bufferSize);

    if (!blackBuffer || !redBuffer) {
      Serial.println("Error: Not enough memory for buffers!");
      http.end();
      return;
    }

    WiFiClient* stream = http.getStreamPtr();

    // Read black buffer (first 4736 bytes)
    int bytesRead = stream->readBytes(blackBuffer, bufferSize);
    if (bytesRead != bufferSize) {
      Serial.println("Warning: Not enough bytes read for black buffer.");
      free(blackBuffer);
      free(redBuffer);
      http.end();
      return;
    }
    Serial.println("Black buffer read successfully");

    // Read red buffer (next 4736 bytes)
    bytesRead = stream->readBytes(redBuffer, bufferSize);
    if (bytesRead != bufferSize) {
      Serial.println("Warning: Not enough bytes read for red buffer.");
      free(blackBuffer);
      free(redBuffer);
      http.end();
      return;
    }
    Serial.println("Red buffer read successfully");

    drawImage(blackBuffer, redBuffer);
  } else {
    Serial.println("HTTP Error: " + String(httpCode));
  }
  http.end();
}

// Example drawImage() implementation for GxEPD2
void drawImage(uint8_t* blackBuffer, uint8_t* redBuffer) {
  display.setRotation(3);
  display.firstPage();

  do {
    for (int y = 0; y < 128; y++) {
      for (int x = 0; x < 296; x++) {
        int idx = y * 296 + x;
        int bytePos = idx / 8;
        int bitPos = 7 - (idx % 8);  // MSB first

        bool isBlack = (blackBuffer[bytePos] >> bitPos) & 1;
        bool isRed = (redBuffer[bytePos] >> bitPos) & 1;

        if (isBlack) {
          display.drawPixel(x, y, GxEPD_BLACK);
        } else if (isRed) {
          display.drawPixel(x, y, GxEPD_RED);
        } else {
          display.drawPixel(x, y, GxEPD_WHITE);
        }
      }
    }
  } while (display.nextPage());

  free(blackBuffer);
  free(redBuffer);
  Serial.println("Image drawn successfully");
}