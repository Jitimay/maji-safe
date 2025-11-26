/*
 * MajiSafe DKG Pump Controller
 * Enhanced ESP32 firmware with OriginTrail DKG integration
 * Sends verifiable pump data to DKG Bridge
 */

#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define PUMP_PIN             2
#define FLOW_SENSOR_PIN      13
#define GPS_TX               16
#define GPS_RX               17

#include <SoftwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>

SoftwareSerial SerialAT(MODEM_RX, MODEM_TX);
SoftwareSerial SerialGPS(GPS_RX, GPS_TX);
TinyGPSPlus gps;

// Network configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* dkgBridgeURL = "http://YOUR_LAPTOP_IP:5002/process-sms";

// Pump data
volatile int flowPulses = 0;
float totalLitersDispensed = 0;
String pumpId = "PUMP001";
unsigned long pumpStartTime = 0;
bool pumpActive = false;

// GPS coordinates
double latitude = 0.0;
double longitude = 0.0;

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(9600);
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  initModem();
  initWiFi();
  initGPS();
  
  Serial.println("🌊 MajiSafe DKG Pump Controller Ready");
  Serial.println("🔗 OriginTrail DKG Integration Active");
}

void loop() {
  // Update GPS coordinates
  updateGPS();
  
  // Check for SMS payments
  if (millis() % 5000 == 0) {
    checkForPaymentSMS();
  }
  
  // Monitor active pump
  if (pumpActive) {
    monitorPumpOperation();
  }
  
  delay(100);
}

void initWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("📶 WiFi Connected: " + WiFi.localIP().toString());
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
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  
  Serial.println("📱 GSM Module Ready");
}

void initGPS() {
  Serial.println("🛰️ GPS Module Initializing...");
}

void updateGPS() {
  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
    }
  }
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
  Serial.println("💰 Processing SMS payment...");
  
  // Extract payment data
  String phoneNumber = extractPhoneNumber(sms);
  String paymentCommand = extractPaymentCommand(sms);
  
  // Parse: "PAY 5000 BIF PUMP001"
  int amount = extractAmount(paymentCommand);
  String currency = extractCurrency(paymentCommand);
  String targetPump = extractPumpId(paymentCommand);
  
  if (targetPump == pumpId) {
    // Create enhanced SMS data for DKG Bridge
    DynamicJsonDocument smsData(1024);
    smsData["phone"] = phoneNumber;
    smsData["amount"] = amount;
    smsData["currency"] = currency;
    smsData["pump_id"] = pumpId;
    smsData["coordinates"]["lat"] = latitude;
    smsData["coordinates"]["lng"] = longitude;
    smsData["timestamp"] = millis();
    smsData["device_id"] = WiFi.macAddress();
    
    // Send to DKG Bridge for processing
    sendToDKGBridge(smsData);
    
    // Activate pump (duration based on payment amount)
    int pumpDuration = calculatePumpDuration(amount, currency);
    activatePump(pumpDuration);
  }
}

void sendToDKGBridge(DynamicJsonDocument& smsData) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(dkgBridgeURL);
    http.addHeader("Content-Type", "application/json");
    
    String payload;
    serializeJson(smsData, payload);
    
    Serial.println("🔗 Sending to DKG Bridge: " + payload);
    
    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      String response = http.getString();
      Serial.println("✅ DKG Bridge Response: " + response);
      
      // Parse response for UAL and verification
      DynamicJsonDocument responseDoc(1024);
      deserializeJson(responseDoc, response);
      
      if (responseDoc["success"]) {
        Serial.println("🔗 Knowledge Asset UAL: " + responseDoc["ual"].as<String>());
        Serial.println("🔐 Verification Hash: " + responseDoc["verification_hash"].as<String>());
      }
    } else {
      Serial.println("❌ DKG Bridge Error: " + String(httpCode));
    }
    
    http.end();
  }
}

void activatePump(int seconds) {
  Serial.println("🚰 Activating pump for " + String(seconds) + " seconds");
  
  pumpActive = true;
  pumpStartTime = millis();
  flowPulses = 0;
  
  digitalWrite(PUMP_PIN, HIGH);
  Serial.println("⚡ Pump ON");
  
  // Run pump for specified duration
  delay(seconds * 1000);
  
  digitalWrite(PUMP_PIN, LOW);
  pumpActive = false;
  
  // Calculate total liters dispensed
  float litersDispensed = flowPulses * 0.1; // 0.1L per pulse
  totalLitersDispensed += litersDispensed;
  
  Serial.println("🛑 Pump OFF");
  Serial.println("💧 Dispensed: " + String(litersDispensed) + "L");
  Serial.println("📊 Total: " + String(totalLitersDispensed) + "L");
  
  // Send completion data to DKG Bridge
  sendPumpCompletionData(litersDispensed, seconds);
}

void sendPumpCompletionData(float liters, int duration) {
  DynamicJsonDocument completionData(512);
  completionData["event"] = "pump_completion";
  completionData["pump_id"] = pumpId;
  completionData["liters_dispensed"] = liters;
  completionData["duration_seconds"] = duration;
  completionData["flow_pulses"] = flowPulses;
  completionData["coordinates"]["lat"] = latitude;
  completionData["coordinates"]["lng"] = longitude;
  completionData["timestamp"] = millis();
  
  // Send to DKG Bridge for Knowledge Asset update
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://YOUR_LAPTOP_IP:5002/pump-completion");
    http.addHeader("Content-Type", "application/json");
    
    String payload;
    serializeJson(completionData, payload);
    
    int httpCode = http.POST(payload);
    Serial.println("📊 Completion data sent: " + String(httpCode));
    
    http.end();
  }
}

void monitorPumpOperation() {
  // Real-time monitoring during pump operation
  static unsigned long lastMonitor = 0;
  
  if (millis() - lastMonitor > 1000) {
    float currentFlow = flowPulses * 0.1;
    Serial.println("💧 Current flow: " + String(currentFlow) + "L");
    lastMonitor = millis();
  }
}

int calculatePumpDuration(int amount, String currency) {
  // Calculate pump duration based on payment amount
  float usdValue = convertToUSD(amount, currency);
  return (int)(usdValue * 10); // 10 seconds per USD
}

float convertToUSD(int amount, String currency) {
  if (currency == "BIF") return amount * 0.000000347;
  if (currency == "RWF") return amount * 0.000000312;
  if (currency == "KES") return amount * 0.0000065;
  return amount * 0.0004; // Default USD
}

String extractPhoneNumber(String sms) {
  int startIndex = sms.indexOf("+");
  if (startIndex == -1) return "";
  int endIndex = sms.indexOf("\"", startIndex);
  return sms.substring(startIndex, endIndex);
}

String extractPaymentCommand(String sms) {
  int payIndex = sms.indexOf("PAY");
  if (payIndex == -1) return "";
  return sms.substring(payIndex);
}

int extractAmount(String command) {
  int spaceIndex = command.indexOf(" ");
  int nextSpace = command.indexOf(" ", spaceIndex + 1);
  return command.substring(spaceIndex + 1, nextSpace).toInt();
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

void flowPulseCounter() {
  flowPulses++;
}

void deleteSMS() {
  SerialAT.println("AT+CMGD=1,4");
  delay(1000);
}
