# MajiSafe: Decentralized Water Access System

**"SMS → Web3 → Physical Water Pump"**

## 🌊 Overview
MajiSafe enables rural communities to purchase clean water using SMS payments that trigger Web3 transactions and activate physical water pumps. The system creates verifiable Knowledge Assets on OriginTrail DKG for complete transparency.

## 🏗️ Architecture
- **ESP32 + SIM800L**: Receives SMS, controls water pump
- **Web Interface**: MetaMask integration for Web3 payments
- **DKG Bridge**: Creates Knowledge Assets on OriginTrail
- **Moonbase Alpha**: Blockchain network for payments

## 🚀 Quick Start

### Prerequisites
- Node.js 18+
- Python 3.8+
- Arduino IDE
- MetaMask wallet
- ESP32 with SIM800L module

### 1. One-Command Launch
```bash
cd /home/josh/Kiro/MajiSafe
./start.sh
```

### 2. Hardware Setup
- Flash `src/esp32/majisafe_working_sms_ui.ino` to ESP32
- Connect SIM800L module (pins defined in code)
- Connect relay to GPIO 2 for pump control

### 3. Access Interfaces
- **Modern UI**: http://localhost:8002/majisafe_modern_ui.html
- **DKG Dashboard**: http://localhost:8002/dkg_dashboard.html
- **DKG Bridge API**: http://localhost:5002/status

## 📱 Complete Flow

### 1. SMS Payment Request
Send SMS to SIM card: `PAY 5000 BIF PUMP001`

### 2. UI Activation
- ESP32 receives SMS → Stores payment request
- Web UI detects SMS → Button becomes clickable
- User connects MetaMask → Ready for payment

### 3. Web3 Payment
- Click "Confirm Web3 Payment" → MetaMask transaction
- Moonbase Alpha confirms → Creates Knowledge Asset
- ESP32 receives confirmation → Activates pump

### 4. Water Dispensing
- Relay activates pump for calculated duration
- Flow sensor monitors water dispensed
- System resets for next payment

## 🛠️ Project Structure

```
MajiSafe/
├── src/
│   ├── esp32/
│   │   └── majisafe_working_sms_ui.ino    # Main ESP32 firmware
│   ├── web/
│   │   ├── majisafe_modern_ui.html        # Modern web interface
│   │   ├── metamask_integration.js        # MetaMask Web3 integration
│   │   └── dkg_dashboard.html            # Knowledge Assets dashboard
│   └── ai-bridge/
│       ├── majisafe_dkg_bridge.py        # Main DKG bridge service
│       └── dkg_agent_simple.py           # Simplified DKG agent
├── start.sh                              # One-command launcher
└── README.md                             # This file
```

## 🔧 Configuration

### ESP32 Settings
Update in `majisafe_working_sms_ui.ino`:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### MetaMask Setup
1. Install MetaMask browser extension
2. Add Moonbase Alpha network (auto-configured)
3. Get DEV tokens from faucet: https://apps.moonbeam.network/moonbase-alpha/faucet/

## 🌟 Features

### ✅ Real Hardware Integration
- SIM800L GSM module for SMS reception
- ESP32 microcontroller with WiFi
- Relay control for water pump
- Flow sensor for water measurement

### ✅ Blockchain Integration
- Moonbase Alpha network (Polkadot parachain)
- MetaMask wallet integration
- Real DEV token transactions
- Transaction verification and confirmation

### ✅ Knowledge Assets
- OriginTrail DKG integration
- Verifiable water dispensing records
- Tamper-proof audit trail
- Universal Asset Locators (UAL)

### ✅ Modern Web Interface
- Real-time system monitoring
- Professional UI with animations
- Mobile-responsive design
- Live connection status indicators

## 🧪 Testing

### Manual SMS Test
Send real SMS to your SIM card number:
```
PAY 5000 BIF PUMP001
```

### System Health Check
```bash
# Check ESP32 status
curl http://192.168.1.30/sms-status

# Check DKG Bridge
curl http://localhost:5002/status
```

## 🌍 Impact
- **Transparent water distribution** in rural Africa
- **Corruption-free payments** via blockchain
- **Verifiable water access** through Knowledge Assets
- **SMS accessibility** for basic mobile phones

## 🔒 Security
- Web3 payment confirmation required before pump activation
- Knowledge Assets provide tamper-proof records
- SMS → Blockchain → Hardware verification chain
- No pump activation without confirmed payment

## 📊 Monitoring
- Real-time transaction statistics
- Water dispensing metrics
- System health indicators
- Knowledge Asset creation tracking

---

**Built with OriginTrail DKG, Moonbase Alpha, and ESP32 hardware integration.**
