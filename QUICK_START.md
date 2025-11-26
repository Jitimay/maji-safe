# MajiSafe Quick Start

## 🚀 One Command Launch

```bash
cd /home/josh/Kiro/MajiSafe
./start.sh
```

## 🌐 Access Points

- **MetaMask Interface**: http://localhost:8002/metamask_test_interface.html
- **DKG Dashboard**: http://localhost:8002/dkg_dashboard.html
- **Test Interface**: http://localhost:8002/test_interface.html
- **DKG Bridge API**: http://localhost:5002

## 🦊 MetaMask Setup

1. **Get DEV Tokens**: https://apps.moonbeam.network/moonbase-alpha/faucet/
2. **Connect MetaMask** → Auto-switches to Moonbase Alpha
3. **Buy Water Credits** → Creates Knowledge Assets

## 📱 ESP32 Testing

**Arduino Serial Monitor Commands:**
```
test                     # Test relay on GPIO 2
sms:PAY 5000 BIF PUMP001  # Simulate SMS payment
wifi                     # Reconnect WiFi
status                   # Show system status
```

## 🔗 Complete Flow

1. **MetaMask** → Buy water with DEV tokens
2. **DKG Bridge** → Creates Knowledge Asset
3. **OriginTrail DKG** → Stores verifiable record
4. **ESP32** → Activates pump relay
5. **Dashboard** → View all transactions

## 🛑 Stop Services

Press `Ctrl+C` in the terminal running `./start.sh`
