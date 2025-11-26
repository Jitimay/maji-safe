/*
 * MajiSafe ESP32 - SMS Debug Version
 * Fixed SMS reception with proper SIM800L handling
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
  Serial.println("🛑 PUMP OFF - Starting SMS debug mode");
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  initModem();
  initWiFi();
  initWebServer();
  
  Serial.println("🌊 MajiSafe SMS Debug System Ready");
  Serial.println("📱 Enhanced SMS reception debugging active");
}

void loop() {
  server.handleClient();
  
  // Check for SMS every 3 seconds (more frequent)
  if (millis() - lastSMSCheck > 3000) {
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
  
  // Test endpoint to simulate SMS
  server.on("/test-sms", HTTP_POST, []() {
    if (server.hasArg("message")) {
      String testMessage = server.arg("message");
      Serial.println("🧪 Test SMS: " + testMessage);
      
      // Simulate SMS format
      String simulatedSMS = "+CMGL: 1,\"REC UNREAD\",\"+250788123456\",\"\",\"25/11/26,16:30:45+12\"\r\n";
      simulatedSMS += testMessage + "\r\n\r\nOK\r\n";
      
      processPaymentSMS(simulatedSMS);
      
      server.send(200, "text/plain", "Test SMS processed");
    } else {
      server.send(400, "text/plain", "Missing message parameter");
    }
  });
  
  server.begin();
  Serial.println("🌐 HTTP Server started - UI sync ready");
  Serial.println("🧪 Test SMS endpoint: http://" + WiFi.localIP().toString() + "/test-sms");
}

void initModem() {
  Serial.println("📡 Initializing SIM800L with enhanced debugging...");
  
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  
  // Power cycle the modem
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, LOW);
  digitalWrite(MODEM_POWER_ON, LOW);
  delay(1000);
  
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  delay(2000);
  
  // Power key pulse
  digitalWrite(MODEM_PWKEY, HIGH);
  delay(1000);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(2000);
  
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  
  // Test basic communication
  Serial.println("📡 Testing SIM800L communication...");
  for (int i = 0; i < 5; i++) {
    SerialAT.println("AT");
    delay(1000);
    if (SerialAT.available()) {
      String response = SerialAT.readString();
      Serial.println("✅ Modem responds: " + response);
      break;
    } else {
      Serial.println("❌ No response, attempt " + String(i + 1));
    }
  }
  
  // Check SIM card
  SerialAT.println("AT+CPIN?");
  delay(2000);
  if (SerialAT.available()) {
    String simStatus = SerialAT.readString();
    Serial.println("📱 SIM Status: " + simStatus);
    if (simStatus.indexOf("READY") == -1) {
      Serial.println("❌ SIM card not ready!");
    }
  }
  
  // Check network registration
  SerialAT.println("AT+CREG?");
  delay(2000);
  if (SerialAT.available()) {
    String networkStatus = SerialAT.readString();
    Serial.println("📶 Network: " + networkStatus);
    if (networkStatus.indexOf("0,1") == -1 && networkStatus.indexOf("0,5") == -1) {
      Serial.println("❌ Not registered to network!");
    }
  }
  
  // Check signal strength
  SerialAT.println("AT+CSQ");
  delay(1000);
  if (SerialAT.available()) {
    String signalStatus = SerialAT.readString();
    Serial.println("📶 Signal: " + signalStatus);
  }
  
  // Set SMS text mode
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  if (SerialAT.available()) {
    String response = SerialAT.readString();
    Serial.println("📨 SMS mode: " + response);
  }
  
  // Set SMS storage
  SerialAT.println("AT+CPMS=\"SM\",\"SM\",\"SM\"");
  delay(1000);
  if (SerialAT.available()) {
    String response = SerialAT.readString();
    Serial.println("📨 SMS storage: " + response);
  }
  
  // Enable SMS notifications
  SerialAT.println("AT+CNMI=1,2,0,0,0");
  delay(1000);
  if (SerialAT.available()) {
    String response = SerialAT.readString();
    Serial.println("📨 SMS notifications: " + response);
  }
  
  Serial.println("✅ SIM800L initialization complete");
}

void checkForPaymentSMS() {
  Serial.println("🔍 Checking for SMS...");
  
  // Clear any pending data
  while (SerialAT.available()) {
    SerialAT.read();
  }
  
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(3000);  // Longer delay for response
  
  String response = "";
  unsigned long startTime = millis();
  
  // Read response with timeout
  while (millis() - startTime < 5000) {
    if (SerialAT.available()) {
      response += SerialAT.readString();
      delay(100);
    }
  }
  
  Serial.println("📨 Raw SMS response length: " + String(response.length()));
  
  if (response.length() > 20) {
    Serial.println("📨 SMS Response: " + response);
    
    if (response.indexOf("PAY") != -1) {
      Serial.println("💰 Payment SMS detected!");
      processPaymentSMS(response);
      deleteSMS();
    } else if (response.indexOf("OK") != -1 && response.indexOf("+CMGL:") == -1) {
      Serial.println("📭 No unread SMS");
    } else {
      Serial.println("📨 SMS found but no PAY command");
    }
  } else {
    Serial.println("📭 No SMS response");
  }
}

void processPaymentSMS(String sms) {
  Serial.println("📱 Processing SMS payment request...");
  Serial.println("📱 Full SMS: " + sms);
  
  String phoneNumber = extractPhoneNumber(sms);
  int payIndex = sms.indexOf("PAY");
  
  if (payIndex == -1) {
    Serial.println("❌ No PAY command found in SMS");
    return;
  }
  
  String paymentData = sms.substring(payIndex);
  
  // Clean payment data
  int okIndex = paymentData.indexOf("OK");
  if (okIndex != -1) {
    paymentData = paymentData.substring(0, okIndex);
  }
  paymentData.trim();
  paymentData.replace("\n", "");
  paymentData.replace("\r", "");
  
  Serial.println("📱 Cleaned payment: " + paymentData);
  
  // Parse payment details
  int amount = extractAmount(paymentData);
  String currency = extractCurrency(paymentData);
  String targetPump = extractPumpId(paymentData);
  
  Serial.println("📱 Phone: " + phoneNumber);
  Serial.println("💰 Amount: " + String(amount) + " " + currency);
  Serial.println("🚰 Target: " + targetPump);
  
  if (targetPump == pumpId || targetPump == "") {  // Accept empty pump ID
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
    Serial.println("❌ Wrong pump ID: " + targetPump + " (expected: " + pumpId + ")");
  }
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
  SerialAT.println("AT+CMGD=1,4");  // Delete all read SMS
  delay(1000);
  Serial.println("🗑️ SMS deleted");
}
