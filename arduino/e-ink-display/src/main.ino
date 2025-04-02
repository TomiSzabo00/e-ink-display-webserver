#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

const byte DNS_PORT = 53;
const char* AP_SSID = "ESP32_Setup";
const char* AP_PASSWORD = "configure123";

WebServer server(80);
DNSServer dnsServer;
#define MAX_SSID_COUNT 50
String availableSSIDs[MAX_SSID_COUNT];
int availableSSIDCount = 0;

#define EEPROM_SIZE 96
#define WIFI_SSID_ADDR 0
#define WIFI_PASS_ADDR 32
#define MAX_STR_LEN 32

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);

  String ssid = readEEPROM(WIFI_SSID_ADDR);
  String pass = readEEPROM(WIFI_PASS_ADDR);

  Serial.printf("Stored SSID: '%s'\n", ssid.c_str());

  if (!ssid.isEmpty()) {
    if (connectWiFi(ssid.c_str(), pass.c_str())) {
      Serial.println("Connected to stored WiFi!");
      startMainOperation();
      return;
    }
  }

  // If no stored credentials or connection failed, scan for available SSIDs
  scanSSIDs();
  startAPMode();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
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

  server.send(200, "text/html", "Attempting to connect to '" + ssid + "'. ESP32 will restart.");

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
  Serial.println("Connected! Implement your main operation here.");
  // Continue your operation (NTP sync, deep sleep, etc.)
}