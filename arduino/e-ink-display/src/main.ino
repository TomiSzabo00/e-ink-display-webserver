#define ENABLE_GxEPD2_GFX 0

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <HTTPClient.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <time.h>

#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

// Access Point (AP) settings
const byte DNS_PORT = 53;
const char* AP_SSID = "KijelzoWifi";
const char* AP_PASSWORD = "Fanncsi220909";
WebServer server(80);
DNSServer dnsServer;
#define MAX_SSID_COUNT 50
String availableSSIDs[MAX_SSID_COUNT];
int availableSSIDCount = 0;

// EEPROM settings (for storing WiFi credentials)
#define EEPROM_SIZE 96
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32
#define MAX_STR_LEN 32

// Display settings
#define IMAGE_WIDTH 296
#define IMAGE_HEIGHT 128
GxEPD2_3C<GxEPD2_290_C90c, GxEPD2_290_C90c::HEIGHT> display(GxEPD2_290_C90c(/*CS=5*/ 5, /*DC=*/ 17, /*RES=*/ 16, /*BUSY=*/ 4));

// Battery settings
SFE_MAX1704X lipo; // Defaults to the MAX17043
double voltage = 0;
double soc = 0;
unsigned long previousBatteryCheckMillis = 0;
#define BATTERY_CHECK_INTERVAL 2000

// Server URL for image data
const char* serverUrl = "http://80.98.23.213:5002/image/buffers";

// deep sleep
#define uS_TO_S_FACTOR 1000000ULL

// QR code and settings
#define QR_CODE_SIZE 70
// 'wifi', 70x70px
const unsigned char epd_bitmap_wifi [] PROGMEM = {
	0xff, 0xff, 0x83, 0x00, 0x00, 0x03, 0xe7, 0xff, 0xfc, 0xff, 0xff, 0x83, 0x00, 0x00, 0x03, 0xe7, 
	0xff, 0xfc, 0xff, 0xff, 0x83, 0x00, 0x00, 0x03, 0xe7, 0xff, 0xfc, 0xe0, 0x01, 0x8f, 0x1f, 0xcf, 
	0xf0, 0xe7, 0x00, 0x0c, 0xe0, 0x01, 0x8f, 0x1f, 0xcf, 0xf0, 0xe7, 0x00, 0x0c, 0xe7, 0xf1, 0x83, 
	0xf8, 0x30, 0x7c, 0xe7, 0x3f, 0x8c, 0xe7, 0xf1, 0x83, 0xf8, 0x30, 0x7c, 0xe7, 0x3f, 0x8c, 0xe7, 
	0xf1, 0x8b, 0x78, 0x30, 0x3c, 0xe7, 0x3f, 0x8c, 0xe7, 0xf1, 0x8f, 0x18, 0xcf, 0x8c, 0xe7, 0x3f, 
	0x8c, 0xe7, 0xf1, 0x8f, 0x18, 0xcf, 0x8c, 0xe7, 0x3f, 0x8c, 0xe7, 0xf1, 0x80, 0xe7, 0x0e, 0x03, 
	0xe7, 0x3f, 0x8c, 0xe7, 0xf1, 0x80, 0xe7, 0x0e, 0x03, 0xe7, 0x3f, 0x8c, 0xe0, 0x01, 0x8c, 0xe0, 
	0xc0, 0x63, 0x07, 0x00, 0x0c, 0xe0, 0x01, 0x8c, 0xe0, 0xc0, 0x73, 0x07, 0x00, 0x0c, 0xe0, 0x01, 
	0x8c, 0xe0, 0xc0, 0x73, 0x07, 0x00, 0x1c, 0xff, 0xff, 0x8c, 0xe7, 0x31, 0x8c, 0xe7, 0xff, 0xfc, 
	0xff, 0xff, 0x8c, 0xe7, 0x31, 0x8c, 0xe7, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x07, 0x31, 0x8f, 0xe0, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x31, 0x8f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x33, 
	0x9f, 0xe8, 0x02, 0x00, 0xff, 0xf1, 0xff, 0xe7, 0x0f, 0xf0, 0x18, 0xce, 0x70, 0xff, 0xf1, 0xff, 
	0xe7, 0x0f, 0xf0, 0x18, 0xce, 0x70, 0xe7, 0x3e, 0x73, 0x00, 0x00, 0x0f, 0x07, 0x0e, 0x00, 0xe7, 
	0x3e, 0x73, 0x00, 0x00, 0x0f, 0x07, 0x0e, 0x00, 0x1f, 0x0f, 0x8f, 0x1f, 0xcf, 0xe3, 0xe0, 0xc0, 
	0x7c, 0x1f, 0x0f, 0x8f, 0x1f, 0xcf, 0xf3, 0xe0, 0xc0, 0x7c, 0x1f, 0x0f, 0x8f, 0x1f, 0xcf, 0xf3, 
	0xe0, 0xc0, 0x7c, 0xf8, 0x0e, 0x73, 0xf8, 0xf1, 0xff, 0xe0, 0x3e, 0x70, 0xf8, 0x0e, 0x73, 0xf8, 
	0xf1, 0xff, 0xe0, 0x3e, 0x70, 0xe0, 0xff, 0x83, 0x1f, 0x0f, 0x8c, 0x18, 0xf1, 0x80, 0xe0, 0xff, 
	0x83, 0x1f, 0x0f, 0x8c, 0x18, 0xf1, 0x80, 0x60, 0xff, 0x83, 0x9f, 0x07, 0x8c, 0x19, 0xf9, 0x80, 
	0x00, 0x30, 0x03, 0xe7, 0x30, 0x00, 0xe7, 0x3e, 0x0c, 0x00, 0x30, 0x03, 0xe7, 0x30, 0x00, 0xe7, 
	0x3e, 0x0c, 0x1e, 0x0f, 0x80, 0x07, 0xce, 0x7f, 0x18, 0x30, 0x00, 0x1f, 0x0f, 0x80, 0x07, 0xce, 
	0x7f, 0x18, 0x30, 0x00, 0xe0, 0xfe, 0x7c, 0xe7, 0x31, 0xf3, 0xe7, 0xf1, 0x80, 0xe0, 0xfe, 0x7c, 
	0xe7, 0x31, 0xf3, 0xe7, 0xf1, 0x80, 0xe1, 0xfe, 0x7c, 0xe7, 0x31, 0xf3, 0xe7, 0xf1, 0x80, 0xe0, 
	0x3f, 0x83, 0xe0, 0x0e, 0x7c, 0xff, 0x3e, 0x00, 0xe0, 0x3f, 0x83, 0xe0, 0x0e, 0x7c, 0xff, 0x3e, 
	0x00, 0xf8, 0xc0, 0x03, 0x1f, 0x01, 0x80, 0xff, 0x30, 0x00, 0xf8, 0xc0, 0x03, 0x1f, 0x01, 0x80, 
	0xff, 0x30, 0x00, 0xf8, 0xc0, 0x03, 0xbf, 0x01, 0x80, 0x7f, 0x38, 0x00, 0xe0, 0xf1, 0xff, 0xf8, 
	0xc0, 0x00, 0x00, 0xff, 0x8c, 0xe0, 0xf1, 0xff, 0xf8, 0xc0, 0x00, 0x00, 0xff, 0x8c, 0xe0, 0x3e, 
	0x7f, 0x1f, 0xf1, 0x83, 0xf8, 0xf1, 0xfc, 0xe0, 0x3e, 0x7f, 0x1f, 0xf1, 0x83, 0xf8, 0xf1, 0xfc, 
	0xe7, 0xc1, 0xff, 0xe0, 0x0e, 0x0c, 0xff, 0xf1, 0xf0, 0xe7, 0xc1, 0xff, 0xe0, 0x0e, 0x0c, 0xff, 
	0xf1, 0xf0, 0xe7, 0xc1, 0xff, 0xe0, 0x0e, 0x0c, 0xff, 0xf1, 0xf0, 0x00, 0x00, 0x0f, 0xff, 0x31, 
	0x8c, 0xe0, 0x3e, 0x70, 0x00, 0x00, 0x0f, 0xff, 0x31, 0x8c, 0xe0, 0x3e, 0x70, 0xff, 0xff, 0x8f, 
	0x18, 0xc0, 0x7f, 0xe7, 0x3e, 0x00, 0xff, 0xff, 0x8f, 0x18, 0xc0, 0x7f, 0xe7, 0x3e, 0x00, 0xff, 
	0xff, 0x8f, 0x19, 0xc0, 0x7f, 0xe6, 0x3e, 0x00, 0xe0, 0x01, 0x83, 0x1f, 0xf1, 0xf0, 0xe0, 0x3e, 
	0x7c, 0xe0, 0x01, 0x83, 0x1f, 0xf1, 0xf0, 0xe0, 0x3e, 0x7c, 0xe7, 0xf1, 0x8f, 0x07, 0xcf, 0xff, 
	0xff, 0xf0, 0x7c, 0xe7, 0xf1, 0x8f, 0x07, 0xcf, 0xff, 0xff, 0xf0, 0x7c, 0xe7, 0xf1, 0x8c, 0x1f, 
	0x3e, 0x1c, 0xff, 0xc0, 0x00, 0xe7, 0xf1, 0x8c, 0x1f, 0x3e, 0x0c, 0xff, 0xc0, 0x00, 0xe7, 0xf1, 
	0x8c, 0x1f, 0x3e, 0x0c, 0xff, 0xc0, 0x00, 0xe7, 0xf1, 0x8f, 0x18, 0x3e, 0x03, 0x07, 0xff, 0xf0, 
	0xe7, 0xf1, 0x8f, 0x18, 0x3e, 0x03, 0x07, 0xff, 0xf0, 0xe0, 0x01, 0x8f, 0x1f, 0xc1, 0x8f, 0x00, 
	0xf1, 0x8c, 0xe0, 0x01, 0x8f, 0x1f, 0xc1, 0x8f, 0x00, 0xf1, 0x8c, 0xe0, 0x03, 0x8f, 0x1f, 0xc1, 
	0x9f, 0x01, 0xf1, 0x8c, 0xff, 0xff, 0x8c, 0xe7, 0x0e, 0x70, 0xff, 0xf1, 0x80, 0xff, 0xff, 0x8c, 
	0xe7, 0x0e, 0x70, 0xff, 0xf1, 0x80
};

void setup() {
  display.init(9600, true, 20, false);
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  Wire.begin();
  lipo.begin();
  lipo.quickStart();

  checkForLowBattery();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  Serial.printf("Wakeup reason: %d\n", wakeup_reason);

  // Set the time zone for Budapest (CET/CEST)
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  String ssid = readEEPROM(WIFI_SSID_ADDR);
  String pass = readEEPROM(WIFI_PASS_ADDR);

  if (!ssid.isEmpty()) {
    WiFi.begin(ssid, pass);
    unsigned long startMillis = millis();
    while(!WiFi.isConnected()) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        
        // Timeout after 10 seconds
        if (millis() - startMillis > 10000) {
            Serial.println("Failed to connect to WiFi");
            drawNoInternet();
            scanSSIDs();
            startAPMode();
            return;
        }
    }

    Serial.println("Connected to WiFi");
    startMainOperation();
  } else {
    Serial.println("No saved WiFi credentials found. Starting AP mode...");
    drawNoInternet();
    scanSSIDs();
    startAPMode();
  }
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}

void measureBattery() {
  delay(100);
  voltage = lipo.getVoltage();
  soc = lipo.getSOC();

  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.println(" V");

  Serial.print("Percentage: ");
  Serial.print(soc);
  Serial.println(" %");
}

void checkForLowBattery() {
  double avgVoltage = 0;
  double avgSoc = 0;
  int numberOfSamples = 5;

  for(int i = 0; i < numberOfSamples; i++) {
    measureBattery();
    lipo.reset();
    lipo.quickStart();
    avgVoltage += voltage;
    avgSoc += soc;
    delay(200);
  }

  voltage = avgVoltage / numberOfSamples;
  soc = avgSoc / numberOfSamples;
}

void deepSleep() {
  configTime(3600, 3600, "pool.ntp.org");

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    esp_deep_sleep_start();
    return;
  }

  Serial.printf("Current time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  int secondsToMidnight = ((5 - timeinfo.tm_hour) * 3600) 
                          + ((29 - timeinfo.tm_min) * 60)
                          + (59 - timeinfo.tm_sec);

  if (secondsToMidnight < 0) {
    // Turn negative seconds into positive by adding 24 hours
    secondsToMidnight += (24 * 3600);
  }

  Serial.printf("Going to deep sleep for %d seconds (~%.2f hours)\n", secondsToMidnight, secondsToMidnight / 3600.0);

  esp_sleep_enable_timer_wakeup((uint64_t)secondsToMidnight * uS_TO_S_FACTOR);

  Serial.println("Entering deep sleep now.");
  WiFi.disconnect();
  EEPROM.end();
  display.end();
  display.powerOff();
  delay(1000); // Allow time for the display to power off
  esp_deep_sleep_start();
}

void scanSSIDs() {
  Serial.println("Scanning for available networks...");
  // Set to station mode and disconnect from any previous network
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);  // Allow time for the radio to settle

  int n = WiFi.scanNetworks();
  if (n < 0) {
    Serial.printf("Scan failed with error code: %d\n", n);
    availableSSIDCount = 0;
    drawError();
    return;
  }
  Serial.printf("Scan complete. Found %d networks.\n", n);
  
  // Limit the count to our maximum array size
  availableSSIDCount = (n < MAX_SSID_COUNT) ? n : MAX_SSID_COUNT;
  for (int i = 0; i < availableSSIDCount; i++) {
    availableSSIDs[i] = WiFi.SSID(i);
  }
}

// WiFi connection attempt
bool connectWiFi(const char* ssid, const char* pass) {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries++ < 20) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

// Start AP + DNS redirection (Captive Portal)
void startAPMode() {
  Serial.println("Starting AP + Captive Portal...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(100); // Wait for AP setup
  
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  Serial.printf("Connect to '%s' (password: '%s')\n", AP_SSID, AP_PASSWORD);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  server.onNotFound(handleRoot);

  server.begin();
  Serial.println("HTTP server started.");
}

// HTML form
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<title>ESP32 WiFi Setup</title></head><body>";
  html += "<h2>Select WiFi Network:</h2>";
  
  if (availableSSIDCount == 0) {
    html += "<p>No networks found. Please refresh the page.</p>";
  } else {
    html += "<form method='POST' action='/save'>";
    html += "<select name='ssid'>";
    for (int i = 0; i < availableSSIDCount; i++) {
      html += "<option value='" + availableSSIDs[i] + "'>" + availableSSIDs[i] + "</option>";
    }
    html += "</select><br><br>";
    html += "Password: <input type='password' name='pass'><br><br>";
    html += "<input type='submit' value='Save & Connect'>";
    html += "</form>";
  }
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Save submitted WiFi settings
void handleSave() {
  if (!server.hasArg("ssid") || !server.hasArg("pass")) {
    server.send(400, "text/plain", "SSID and Password required.");
    return;
  }

  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  server.send(200, "text/html", "Csatlakozas a '" + ssid + "' nevu halozathoz. Ha sikerul, a kijelzo ujraindul. Lecsatlakozhatsz errol a wifirol.");

  delay(1000);

  writeEEPROM(WIFI_SSID_ADDR, ssid);
  writeEEPROM(WIFI_PASS_ADDR, pass);

  delay(500);
  ESP.restart();
}

// EEPROM helper functions
void writeEEPROM(int addr, String data) {
  for (int i = 0; i < MAX_STR_LEN; ++i) {
    if (i < data.length()) {
      EEPROM.write(addr + i, data[i]);
    } else {
      EEPROM.write(addr + i, '\0');
    }
  }
  EEPROM.commit();
}

String readEEPROM(int addr) {
  char data[MAX_STR_LEN];
  for (int i = 0; i < MAX_STR_LEN; ++i) {
    data[i] = EEPROM.read(addr + i);
  }
  data[MAX_STR_LEN - 1] = '\0';
  return String(data);
}

// Main operation once connected
void startMainOperation() {
  downloadImage();
  deepSleep();
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
      drawError();
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
      drawError();
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
      drawError();
      return;
    }
    Serial.println("Red buffer read successfully");
    drawImage(blackBuffer, redBuffer);
  } else {
    Serial.println("HTTP Error: " + String(httpCode));
    drawError();
  }
  http.end();
}

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
  display.hibernate();
}

void drawError() {
  display.setRotation(3);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  String message = "Szolj errol Tominak, es 2 pusziert cserebe megjavitja";

  display.firstPage();
  do {
    int rectWidth = IMAGE_WIDTH - 20;
    int rectHeight = IMAGE_HEIGHT - 20;
    int rectX = (IMAGE_WIDTH - rectWidth) / 2;
    int rectY = (IMAGE_HEIGHT - rectHeight) / 2;

    // Draw red/black/white layered rectangle
    display.fillRect(rectX + 5, rectY + 5, rectWidth, rectHeight, GxEPD_RED);
    display.fillRect(rectX, rectY, rectWidth, rectHeight, GxEPD_BLACK);
    display.fillRect(rectX + 2, rectY + 2, rectWidth - 4, rectHeight - 4, GxEPD_WHITE);

    // Text box margins
    int textX = rectX + 10;
    int textY = rectY + 20;
    int maxWidth = rectWidth - 60;

    // Draw title
    display.setCursor(textX + 1, textY + 1);
    display.setTextColor(GxEPD_RED);
    display.print("Hiba :(");
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(textX - 1, textY - 1);
    display.print("Hiba :(");

    // Draw separator
    textY += 10; // move down for line
    display.drawFastHLine(10, textY, rectWidth, GxEPD_BLACK);

    // Draw message with word wrap
    textY += 20;
    int lineHeight = 16;
    String word = "";
    String line = "";
    for (unsigned int i = 0; i < message.length(); i++) {
      char c = message[i];
      if (c == ' ' || c == '\n') {
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(line + word, textX, textY, &x1, &y1, &w, &h);
        if (w > maxWidth) {
          display.setCursor(textX, textY);
          display.print(line);
          line = word + " ";
          textY += lineHeight;
        } else {
          line += word + " ";
        }
        word = "";
        if (c == '\n') {
          display.setCursor(textX, textY);
          display.print(line);
          line = "";
          textY += lineHeight;
        }
      } else {
        word += c;
      }
    }
    // Print the remaining text
    if (line.length() > 0) {
      display.setCursor(textX, textY);
      display.print(line + word);
    }

  } while (display.nextPage());

  display.hibernate();
  deepSleep();
}

void drawNoInternet() {
  display.setRotation(3);
  display.firstPage();
  display.fillScreen(GxEPD_WHITE);
  do {
    int rectWidth = IMAGE_WIDTH - 20;
    int rectHeight = IMAGE_HEIGHT - 20;
    int rectX = (IMAGE_WIDTH - rectWidth) / 2;
    int rectY = (IMAGE_HEIGHT - rectHeight) / 2;
    display.fillRect(rectX + 5, rectY + 5, rectWidth, rectHeight, GxEPD_RED);
    display.fillRect(rectX, rectY, rectWidth, rectHeight, GxEPD_BLACK);
    display.fillRect(rectX + 2, rectY + 2, rectWidth - 4, rectHeight - 4, GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(20, 30);
    display.print("Nincs internet :(");
    display.drawFastHLine(10, nextLine(), rectWidth, GxEPD_BLACK);
    display.setCursor(20, 60);
    display.print("Csatlakozz a ");
    display.setCursor(20, nextTextLine());
    display.setTextColor(GxEPD_RED);
    display.print("KijelzoWifi");
    display.setCursor(display.getCursorX(), sameLine());
    display.setTextColor(GxEPD_BLACK);
    display.print("-hez");

    // qr code
    display.drawBitmap(rectWidth - QR_CODE_SIZE + 5, rectHeight - QR_CODE_SIZE + 5, epd_bitmap_wifi, QR_CODE_SIZE, QR_CODE_SIZE, GxEPD_BLACK);
  } while (display.nextPage());

  display.hibernate();
}

void drawLowBattery() {
  display.setRotation(3);
  display.firstPage();
  do {
    // draw a little battery icon on the top right corner
    int rectWidth = 30;
    int rectHeight = 18;
    int rectX = IMAGE_WIDTH - rectWidth;
    int rectY = 0;
    display.fillRect(rectX, rectY, rectWidth, rectHeight, GxEPD_BLACK);
    display.fillRect(rectX + 1, rectY + 1, rectWidth - 2, rectHeight - 2, GxEPD_WHITE);

    int batteryWidth = 20;
    int batteryHeight = 10;
    int batteryX = rectX + (rectWidth - batteryWidth) / 2 - 1;
    int batteryY = rectY + (rectHeight - batteryHeight) / 2;
    display.fillRect(batteryX, batteryY, batteryWidth, batteryHeight, GxEPD_BLACK);
    display.fillRect(batteryX + 1, batteryY + 1, 3, batteryHeight - 2, GxEPD_RED);
    display.fillRect(batteryX + batteryWidth, batteryY + batteryHeight / 2 - 2.5, 2, 6, GxEPD_BLACK);

  } while (display.nextPage());
  display.hibernate();
}

int16_t nextLine() {
  return display.getCursorY() + 10;
}

int16_t nextTextLine() {
  return display.getCursorY() + 20;
}

int16_t sameLine() {
  return display.getCursorY();
}
