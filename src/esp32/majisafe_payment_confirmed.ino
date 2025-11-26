/*
 * MajiSafe ESP32 - Payment Confirmation Required
 * Pump only activates after Web3 payment is confirmed
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

// Network configuration
const char* ssid = "Josh";
const char* password = "Jitimay$$";
const char* dkgBridgeURL = "http://192.168.1.59:5002/process-sms";

// MajiSafe configuration
String pumpId = "PUMP001";
String deviceId = "MAJISAFE_ESP32_001";

// Global variables
volatile int flowPulses = 0;
unsigned long lastSMSCheck = 0;
bool pumpActive = false;
float totalLitersDispensed = 0;

// Payment tracking
struct PendingPayment {
  String phone;
  int amount;
  String currency;
  String eventId;
  unsigned long timestamp;
  bool confirmed;
};

PendingPayment pendingPayment = {"", 0, "", "", 0, false};

void setup() {
  Serial.begin(115200);
  
  // Initialize pump to OFF state
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF
  delay(500);
  Serial.println("🛑 PUMP FORCED OFF - Awaiting payment confirmation");
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  initModem();
  initWiFi();
  initWebServer();
  
  Serial.println("🌊 MajiSafe Payment Confirmation System Ready");
  Serial.println("⚠️  Pump requires Web3 payment confirmation");
}

void loop() {
  server.handleClient();
  
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
    Serial.println("\n❌ WiFi Failed");
  }
}

void initWebServer() {
  // Endpoint for payment confirmation from DKG Bridge
  server.on("/confirm-payment", HTTP_POST, []() {
    if (server.hasArg("event_id") && server.hasArg("tx_hash") && server.hasArg("confirmed")) {
      String eventId = server.arg("event_id");
      String txHash = server.arg("tx_hash");
      bool confirmed = server.arg("confirmed") == "true";
      
      Serial.println("💰 Payment confirmation received:");
      Serial.println("   Event ID: " + eventId);
      Serial.println("   TX Hash: " + txHash);
      Serial.println("   Confirmed: " + String(confirmed ? "YES" : "NO"));
      
      if (confirmed && eventId == pendingPayment.eventId) {
        Serial.println("✅ Payment confirmed! Activating pump...");
        
        // Calculate pump duration
        int duration = calculatePumpDuration(pendingPayment.amount, pendingPayment.currency);
        
        // Activate pump
        activatePump(duration);
        
        // Clear pending payment
        pendingPayment = {"", 0, "", "", 0, false};
        
        server.send(200, "application/json", 
          "{\"status\":\"success\",\"message\":\"Pump activated after payment confirmation\"}");
      } else {
        Serial.println("❌ Payment not confirmed or event ID mismatch");
        server.send(400, "application/json", 
          "{\"status\":\"error\",\"message\":\"Payment not confirmed\"}");
      }
    } else {
      server.send(400, "application/json", 
        "{\"status\":\"error\",\"message\":\"Missing confirmation parameters\"}");
    }
  });
  
  // Status endpoint
  server.on("/status", HTTP_GET, []() {
    DynamicJsonDocument doc(512);
    doc["pump_status"] = pumpActive ? "active" : "inactive";
    doc["pump_id"] = pumpId;
    doc["pending_payment"] = (pendingPayment.eventId != "");
    doc["pending_amount"] = pendingPayment.amount;
    doc["pending_currency"] = pendingPayment.currency;
    doc["total_liters"] = totalLitersDispensed;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  server.begin();
  Serial.println("🌐 HTTP Server started - Ready for payment confirmations");
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
  
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  Serial.println("📨 SMS text mode enabled");
}

void checkForPaymentSMS() {
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  
  String response = "";
  while (SerialAT.available()) {
    response += SerialAT.readString();
  }
  
  if (response.length() > 10 && response.indexOf("PAY") != -1) {
    Serial.println("📱 Payment SMS detected!");
    processPaymentSMS(response);
    deleteSMS();
  }
}

void processPaymentSMS(String sms) {
  Serial.println("📱 Processing SMS payment request...");
  
  String phoneNumber = extractPhoneNumber(sms);
  int payIndex = sms.indexOf("PAY");
  String paymentData = sms.substring(payIndex);
  
  // Clean payment data
  int okIndex = paymentData.indexOf("OK");
  if (okIndex != -1) {
    paymentData = paymentData.substring(0, okIndex);
  }
  paymentData.trim();
  
  // Parse payment details
  int amount = extractAmount(paymentData);
  String currency = extractCurrency(paymentData);
  String targetPump = extractPumpId(paymentData);
  
  Serial.println("📱 Phone: " + phoneNumber);
  Serial.println("💰 Amount: " + String(amount) + " " + currency);
  Serial.println("🚰 Target: " + targetPump);
  
  if (targetPump == pumpId) {
    // Store pending payment
    pendingPayment.phone = phoneNumber;
    pendingPayment.amount = amount;
    pendingPayment.currency = currency;
    pendingPayment.timestamp = millis();
    pendingPayment.confirmed = false;
    
    Serial.println("⏳ Payment request stored - Awaiting Web3 confirmation");
    
    // Send to DKG Bridge (but don't activate pump yet)
    sendToDKGBridge(phoneNumber, amount, currency, paymentData);
  } else {
    Serial.println("❌ Wrong pump ID");
  }
}

void sendToDKGBridge(String phone, int amount, String currency, String originalSMS) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return;
  }
  
  Serial.println("🔗 Sending payment request to DKG Bridge...");
  
  HTTPClient http;
  http.begin(dkgBridgeURL);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument smsData(1024);
  smsData["phone"] = phone;
  smsData["amount"] = amount;
  smsData["currency"] = currency;
  smsData["pump_id"] = pumpId;
  smsData["device_id"] = deviceId;
  smsData["requires_confirmation"] = true;  // Flag for payment confirmation
  smsData["esp32_ip"] = WiFi.localIP().toString();
  smsData["timestamp"] = millis();
  
  String payload;
  serializeJson(smsData, payload);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("📥 DKG Response: " + response);
    
    if (httpCode == 200) {
      DynamicJsonDocument responseDoc(1024);
      deserializeJson(responseDoc, response);
      
      if (responseDoc["success"]) {
        // Store event ID for confirmation matching
        pendingPayment.eventId = responseDoc["event_id"].as<String>();
        
        Serial.println("✅ Payment request registered");
        Serial.println("🔗 Event ID: " + pendingPayment.eventId);
        Serial.println("⏳ Waiting for Web3 payment confirmation...");
      }
    }
  }
  
  http.end();
}

void activatePump(int seconds) {
  Serial.println("🚰 Payment confirmed! Activating pump for " + String(seconds) + " seconds");
  
  pumpActive = true;
  flowPulses = 0;
  
  digitalWrite(PUMP_PIN, LOW);   // LOW = ON
  Serial.println("⚡ RELAY ON - Pump Running (Payment Confirmed)");
  
  unsigned long startTime = millis();
  unsigned long duration = seconds * 1000;
  
  while (millis() - startTime < duration) {
    if ((millis() - startTime) % 1000 == 0) {
      int remaining = (duration - (millis() - startTime)) / 1000;
      Serial.println("💧 Pumping... " + String(remaining) + "s remaining");
    }
    delay(100);
  }
  
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF
  pumpActive = false;
  
  float litersDispensed = flowPulses * 0.1;
  totalLitersDispensed += litersDispensed;
  
  Serial.println("🛑 RELAY OFF - Pump Stopped");
  Serial.println("💧 Dispensed: " + String(litersDispensed) + "L");
}

// Helper functions
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
  float usdValue = convertToUSD(amount, currency);
  return max(3, (int)(usdValue * 10));
}

float convertToUSD(int amount, String currency) {
  if (currency == "BIF") return amount * 0.000000347;
  if (currency == "RWF") return amount * 0.000000312;
  if (currency == "KES") return amount * 0.0000065;
  return amount * 0.0004;
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
  SerialAT.println("AT+CMGD=1,4");
  delay(1000);
}
