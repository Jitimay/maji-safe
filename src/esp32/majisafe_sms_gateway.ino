#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// --- Modem Pins ---
#define MODEM_RST        5
#define MODEM_PWRKEY     4
#define MODEM_POWER_ON   23
#define MODEM_TX         27
#define MODEM_RX         26
#define MODEM_BAUD       115200
#define PUMP_PIN         2

// --- Serial for AT Commands ---
HardwareSerial SerialAT(1);

// --- WiFi Credentials ---
const char* ssid = "Josh";
const char* password = "Jitimay$$";

// --- MajiSafe AI Bridge URL ---
const char* aiBridgeURL = "http://192.168.155.181:5001/process-sms";

bool pumpActive = false;

// --- Function to Power On Modem ---
void powerOnModem() {
  pinMode(MODEM_PWRKEY, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  Serial.println("Powering on modem...");
  digitalWrite(MODEM_POWER_ON, HIGH);
  digitalWrite(MODEM_RST, HIGH);

  digitalWrite(MODEM_PWRKEY, HIGH); delay(100);
  digitalWrite(MODEM_PWRKEY, LOW); delay(1000);
  digitalWrite(MODEM_PWRKEY, HIGH);
  Serial.println("Modem power sequence complete.");
  delay(2000);
}

// --- Function to Connect to WiFi ---
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 40) {
    delay(500); Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Failed");
  }
}

// --- Process Payment SMS with MajiSafe AI ---
String processPayment(String smsContent, String sender) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi disconnected");
    return "Network error. Try again.";
  }

  HTTPClient http;
  http.begin(aiBridgeURL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(30000);

  // Create JSON payload for MajiSafe AI
  String jsonData = "{\"phone\":\"" + sender + "\",\"message\":\"" + smsContent + "\"}";
  Serial.println("📡 Sending to AI: " + jsonData);

  int httpCode = http.POST(jsonData);
  String response = "";

  if (httpCode == 200) {
    String rawResponse = http.getString();
    Serial.println("🤖 AI Response: " + rawResponse);

    // Parse JSON response
    const size_t capacity = JSON_OBJECT_SIZE(4) + 512;
    DynamicJsonDocument doc(capacity);
    
    DeserializationError error = deserializeJson(doc, rawResponse);
    
    if (!error) {
      String status = doc["status"].as<String>();
      String message = doc["message"].as<String>();
      
      if (status == "success" && message == "activate") {
        // Payment successful - activate pump
        String txHash = doc["tx_hash"].as<String>();
        String pumpId = doc["pump_id"].as<String>();
        
        Serial.println("💰 Payment confirmed: " + txHash);
        Serial.println("🚰 Activating pump: " + pumpId);
        
        activatePump();
        
        response = "Payment successful! " + txHash.substring(0, 10) + "... Water dispensed.";
      } else {
        // Payment failed
        response = doc["message"].as<String>();
        if (response.length() == 0) response = "Payment failed";
      }
    } else {
      Serial.println("❌ JSON parse error");
      response = "System error. Try again.";
    }
  } else {
    Serial.println("❌ HTTP Error: " + String(httpCode));
    response = "Payment system offline.";
  }

  http.end();
  
  // Truncate for SMS
  if (response.length() > 150) {
    response = response.substring(0, 150);
  }
  
  return response;
}

// --- Activate Water Pump ---
void activatePump() {
  Serial.println("🚰 PUMP ACTIVATING!");
  pumpActive = true;
  digitalWrite(PUMP_PIN, HIGH);
  
  delay(10000); // Run for 10 seconds
  
  digitalWrite(PUMP_PIN, LOW);
  pumpActive = false;
  Serial.println("🛑 Pump stopped");
}

// --- Send SMS Response ---
void sendSMS(String number, String message) {
  Serial.println("📱 Sending SMS to: " + number);
  SerialAT.println("AT+CMGF=1");
  delay(300);
  
  while(SerialAT.available()) { SerialAT.read(); }
  
  SerialAT.println("AT+CMGS=\"" + number + "\"");
  delay(500);
  
  SerialAT.print(message);
  Serial.println("📤 SMS: " + message);
  delay(100);
  SerialAT.write(0x1A); // Ctrl+Z
  delay(5000);
  
  // Read response
  while(SerialAT.available()) {
    String resp = SerialAT.readStringUntil('\n');
    resp.trim();
    if (resp.length() > 0) {
      Serial.println("📱 " + resp);
    }
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("\n🚰 MajiSafe SMS Payment Gateway Starting...");
  delay(1000);

  powerOnModem();
  
  SerialAT.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(1000);

  Serial.println("📡 Configuring modem...");
  SerialAT.println("ATE0");
  delay(500);
  SerialAT.println("AT+CMGF=1");
  delay(500);
  SerialAT.println("AT+CNMI=2,1,0,0,0");
  delay(500);

  // Clear buffer
  while(SerialAT.available()) { SerialAT.read(); }
  
  connectWiFi();
  
  Serial.println("\n🚰 MajiSafe Ready!");
  Serial.println("📱 Send: PAY 5000 BIF PUMP001");
  Serial.println("💧 Phone: +25766303339");
}

// --- Main Loop ---
void loop() {
  if (SerialAT.available()) {
    String modemResponse = SerialAT.readStringUntil('\n');
    modemResponse.trim();

    if (modemResponse.length() > 0) {
      Serial.println("📡 " + modemResponse);
    }

    // Check for new SMS notification
    if (modemResponse.startsWith("+CMTI:")) {
      int indexStart = modemResponse.lastIndexOf(',');
      if (indexStart != -1) {
        String indexStr = modemResponse.substring(indexStart + 1);
        indexStr.trim();
        int index = indexStr.toInt();

        if (index > 0) {
          Serial.println("📥 New SMS at index: " + String(index));

          // Read SMS
          SerialAT.println("AT+CMGR=" + String(index));
          delay(750);

          String smsContent = "";
          String sender = "";
          bool readingContent = false;
          bool okReceived = false;

          unsigned long readStart = millis();
          while (millis() - readStart < 5000) {
            if (SerialAT.available()) {
              String line = SerialAT.readStringUntil('\n');
              line.trim();

              if (line.length() > 0) {
                Serial.println(">> " + line);

                if (line.startsWith("+CMGR:")) {
                  // Extract sender
                  int q3 = line.indexOf('"', line.indexOf('"', line.indexOf('"') + 1) + 1);
                  int q4 = line.indexOf('"', q3 + 1);
                  if (q3 != -1 && q4 != -1) {
                    sender = line.substring(q3 + 1, q4);
                    Serial.println("📞 From: " + sender);
                  }
                  readingContent = true;
                } else if (readingContent && !line.startsWith("OK")) {
                  if (smsContent.length() > 0) smsContent += "\n";
                  smsContent += line;
                } else if (line.startsWith("OK")) {
                  okReceived = true;
                  break;
                }
              }
              readStart = millis();
            }
            delay(10);
          }

          // Process SMS if valid
          smsContent.trim();
          Serial.println("💬 Content: " + smsContent);

          if (sender.length() > 0 && smsContent.length() > 0) {
            // Check if it's a payment SMS
            smsContent.toUpperCase(); // Fix: toUpperCase() modifies in place
            if (smsContent.startsWith("PAY")) {
              Serial.println("💰 Processing payment...");
              String reply = processPayment(smsContent, sender);
              sendSMS(sender, reply);
            } else {
              // Not a payment SMS
              sendSMS(sender, "Send: PAY [amount] [currency] [pump_id]\nExample: PAY 5000 BIF PUMP001");
            }
          }

          // Delete SMS
          Serial.println("🗑️ Deleting SMS " + String(index));
          SerialAT.println("AT+CMGD=" + String(index));
          delay(500);
          
          while(SerialAT.available()) {
            String delResp = SerialAT.readStringUntil('\n');
            delResp.trim();
            if (delResp.length() > 0) {
              Serial.println("🗑️ " + delResp);
            }
          }
        }
      }
    }
  }
  
  delay(20);
}
