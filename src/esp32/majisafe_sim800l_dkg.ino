/*
 * MajiSafe ESP32 - Real SIM800L + DKG Integration
 * SMS Payment → Knowledge Asset → Pump Control
 * Based on your working SIM800L setup
 */

#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define PUMP_PIN             2
#define FLOW_SENSOR_PIN      13

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

HardwareSerial SerialAT(1);
WebServer server(80);

// Network configuration - UPDATE THESE
const char* ssid = "Josh";
const char* password = "Jitimay$$";
const char* dkgBridgeURL = "http://192.168.1.59:5002/process-sms";  // Your laptop IP

// MajiSafe configuration
String pumpId = "PUMP001";
String deviceId = "MAJISAFE_ESP32_001";

// Global variables
volatile int flowPulses = 0;
unsigned long lastSMSCheck = 0;
bool pumpActive = false;
float totalLitersDispensed = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize pump to OFF state (active-low relay)
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF
  delay(500);
  Serial.println("🛑 PUMP FORCED OFF - Relay initialized");
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  initModem();
  initWiFi();
  initWebServer();
  
  Serial.println("🌊 MajiSafe SMS → DKG → Pump System Ready");
  Serial.println("🌙 Network: Moonbase Alpha");
  Serial.println("🔗 DKG Bridge: " + String(dkgBridgeURL));
  Serial.println("🌐 HTTP Server: http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
  
  // Check for SMS every 5 seconds
  if (millis() - lastSMSCheck > 5000) {
    checkForPaymentSMS();
    lastSMSCheck = millis();
  }
  
  if (pumpActive) {
    monitorPump();
  }
  
  delay(100);
}

void initWiFi() {
  Serial.println("📶 Connecting to WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n❌ WiFi Failed - SMS will still work");
  }
}

void initWebServer() {
  // HTTP endpoint for pump activation (from DKG Bridge)
  server.on("/activate-pump", HTTP_POST, []() {
    if (server.hasArg("seconds") && server.hasArg("ual")) {
      int seconds = server.arg("seconds").toInt();
      String ual = server.arg("ual");
      
      Serial.println("🔗 DKG Pump activation: " + String(seconds) + "s");
      Serial.println("🔗 Knowledge Asset UAL: " + ual);
      
      activatePump(seconds);
      
      // Send completion data back to DKG Bridge
      sendPumpCompletionData(seconds, ual);
      
      server.send(200, "application/json", 
        "{\"status\":\"success\",\"message\":\"Pump activated\",\"liters\":" + String(totalLitersDispensed) + "}");
    } else {
      server.send(400, "application/json", 
        "{\"status\":\"error\",\"message\":\"Missing parameters\"}");
    }
  });
  
  // Status endpoint
  server.on("/status", HTTP_GET, []() {
    String status = pumpActive ? "active" : "inactive";
    DynamicJsonDocument doc(512);
    doc["pump_status"] = status;
    doc["pump_id"] = pumpId;
    doc["device_id"] = deviceId;
    doc["flow_pulses"] = flowPulses;
    doc["total_liters"] = totalLitersDispensed;
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["ip_address"] = WiFi.localIP().toString();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  server.begin();
  Serial.println("🌐 HTTP Server started on port 80");
}

void initModem() {
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  
  Serial.println("📡 Initializing SIM800L...");
  SerialAT.println("AT");
  delay(1000);
  if (SerialAT.available()) {
    Serial.println("✅ Modem responds: " + SerialAT.readString());
  } else {
    Serial.println("❌ No modem response!");
  }
  
  // Check SIM card
  SerialAT.println("AT+CPIN?");
  delay(1000);
  if (SerialAT.available()) {
    String simStatus = SerialAT.readString();
    Serial.println("📱 SIM Status: " + simStatus);
  }
  
  // Check network registration
  SerialAT.println("AT+CREG?");
  delay(1000);
  if (SerialAT.available()) {
    String networkStatus = SerialAT.readString();
    Serial.println("📶 Network: " + networkStatus);
  }
  
  // Set SMS text mode
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  Serial.println("📨 SMS text mode enabled");
  
  Serial.println("✅ SIM800L initialized - Ready for SMS");
}

void checkForPaymentSMS() {
  Serial.println("🔍 Checking for SMS...");
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  
  String response = "";
  while (SerialAT.available()) {
    response += SerialAT.readString();
  }
  
  if (response.length() > 10) {
    Serial.println("📨 SMS Response: " + response);
    
    if (response.indexOf("PAY") != -1) {
      Serial.println("💰 Payment SMS detected!");
      processPaymentSMS(response);
      deleteSMS();
    }
  } else {
    Serial.println("📭 No new SMS");
  }
}

void processPaymentSMS(String sms) {
  Serial.println("📱 Processing SMS payment...");
  
  // Extract phone number
  String phoneNumber = extractPhoneNumber(sms);
  
  // Extract payment command
  int payIndex = sms.indexOf("PAY");
  if (payIndex == -1) {
    Serial.println("❌ No PAY command found");
    return;
  }
  
  String paymentData = sms.substring(payIndex);
  int okIndex = paymentData.indexOf("OK");
  if (okIndex != -1) {
    paymentData = paymentData.substring(0, okIndex);
  }
  
  paymentData.trim();
  paymentData.replace("\n", "");
  paymentData.replace("\r", "");
  
  Serial.println("📱 Phone: " + phoneNumber);
  Serial.println("💰 Payment: " + paymentData);
  
  // Parse payment: "PAY 5000 BIF PUMP001"
  int amount = extractAmount(paymentData);
  String currency = extractCurrency(paymentData);
  String targetPump = extractPumpId(paymentData);
  
  Serial.println("💰 Amount: " + String(amount) + " " + currency);
  Serial.println("🚰 Target: " + targetPump);
  
  if (targetPump == pumpId) {
    // Send to DKG Bridge for Knowledge Asset creation
    sendToDKGBridge(phoneNumber, amount, currency, paymentData);
  } else {
    Serial.println("❌ Wrong pump ID: " + targetPump + " (expected: " + pumpId + ")");
  }
}

void sendToDKGBridge(String phone, int amount, String currency, String originalSMS) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected - cannot send to DKG Bridge");
    return;
  }
  
  Serial.println("🔗 Sending to DKG Bridge...");
  
  HTTPClient http;
  http.begin(dkgBridgeURL);
  http.addHeader("Content-Type", "application/json");
  
  // Create enhanced SMS data for DKG Bridge
  DynamicJsonDocument smsData(1024);
  smsData["phone"] = phone;
  smsData["amount"] = amount;
  smsData["currency"] = currency;
  smsData["pump_id"] = pumpId;
  smsData["device_id"] = deviceId;
  smsData["original_sms"] = originalSMS;
  smsData["coordinates"]["lat"] = -1.9441;  // Rwanda coordinates
  smsData["coordinates"]["lng"] = 30.0619;
  smsData["timestamp"] = millis();
  smsData["network"] = "GSM";
  
  String payload;
  serializeJson(smsData, payload);
  
  Serial.println("📤 Payload: " + payload);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("📥 DKG Response (" + String(httpCode) + "): " + response);
    
    if (httpCode == 200) {
      // Parse DKG Bridge response
      DynamicJsonDocument responseDoc(1024);
      deserializeJson(responseDoc, response);
      
      if (responseDoc["success"]) {
        Serial.println("✅ Knowledge Asset created!");
        Serial.println("🔗 UAL: " + responseDoc["ual"].as<String>());
        Serial.println("🔐 Hash: " + responseDoc["verification_hash"].as<String>());
        
        // Calculate pump duration based on payment
        int pumpDuration = calculatePumpDuration(amount, currency);
        
        // Activate pump
        activatePump(pumpDuration);
        
        // Send completion data
        sendPumpCompletionData(pumpDuration, responseDoc["ual"].as<String>());
        
      } else {
        Serial.println("❌ DKG Bridge error: " + responseDoc["error"].as<String>());
      }
    }
  } else {
    Serial.println("❌ HTTP Error: " + String(httpCode));
  }
  
  http.end();
}

void activatePump(int seconds) {
  Serial.println("🚰 Activating pump for " + String(seconds) + " seconds");
  
  pumpActive = true;
  flowPulses = 0;
  
  digitalWrite(PUMP_PIN, LOW);   // LOW = ON for active-low relay
  Serial.println("⚡ RELAY ON - Pump Running");
  
  // Non-blocking pump operation with monitoring
  unsigned long startTime = millis();
  unsigned long duration = seconds * 1000;
  
  while (millis() - startTime < duration) {
    if ((millis() - startTime) % 1000 == 0) {
      int remaining = (duration - (millis() - startTime)) / 1000;
      Serial.println("💧 Pumping... " + String(remaining) + "s remaining, Flow: " + String(flowPulses));
    }
    delay(100);
  }
  
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF for active-low relay
  pumpActive = false;
  
  // Calculate liters dispensed (assuming 0.1L per pulse)
  float litersDispensed = flowPulses * 0.1;
  totalLitersDispensed += litersDispensed;
  
  Serial.println("🛑 RELAY OFF - Pump Stopped");
  Serial.println("💧 Dispensed: " + String(litersDispensed) + "L");
  Serial.println("📊 Total: " + String(totalLitersDispensed) + "L");
}

void sendPumpCompletionData(int duration, String ual) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("📊 Sending pump completion data...");
  
  HTTPClient http;
  http.begin("http://192.168.1.59:5002/pump-completion");
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument completionData(512);
  completionData["event"] = "pump_completion";
  completionData["pump_id"] = pumpId;
  completionData["device_id"] = deviceId;
  completionData["ual"] = ual;
  completionData["duration_seconds"] = duration;
  completionData["flow_pulses"] = flowPulses;
  completionData["liters_dispensed"] = flowPulses * 0.1;
  completionData["total_liters"] = totalLitersDispensed;
  completionData["timestamp"] = millis();
  
  String payload;
  serializeJson(completionData, payload);
  
  int httpCode = http.POST(payload);
  Serial.println("📊 Completion data sent: " + String(httpCode));
  
  http.end();
}

// Helper functions (same as your original code)
String extractPhoneNumber(String sms) {
  int phoneStart = sms.indexOf("\",\"");
  if (phoneStart == -1) return "UNKNOWN";
  
  phoneStart += 3;
  int phoneEnd = sms.indexOf("\"", phoneStart);
  
  if (phoneEnd == -1) return "UNKNOWN";
  
  return sms.substring(phoneStart, phoneEnd);
}

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
  // Convert to USD equivalent and calculate duration
  float usdValue = convertToUSD(amount, currency);
  return max(3, (int)(usdValue * 10)); // Minimum 3 seconds, 10 seconds per USD
}

float convertToUSD(int amount, String currency) {
  if (currency == "BIF") return amount * 0.000000347;  // Burundi Franc
  if (currency == "RWF") return amount * 0.000000312;  // Rwanda Franc
  if (currency == "KES") return amount * 0.0000065;    // Kenya Shilling
  return amount * 0.0004; // Default
}

void monitorPump() {
  static unsigned long lastFlowCheck = 0;
  
  if (millis() - lastFlowCheck > 1000) {
    Serial.println("💧 Flow: " + String(flowPulses) + " pulses");
    lastFlowCheck = millis();
  }
}

void flowPulseCounter() {
  flowPulses++;
}

void deleteSMS() {
  SerialAT.println("AT+CMGD=1,4");  // Delete all read SMS
  delay(1000);
  Serial.println("🗑️ SMS deleted");
}
