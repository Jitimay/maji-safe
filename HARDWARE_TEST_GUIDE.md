# MajiSafe Hardware Testing Guide
## ESP32 + Relay + UI Testing

### 🔧 Hardware Setup

1. **ESP32 Connections:**
   ```
   GPIO 2  → Relay IN (Signal)
   GPIO 13 → LED (Optional indicator)
   VCC     → Relay VCC (3.3V or 5V)
   GND     → Relay GND
   ```

2. **Relay Connections:**
   ```
   Relay COM → Pump Positive
   Relay NO  → Power Supply Positive
   Pump GND  → Power Supply GND
   ```

### 🚀 Step-by-Step Testing

#### Step 1: Start DKG Architecture
```bash
cd /home/josh/Kiro/MajiSafe
python start_majisafe_dkg.py
```
**Expected Output:**
```
🔗 DKG Bridge started on port 5002
🌐 DKG Dashboard started on port 8001
```

#### Step 2: Flash ESP32
1. Open Arduino IDE
2. Load: `src/esp32/majisafe_test_relay.ino`
3. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
4. Update DKG Bridge IP:
   ```cpp
   const char* dkgBridgeURL = "http://192.168.1.XXX:5002/process-sms";
   ```
5. Flash to ESP32

#### Step 3: Open Serial Monitor
1. Arduino IDE → Tools → Serial Monitor
2. Set baud rate: **115200**
3. You should see:
   ```
   🌊 MajiSafe Test - Relay Control
   ================================
   GPIO 2: Relay (Pump Control)
   📶 Connecting to WiFi...
   ✅ WiFi Connected!
   IP: 192.168.1.XXX
   ```

#### Step 4: Test Relay Hardware
In Serial Monitor, type:
```
test
```
**Expected Result:**
- Relay clicks ON for 3 seconds
- GPIO 2 goes HIGH
- LED turns ON (if connected)
- Relay clicks OFF
- Serial shows: "✅ Relay test complete"

#### Step 5: Open Test UI
1. Open browser: `http://localhost:8001/test_interface.html`
2. Click "Check DKG Bridge" - should show ✅ Online
3. Click "Check Dashboard" - should show ✅ Online

#### Step 6: Test SMS Flow
**Option A: Via Web UI**
1. In Test Interface, fill SMS form:
   - Phone: `+250788123456`
   - Amount: `5000`
   - Currency: `BIF`
   - Pump ID: `PUMP001`
2. Click "📱 Send SMS"
3. Watch Serial Monitor for DKG response

**Option B: Via Serial Monitor**
Type in Serial Monitor:
```
sms:PAY 5000 BIF PUMP001
```

**Expected Serial Output:**
```
📱 Processing SMS: PAY 5000 BIF PUMP001
💰 Amount: 5000 BIF
🚰 Target Pump: PUMP001
🔗 Sending to DKG Bridge...
📤 Payload: {"phone":"+250788123456"...}
📥 Response Code: 200
✅ DKG Bridge Success!
🔗 UAL: did:dkg:otp:2043/0x...
🔐 Hash: a1b2c3d4...
🚰 Activating pump for 17 seconds
⚡ RELAY ON - Pump Running
💧 Pumping... 16s remaining
💧 Pumping... 15s remaining
...
🛑 RELAY OFF - Pump Stopped
```

#### Step 7: Verify Knowledge Asset
1. In Test UI, click "📊 Load Assets"
2. Should show your water dispensing event
3. Click "🌐 Open Dashboard"
4. Verify Knowledge Asset appears in DKG Dashboard

### 🧪 Test Commands (Serial Monitor)

| Command | Action |
|---------|--------|
| `test` | Test relay for 3 seconds |
| `sms:PAY 5000 BIF PUMP001` | Simulate SMS payment |
| `relay on` | Turn relay ON |
| `relay off` | Turn relay OFF |
| `wifi` | Reconnect WiFi |
| `status` | Show system status |

### 📊 Expected Test Results

#### ✅ Success Indicators:
- ESP32 connects to WiFi
- Relay activates on GPIO 2
- DKG Bridge receives SMS data
- Knowledge Asset created with UAL
- Verification hash generated
- Web UI shows successful tests

#### ❌ Troubleshooting:

**Relay Not Working:**
- Check GPIO 2 connection
- Verify relay power supply
- Test with multimeter

**WiFi Connection Failed:**
- Check SSID/password
- Verify network connectivity
- Check IP address in code

**DKG Bridge Error:**
- Ensure DKG Bridge running on port 5002
- Check IP address in ESP32 code
- Verify network connectivity

**No Knowledge Asset:**
- Check DKG Bridge logs
- Verify SMS format
- Check database permissions

### 🔍 Monitoring Points

1. **Serial Monitor:** Real-time ESP32 status
2. **Test UI:** Web-based testing interface
3. **DKG Dashboard:** Knowledge Assets visualization
4. **DKG Bridge Logs:** Backend processing status

### 📈 Performance Targets

- SMS processing: < 5 seconds
- Relay activation: < 1 second
- Knowledge Asset creation: < 10 seconds
- UI response: < 2 seconds

### 🎯 Complete Test Checklist

- [ ] ESP32 flashed and connected
- [ ] Relay connected to GPIO 2
- [ ] WiFi connection established
- [ ] DKG Bridge running
- [ ] Test UI accessible
- [ ] Relay test successful
- [ ] SMS simulation works
- [ ] Knowledge Asset created
- [ ] DKG Dashboard shows data
- [ ] End-to-end flow complete

**🎉 Success:** All components working, relay activates on SMS, Knowledge Assets created!
