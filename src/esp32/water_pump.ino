/*
 * MajiSafe ESP32 - SMS Payment Receiver & Pump Controller
 * Receives SMS payments from rural users, forwards to AI Bridge
 */

#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define PUMP_PIN             2
#define FLOW_SENSOR_PIN      13

#include <SoftwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>

SoftwareSerial SerialAT(MODEM_RX, MODEM_TX);

// AI Bridge connection
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aiBridgeURL = "http://YOUR_LAPTOP_IP:5000/sms-payment";

volatile int flowPulses = 0;
unsigned long lastSMSCheck = 0;
bool pumpActive = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  initModem();
  initWiFi();
  
  Serial.println("MajiSafe SMS Payment System Ready");
}

void loop() {
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
  
  SerialAT.println("AT");
  delay(1000);
  SerialAT.println("AT+CMGF=1"); // SMS text mode
  delay(1000);
}

void checkForPaymentSMS() {
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  
  String response = "";
  while (SerialAT.available()) {
    response += SerialAT.readString();
  }
  
  if (response.indexOf("PAY") != -1) {
    processPaymentSMS(response);
    deleteSMS();
  }
}

void processPaymentSMS(String sms) {
  // Extract payment info: "PAY 1000 PUMP001"
  int payIndex = sms.indexOf("PAY");
  if (payIndex == -1) return;
  
  String paymentData = sms.substring(payIndex);
  String phoneNumber = extractPhoneNumber(sms);
  
  // Forward to AI Bridge
  forwardToAIBridge(paymentData, phoneNumber);
}

String extractPhoneNumber(String sms) {
  int startIndex = sms.indexOf("+");
  if (startIndex == -1) return "";
  
  int endIndex = sms.indexOf("\"", startIndex);
  return sms.substring(startIndex, endIndex);
}

void forwardToAIBridge(String payment, String phone) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(aiBridgeURL);
    http.addHeader("Content-Type", "application/json");
    
    String payload = "{\"payment\":\"" + payment + "\",\"phone\":\"" + phone + "\"}";
    
    int httpCode = http.POST(payload);
    if (httpCode > 0) {
      Serial.println("Payment forwarded to AI Bridge: " + String(httpCode));
    }
    
    http.end();
  }
}

void activatePump(int seconds) {
  Serial.println("🚰 Activating pump via RELAY for " + String(seconds) + " seconds");
  
  pumpActive = true;
  digitalWrite(PUMP_PIN, HIGH);  // Turn ON relay
  Serial.println("⚡ Relay ON - Pump Running");
  
  delay(seconds * 1000);
  
  digitalWrite(PUMP_PIN, LOW);   // Turn OFF relay
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
