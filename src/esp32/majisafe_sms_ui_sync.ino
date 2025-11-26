/*
 * MajiSafe ESP32 - SMS → UI → Web3 → Pump Flow
 * ESP32 communicates with UI about SMS arrival
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

// SMS and Payment tracking
struct PaymentRequest {
  String phone;
  int amount;
  String currency;
  String eventId;
  bool smsReceived;
  bool web3Confirmed;
  unsigned long timestamp;
};

PaymentRequest currentPayment = {"", 0, "", "", false, false, 0};

void setup() {
  Serial.begin(115200);
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF
  Serial.println("🛑 PUMP OFF - Awaiting SMS → Web3 → Confirmation flow");
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  initModem();
  initWiFi();
  initWebServer();
  
  Serial.println("🌊 MajiSafe SMS → UI → Web3 → Pump System Ready");
  Serial.println("📱 Waiting for SMS to enable UI button...");
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Connected: " + WiFi.localIP().toString());
}

void initWebServer() {
  // Endpoint for UI to check SMS status
  server.on("/sms-status", HTTP_GET, []() {
    DynamicJsonDocument doc(512);
    doc["sms_received"] = currentPayment.smsReceived;
    doc["web3_confirmed"] = currentPayment.web3Confirmed;
    doc["pump_id"] = pumpId;
    doc["amount"] = currentPayment.amount;
    doc["currency"] = currentPayment.currency;
    doc["phone"] = currentPayment.phone;
    doc["event_id"] = currentPayment.eventId;
    doc["pump_active"] = pumpActive;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  // Endpoint for Web3 payment confirmation
  server.on("/confirm-web3", HTTP_POST, []() {
    if (server.hasArg("tx_hash") && server.hasArg("event_id")) {
      String txHash = server.arg("tx_hash");
      String eventId = server.arg("event_id");
      
      if (eventId == currentPayment.eventId && currentPayment.smsReceived) {
        currentPayment.web3Confirmed = true;
        
        Serial.println("✅ Web3 payment confirmed!");
        Serial.println("   TX Hash: " + txHash);
        Serial.println("   Event ID: " + eventId);
        
        // Activate pump
        int duration = calculatePumpDuration(currentPayment.amount, currentPayment.currency);
        activatePump(duration);
        
        // Reset for next payment
        currentPayment = {"", 0, "", "", false, false, 0};
        
        server.send(200, "application/json", 
          "{\"status\":\"success\",\"message\":\"Payment confirmed, pump activated\"}");
      } else {
        server.send(400, "application/json", 
          "{\"status\":\"error\",\"message\":\"Invalid confirmation\"}");
      }
    } else {
      server.send(400, "application/json", 
        "{\"status\":\"error\",\"message\":\"Missing parameters\"}");
    }
  });
  
  server.begin();
  Serial.println("🌐 HTTP Server started - UI sync ready");
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
  
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  Serial.println("📨 SMS mode enabled");
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
    // Store SMS payment request
    currentPayment.phone = phoneNumber;
    currentPayment.amount = amount;
    currentPayment.currency = currency;
    currentPayment.smsReceived = true;
    currentPayment.web3Confirmed = false;
    currentPayment.timestamp = millis();
    
    // Create event ID for tracking
    currentPayment.eventId = "water-" + pumpId + "-" + String(millis());
    
    Serial.println("✅ SMS payment request stored");
    Serial.println("🔗 Event ID: " + currentPayment.eventId);
    Serial.println("🌐 UI button should now be enabled");
    
    // Send to DKG Bridge (but don't activate pump yet)
    sendToDKGBridge(phoneNumber, amount, currency, paymentData);
  } else {
    Serial.println("❌ Wrong pump ID");
  }
}

void sendToDKGBridge(String phone, int amount, String currency, String originalSMS) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.println("🔗 Registering payment request with DKG Bridge...");
  
  HTTPClient http;
  http.begin(dkgBridgeURL);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument smsData(1024);
  smsData["phone"] = phone;
  smsData["amount"] = amount;
  smsData["currency"] = currency;
  smsData["pump_id"] = pumpId;
  smsData["device_id"] = deviceId;
  smsData["event_id"] = currentPayment.eventId;
  smsData["status"] = "sms_received";
  smsData["coordinates"]["lat"] = -1.9441;
  smsData["coordinates"]["lng"] = 30.0619;
  smsData["timestamp"] = millis();
  
  String payload;
  serializeJson(smsData, payload);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("📥 DKG Response: Payment request registered");
  }
  
  http.end();
}

void activatePump(int seconds) {
  Serial.println("🚰 Web3 confirmed! Activating pump for " + String(seconds) + " seconds");
  
  pumpActive = true;
  flowPulses = 0;
  
  digitalWrite(PUMP_PIN, LOW);   // LOW = ON
  Serial.println("⚡ RELAY ON - Pump Running (Web3 Confirmed)");
  
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
