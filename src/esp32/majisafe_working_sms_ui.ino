/*
 * MajiSafe ESP32 - Working SMS + UI Sync
 * Based on your working SMS code with UI communication
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

// MajiSafe configuration
String pumpId = "PUMP001";

// Global variables
volatile int flowPulses = 0;
unsigned long lastSMSCheck = 0;
bool pumpActive = false;

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
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF for active-low relay
  delay(500);
  Serial.println("🛑 PUMP FORCED OFF - Relay initialized to HIGH (OFF)");
  
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
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi Connected: " + WiFi.localIP().toString());
  Serial.println("🌐 ESP32 IP: " + WiFi.localIP().toString());
  Serial.println("🔗 Web UI expects: 192.168.1.30");
}

void initWebServer() {
  // Endpoint for UI to check SMS status
  server.on("/sms-status", HTTP_GET, []() {
    // Add CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    
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
  
  // Handle CORS preflight
  server.on("/sms-status", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(200, "text/plain", "");
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
  
  Serial.println("📡 Testing SIM800L connection...");
  SerialAT.println("AT");
  delay(1000);
  if (SerialAT.available()) {
    Serial.println("✅ Modem responds: " + SerialAT.readString());
  } else {
    Serial.println("❌ No modem response!");
  }
  
  SerialAT.println("AT+CPIN?");
  delay(1000);
  if (SerialAT.available()) {
    String simStatus = SerialAT.readString();
    Serial.println("📱 SIM Status: " + simStatus);
  }
  
  SerialAT.println("AT+CREG?");
  delay(1000);
  if (SerialAT.available()) {
    String networkStatus = SerialAT.readString();
    Serial.println("📶 Network: " + networkStatus);
  }
  
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  Serial.println("📨 SMS mode enabled");
}

void checkForPaymentSMS() {
  Serial.println("🔍 Checking for SMS...");
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  
  String response = "";
  while (SerialAT.available()) {
    response += SerialAT.readString();
  }
  
  if (response.length() > 0) {
    Serial.println("📨 SMS Response: " + response);
  } else {
    Serial.println("📭 No SMS found");
  }
  
  if (response.indexOf("PAY") != -1) {
    Serial.println("💰 Payment SMS detected!");
    processPaymentSMS(response);
    deleteSMS();
  }
}

void processPaymentSMS(String sms) {
  int payIndex = sms.indexOf("PAY");
  if (payIndex == -1) return;
  
  // Extract just the payment message line
  String paymentData = sms.substring(payIndex);
  
  // Find the end of the payment line (before "OK")
  int okIndex = paymentData.indexOf("OK");
  if (okIndex != -1) {
    paymentData = paymentData.substring(0, okIndex);
  }
  
  paymentData.trim();
  paymentData.replace("\n", "");
  paymentData.replace("\r", "");
  
  String phoneNumber = extractPhoneNumber(sms);
  phoneNumber.trim();
  
  Serial.println("📱 Clean Phone: " + phoneNumber);
  Serial.println("💰 Clean Payment: " + paymentData);
  
  // Parse payment details
  int amount = extractAmount(paymentData);
  String currency = extractCurrency(paymentData);
  String targetPump = extractPumpId(paymentData);
  
  Serial.println("💰 Amount: " + String(amount) + " " + currency);
  Serial.println("🚰 Target: " + targetPump);
  
  if (targetPump == pumpId || targetPump == "") {
    // Store SMS payment request
    currentPayment.phone = phoneNumber;
    currentPayment.amount = amount;
    currentPayment.currency = currency;
    currentPayment.smsReceived = true;
    currentPayment.web3Confirmed = false;
    currentPayment.timestamp = millis();
    currentPayment.eventId = "water-" + pumpId + "-" + String(millis());
    
    Serial.println("✅ SMS payment request stored");
    Serial.println("🔗 Event ID: " + currentPayment.eventId);
    Serial.println("🌐 UI button should now be enabled");
  } else {
    Serial.println("❌ Wrong pump ID: " + targetPump);
  }
}

String extractPhoneNumber(String sms) {
  // Look for phone number in format: "REC UNREAD","+25769820499"
  int phoneStart = sms.indexOf("\",\"");
  if (phoneStart == -1) return "UNKNOWN";
  
  phoneStart += 3; // Skip ","
  int phoneEnd = sms.indexOf("\"", phoneStart);
  
  if (phoneEnd == -1) return "UNKNOWN";
  
  return sms.substring(phoneStart, phoneEnd);
}

int extractAmount(String command) {
  int firstSpace = command.indexOf(" ");
  if (firstSpace == -1) return 5000; // Default
  int secondSpace = command.indexOf(" ", firstSpace + 1);
  if (secondSpace == -1) return 5000; // Default
  return command.substring(firstSpace + 1, secondSpace).toInt();
}

String extractCurrency(String command) {
  int firstSpace = command.indexOf(" ");
  if (firstSpace == -1) return "BIF";
  int secondSpace = command.indexOf(" ", firstSpace + 1);
  if (secondSpace == -1) return "BIF";
  int thirdSpace = command.indexOf(" ", secondSpace + 1);
  if (thirdSpace == -1) return command.substring(secondSpace + 1);
  return command.substring(secondSpace + 1, thirdSpace);
}

String extractPumpId(String command) {
  int lastSpace = command.lastIndexOf(" ");
  if (lastSpace == -1) return pumpId; // Default to current pump
  String extracted = command.substring(lastSpace + 1);
  extracted.trim();
  return extracted;
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

void activatePump(int seconds) {
  Serial.println("🚰 Web3 confirmed! Activating pump for " + String(seconds) + " seconds");
  
  pumpActive = true;
  digitalWrite(PUMP_PIN, LOW);   // LOW = ON for active-low relay
  Serial.println("⚡ Relay ON - Pump Running (Web3 Confirmed)");
  
  delay(seconds * 1000);
  
  digitalWrite(PUMP_PIN, HIGH);  // HIGH = OFF for active-low relay
  pumpActive = false;
  Serial.println("🛑 Relay OFF - Pump Stopped");
  
  Serial.println("Water dispensed. Flow: " + String(flowPulses) + " pulses");
  flowPulses = 0;
}

void monitorPump() {
  static unsigned long lastFlowCheck = 0;
  
  if (millis() - lastFlowCheck > 1000) {
    Serial.println("Flow rate: " + String(flowPulses) + " L/min");
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
