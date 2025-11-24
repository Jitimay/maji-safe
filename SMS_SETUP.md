# Real SMS Integration Setup

## 🎯 Goal: Send SMS to +25766303339 → AI processes → Web3 payment → ESP32 activation

## Setup Steps:

### 1. SMS Service Setup (Choose One):

**Option A: Twilio**
```bash
# Sign up at twilio.com
# Get phone number +25766303339
# Get Account SID and Auth Token
```

**Option B: Africa's Talking (Better for African numbers)**
```bash
# Sign up at africastalking.com
# Get Burundi number +25766303339
# Get API key
```

**Option C: Local SIM Card (Simplest)**
```bash
# Insert SIM card with +25766303339 into USB modem
# Use AT commands to read SMS
```

### 2. Configure SMS Receiver:

Edit `src/ai-bridge/sms_receiver.py`:
```python
# Add your credentials
self.sms_service_url = "YOUR_SMS_API_URL"
self.auth_token = "YOUR_AUTH_TOKEN"
self.phone_number = "+25766303339"
self.private_key = "YOUR_METAMASK_PRIVATE_KEY"
```

### 3. Test Flow:

1. **Start SMS Receiver**:
```bash
cd src/ai-bridge
source venv/bin/activate
python sms_receiver.py
```

2. **Send Test SMS**:
```
To: +25766303339
Message: PAY 5000 BIF PUMP001
```

3. **Expected Flow**:
```
SMS received → AI parses → Web3 payment → ESP32 activation
```

## 🧪 Quick Test (Simulated):

```bash
python3 sms_receiver.py
```

This will simulate receiving your SMS and show the complete flow!
