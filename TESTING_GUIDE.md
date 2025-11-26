# MajiSafe DKG Architecture Testing Guide

## 🚀 Quick Start Testing

### Step 1: Start the Architecture
```bash
cd /home/josh/Kiro/MajiSafe
python start_majisafe_dkg.py
```

### Step 2: Run Test Suite
```bash
# In another terminal
python test_majisafe_dkg.py
```

## 🧪 Test Components

### 1. **DKG Bridge Status Test**
- Checks if DKG Bridge is running on port 5002
- Validates MCP tools integration
- Confirms Knowledge Assets database

### 2. **SMS Payment Processing Test**
- Simulates SMS: "PAY 5000 BIF PUMP001"
- Creates Knowledge Asset in OriginTrail DKG
- Returns UAL (Universal Asset Locator)
- Generates verification hash

### 3. **Knowledge Assets Retrieval Test**
- Fetches all created Knowledge Assets
- Displays water dispensing records
- Shows UAL references

### 4. **Asset Verification Test**
- Verifies Knowledge Asset integrity
- Checks tamper-proof records
- Validates DKG storage

### 5. **Web Dashboard Test**
- Tests DKG Dashboard accessibility
- Confirms visualization components

### 6. **Load Testing**
- Multiple simultaneous SMS payments
- Tests system scalability
- Validates concurrent processing

## 🔗 Manual Testing URLs

### DKG Bridge API Endpoints:
```
http://localhost:5002/status
http://localhost:5002/knowledge-assets
http://localhost:5002/process-sms
http://localhost:5002/verify-asset/{UAL}
```

### Web Dashboard:
```
http://localhost:8001/dkg_dashboard.html
```

## 📱 SMS Testing Format

Send POST to `http://localhost:5002/process-sms`:
```json
{
  "phone": "+250788123456",
  "amount": 5000,
  "currency": "BIF",
  "pump_id": "PUMP001",
  "coordinates": {"lat": -1.9441, "lng": 30.0619},
  "timestamp": 1640995200,
  "device_id": "ESP32_001"
}
```

## 🔍 Expected Test Results

### ✅ Success Indicators:
- All 6 tests pass
- Knowledge Assets created with UAL
- Verification hashes generated
- DKG Dashboard shows assets
- No HTTP errors

### ❌ Failure Indicators:
- DKG Bridge offline (port 5002)
- Web server not running (port 8001)
- Database connection errors
- Missing OriginTrail DKG node

## 🛠️ Troubleshooting

### Common Issues:

1. **DKG Bridge Not Starting**
   ```bash
   cd src/ai-bridge
   source venv/bin/activate
   pip install -r requirements_dkg.txt
   python majisafe_dkg_bridge.py
   ```

2. **Web Dashboard 404**
   ```bash
   cd src/web
   python -m http.server 8001
   ```

3. **OriginTrail DKG Node Missing**
   ```bash
   npm install -g @origintrail/dkg
   dkg-node start
   ```

4. **Database Errors**
   ```bash
   rm src/ai-bridge/majisafe_dkg.db
   # Restart DKG Bridge to recreate
   ```

## 🌊 End-to-End Testing

### Complete Flow Test:
1. **SMS Input**: Send payment SMS to ESP32
2. **IoT Processing**: ESP32 forwards to DKG Bridge
3. **Knowledge Asset**: DKG Bridge creates verifiable record
4. **DKG Storage**: Asset stored in OriginTrail DKG
5. **Blockchain Anchor**: Cryptographic fingerprint on Base L2
6. **Web Visualization**: View in DKG Dashboard

### Real Hardware Test:
1. Flash ESP32 with `majisafe_dkg_pump.ino`
2. Insert SIM card and connect pump
3. Send real SMS: "PAY 5000 BIF PUMP001"
4. Verify pump activation and water flow
5. Check Knowledge Asset creation in dashboard

## 📊 Performance Metrics

### Target Performance:
- SMS processing: < 5 seconds
- Knowledge Asset creation: < 10 seconds
- DKG storage: < 30 seconds
- Verification: < 2 seconds
- Dashboard load: < 3 seconds

### Scalability Targets:
- 100+ SMS payments per hour
- 1000+ Knowledge Assets stored
- Multiple pump locations
- Real-time monitoring

## 🎯 Production Readiness Checklist

- [ ] All tests pass (6/6)
- [ ] DKG node connectivity confirmed
- [ ] ESP32 hardware tested
- [ ] SMS gateway configured
- [ ] Base L2 contract deployed
- [ ] Dashboard accessible
- [ ] Knowledge Assets verifiable
- [ ] Performance targets met
