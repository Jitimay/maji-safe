/*
 * Maji-Vibe ESP32 Water Pump Controller
 * LilyGO T-Call SIM800L with water pump control
 */

#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define PUMP_PIN             2
#define FLOW_SENSOR_PIN      13

#include <SoftwareSerial.h>
SoftwareSerial SerialAT(MODEM_RX, MODEM_TX);

volatile int flowPulses = 0;
unsigned long lastSMSCheck = 0;
bool pumpActive = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize pump control
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  
  // Initialize flow sensor
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulseCounter, FALLING);
  
  // Initialize modem
  initModem();
  
  Serial.println("Maji-Vibe Water Pump Ready");
}

void loop() {
  // Check for SMS every 5 seconds
  if (millis() - lastSMSCheck > 5000) {
    checkSMS();
    lastSMSCheck = millis();
  }
  
  // Monitor pump status
  if (pumpActive) {
    monitorPump();
  }
  
  delay(100);
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

void checkSMS() {
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(2000);
  
  String response = "";
  while (SerialAT.available()) {
    response += SerialAT.readString();
  }
  
  if (response.indexOf("P") != -1) {
    processSMSCommand(response);
    deleteSMS();
  }
}

void processSMSCommand(String sms) {
  int pIndex = sms.indexOf("P");
  if (pIndex != -1 && pIndex + 3 < sms.length()) {
    String creditStr = sms.substring(pIndex + 1, pIndex + 3);
    int credits = creditStr.toInt();
    
    if (credits > 0) {
      activatePump(credits);
    }
  }
}

void activatePump(int credits) {
  Serial.println("Activating pump for " + String(credits) + " credits");
  
  pumpActive = true;
  digitalWrite(PUMP_PIN, HIGH);
  
  // Run pump for credits * 10 seconds
  unsigned long pumpDuration = credits * 10000;
  unsigned long pumpStart = millis();
  
  while (millis() - pumpStart < pumpDuration) {
    delay(100);
  }
  
  digitalWrite(PUMP_PIN, LOW);
  pumpActive = false;
  
  Serial.println("Pump deactivated. Flow: " + String(flowPulses) + " pulses");
  flowPulses = 0;
}

void monitorPump() {
  // Monitor flow rate and pump health
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
  SerialAT.println("AT+CMGD=1,4"); // Delete all read SMS
  delay(1000);
}
