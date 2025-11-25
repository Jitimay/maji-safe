/*
 * MajiSafe ESP32 SMS Receiver
 * SIM card number: +25766303339
 * Receives SMS payments and forwards to AI Bridge
 */

#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define PUMP_PIN             2

#include <SoftwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>

SoftwareSerial SerialAT(MODEM_RX, MODEM_TX);

// WiFi and AI Bridge settings
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aiBridgeURL = "http://192.168.1.100:5000/process-sms";  // Your laptop IP

unsigned long lastSMSCheck = 0;
bool pumpActive = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  
  initModem();
  initWiFi();
  
  Serial.println("📱 MajiSafe SMS Receiver Ready");
  Serial.println("📞 Phone: +25766303339");
  Serial.println("💬 Send: PAY 5000 BIF PUMP001");
}

void loop() {
  // Check for SMS every 5 seconds
  if (millis() - lastSMSCheck > 5000) {
    checkForSMS();
    lastSMSCheck = millis();
  }
  
  delay(100);
}

void initWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("✅ WiFi Connected: " + WiFi.localIP().toString());
}

void initModem() {
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  
  SerialAT.begin(115200);
  delay(3000);
  
  // Initialize modem
  SerialAT.println("AT");
  delay(1000);
  SerialAT.println("AT+CMGF=1"); // SMS text mode
  delay(1000);
  
  Serial.println("📡 Modem initialized");
}

void checkForSMS() {
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  
  String response = "";
  while (SerialAT.available()) {
    response += SerialAT.readString();
  }
  
  if (response.indexOf("PAY") != -1) {
    Serial.println("📱 Payment SMS received!");
    processSMS(response);
    deleteSMS();
  }
}

void processSMS(String smsData) {
  // Extract phone number and message
  String phoneNumber = extractPhoneNumber(smsData);
  String message = extractMessage(smsData);
  
  Serial.println("📞 From: " + phoneNumber);
  Serial.println("💬 Message: " + message);
  
  // Forward to AI Bridge
  forwardToAI(phoneNumber, message);
}

String extractPhoneNumber(String sms) {
  int startIndex = sms.indexOf("+");
  if (startIndex == -1) return "Unknown";
  
  int endIndex = sms.indexOf("\"", startIndex);
  return sms.substring(startIndex, endIndex);
}

String extractMessage(String sms) {
  int payIndex = sms.indexOf("PAY");
  if (payIndex == -1) return "";
  
  int endIndex = sms.indexOf("\n", payIndex);
  if (endIndex == -1) endIndex = sms.length();
  
  return sms.substring(payIndex, endIndex);
}

void forwardToAI(String phone, String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(aiBridgeURL);
    http.addHeader("Content-Type", "application/json");
    
    String payload = "{";
    payload += "\"phone\":\"" + phone + "\",";
    payload += "\"message\":\"" + message + "\"";
    payload += "}";
    
    Serial.println("📡 Forwarding to AI: " + payload);
    
    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("🤖 AI Response: " + response);
      
      // Check if pump should activate
      if (response.indexOf("activate") != -1) {
        activatePump();
      }
    } else {
      Serial.println("❌ AI Bridge connection failed");
    }
    
    http.end();
  }
}

void activatePump() {
  Serial.println("🚰 ACTIVATING PUMP!");
  
  pumpActive = true;
  digitalWrite(PUMP_PIN, HIGH);
  
  delay(10000); // Run for 10 seconds
  
  digitalWrite(PUMP_PIN, LOW);
  pumpActive = false;
  
  Serial.println("🛑 Pump stopped");
}

void deleteSMS() {
  SerialAT.println("AT+CMGD=1,4"); // Delete all read SMS
  delay(1000);
}
