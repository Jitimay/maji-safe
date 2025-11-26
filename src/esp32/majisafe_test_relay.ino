/*
 * MajiSafe Test - Relay Control via Serial Monitor
 * GPIO 2 connected to relay for pump control
 * Test SMS processing and DKG integration
 */

#define RELAY_PIN 2
#define LED_PIN 13

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// DKG Bridge URL (update with your laptop IP)
const char* dkgBridgeURL = "http://192.168.1.100:5002/process-sms";

String pumpId = "PUMP001";
bool relayActive = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Relay OFF
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("🌊 MajiSafe Test - Moonbase Alpha");
  Serial.println("================================");
  Serial.println("Network: Moonbase Alpha");
  Serial.println("GPIO 2: Relay (Pump Control)");
  Serial.println("Commands:");
  Serial.println("  'test' - Test relay");
  Serial.println("  'sms:PAY 5000 BIF PUMP001' - Simulate SMS");
  Serial.println("  'wifi' - Connect WiFi");
  Serial.println("  'status' - Show status");
  
  connectWiFi();
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    processCommand(command);
  }
  
  delay(100);
}

void connectWiFi() {
  Serial.println("\n📶 Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Failed");
  }
}

void processCommand(String command) {
  Serial.println("\n> " + command);
  
  if (command == "test") {
    testRelay();
  }
  else if (command.startsWith("sms:")) {
    String smsContent = command.substring(4);
    processSMS(smsContent);
  }
  else if (command == "wifi") {
    connectWiFi();
  }
  else if (command == "status") {
    showStatus();
  }
  else if (command == "relay on") {
    activateRelay(5);
  }
  else if (command == "relay off") {
    deactivateRelay();
  }
  else {
    Serial.println("❌ Unknown command");
    Serial.println("Available: test, sms:PAY 5000 BIF PUMP001, wifi, status");
  }
}

void testRelay() {
  Serial.println("🔧 Testing Relay on GPIO 2...");
  
  // Turn ON relay
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("⚡ Relay ON (GPIO 2 HIGH)");
  
  delay(3000);
  
  // Turn OFF relay
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  Serial.println("🛑 Relay OFF (GPIO 2 LOW)");
  
  Serial.println("✅ Relay test complete");
}

void processSMS(String smsContent) {
  Serial.println("📱 Processing SMS: " + smsContent);
  
  // Parse SMS: "PAY 5000 BIF PUMP001"
  if (!smsContent.startsWith("PAY")) {
    Serial.println("❌ Invalid SMS format");
    return;
  }
  
  // Extract payment details
  int amount = extractAmount(smsContent);
  String currency = extractCurrency(smsContent);
  String targetPump = extractPumpId(smsContent);
  
  Serial.println("💰 Amount: " + String(amount) + " " + currency);
  Serial.println("🚰 Target Pump: " + targetPump);
  
  if (targetPump != pumpId) {
    Serial.println("❌ Wrong pump ID");
    return;
  }
  
  // Create SMS data for DKG Bridge
  DynamicJsonDocument smsData(512);
  smsData["phone"] = "+250788123456";
  smsData["amount"] = amount;
  smsData["currency"] = currency;
  smsData["pump_id"] = pumpId;
  smsData["coordinates"]["lat"] = -1.9441;
  smsData["coordinates"]["lng"] = 30.0619;
  smsData["timestamp"] = millis();
  smsData["device_id"] = "TEST_ESP32";
  
  // Send to DKG Bridge
  if (WiFi.status() == WL_CONNECTED) {
    sendToDKGBridge(smsData);
  } else {
    Serial.println("❌ WiFi not connected");
  }
  
  // Activate pump based on payment
  int pumpDuration = calculatePumpDuration(amount, currency);
  activateRelay(pumpDuration);
}

void sendToDKGBridge(DynamicJsonDocument& smsData) {
  Serial.println("🔗 Sending to DKG Bridge...");
  
  HTTPClient http;
  http.begin(dkgBridgeURL);
  http.addHeader("Content-Type", "application/json");
  
  String payload;
  serializeJson(smsData, payload);
  
  Serial.println("📤 Payload: " + payload);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("📥 Response Code: " + String(httpCode));
    Serial.println("📥 Response: " + response);
    
    if (httpCode == 200) {
      // Parse response
      DynamicJsonDocument responseDoc(1024);
      deserializeJson(responseDoc, response);
      
      if (responseDoc["success"]) {
        Serial.println("✅ DKG Bridge Success!");
        Serial.println("🔗 UAL: " + responseDoc["ual"].as<String>());
        Serial.println("🔐 Hash: " + responseDoc["verification_hash"].as<String>());
      } else {
        Serial.println("❌ DKG Bridge Error: " + responseDoc["error"].as<String>());
      }
    }
  } else {
    Serial.println("❌ HTTP Error: " + String(httpCode));
  }
  
  http.end();
}

void activateRelay(int seconds) {
  Serial.println("🚰 Activating pump for " + String(seconds) + " seconds");
  
  relayActive = true;
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("⚡ RELAY ON - Pump Running");
  
  // Non-blocking delay with status updates
  unsigned long startTime = millis();
  unsigned long duration = seconds * 1000;
  
  while (millis() - startTime < duration) {
    if ((millis() - startTime) % 1000 == 0) {
      int remaining = (duration - (millis() - startTime)) / 1000;
      Serial.println("💧 Pumping... " + String(remaining) + "s remaining");
    }
    delay(100);
  }
  
  deactivateRelay();
}

void deactivateRelay() {
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  relayActive = false;
  Serial.println("🛑 RELAY OFF - Pump Stopped");
}

void showStatus() {
  Serial.println("\n📊 System Status:");
  Serial.println("================");
  Serial.println("Pump ID: " + pumpId);
  Serial.println("Relay Pin: GPIO " + String(RELAY_PIN));
  Serial.println("Relay Status: " + String(relayActive ? "ON" : "OFF"));
  Serial.println("WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("IP: " + WiFi.localIP().toString());
  }
  Serial.println("DKG Bridge: " + String(dkgBridgeURL));
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
}

// Helper functions
int extractAmount(String command) {
  int firstSpace = command.indexOf(" ");
  int secondSpace = command.indexOf(" ", firstSpace + 1);
  return command.substring(firstSpace + 1, secondSpace).toInt();
}

String extractCurrency(String command) {
  int firstSpace = command.indexOf(" ");
  int secondSpace = command.indexOf(" ", firstSpace + 1);
  int thirdSpace = command.indexOf(" ", secondSpace + 1);
  return command.substring(secondSpace + 1, thirdSpace);
}

String extractPumpId(String command) {
  int lastSpace = command.lastIndexOf(" ");
  return command.substring(lastSpace + 1);
}

int calculatePumpDuration(int amount, String currency) {
  float usdValue = convertToUSD(amount, currency);
  return max(3, (int)(usdValue * 10)); // Minimum 3 seconds
}

float convertToUSD(int amount, String currency) {
  if (currency == "BIF") return amount * 0.000000347;
  if (currency == "RWF") return amount * 0.000000312;
  if (currency == "KES") return amount * 0.0000065;
  return amount * 0.0004;
}
