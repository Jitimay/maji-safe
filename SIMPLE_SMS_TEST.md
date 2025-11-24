# Simple SMS Test with ESP32 SIM800L

## 🎯 Your Setup:
- **ESP32**: LilyGO TTGO T-Call with SIM800L
- **SIM Card**: +25766303339
- **AI Bridge**: Your laptop (Python)
- **Flow**: SMS → ESP32 → AI → Web3 → ESP32 → Pump

## 📋 Setup Steps:

### 1. Prepare ESP32:
```arduino
// In sms_receiver.ino, update:
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aiBridgeURL = "http://YOUR_LAPTOP_IP:5000/process-sms";
```

### 2. Start AI Bridge:
```bash
cd src/ai-bridge
source venv/bin/activate
python simple_sms_ai.py
```

### 3. Flash ESP32:
- Upload `sms_receiver.ino` to your ESP32
- Insert SIM card with +25766303339
- Connect pump to GPIO pin 2

### 4. Test Flow:
```
1. Send SMS to +25766303339: "PAY 5000 BIF PUMP001"
2. ESP32 receives SMS
3. ESP32 forwards to AI Bridge (your laptop)
4. AI makes Web3 payment automatically
5. AI sends "activate" back to ESP32
6. ESP32 activates pump for 10 seconds
```

## 🧪 Expected Output:

**ESP32 Serial Monitor:**
```
📱 Payment SMS received!
📞 From: +250788123456
💬 Message: PAY 5000 BIF PUMP001
📡 Forwarding to AI
🤖 AI Response: {"status":"success","message":"activate"}
🚰 ACTIVATING PUMP!
🛑 Pump stopped
```

**AI Bridge Console:**
```
📱 SMS from +250788123456: PAY 5000 BIF PUMP001
💰 Payment: 5000.0 BIF = 0.001735 ETH
✅ Payment successful: 0x123...
🚰 Activating pump: PUMP001
```

## 🚀 Ready to Test!
Send SMS to +25766303339 and watch the magic happen!
