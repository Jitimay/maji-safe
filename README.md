# MajiSafe: Decentralized Water Broker for Rural Africa

**"Turning DeFi into Drinkable Water"**

## Overview
MajiSafe transforms crypto payments into real-world water access for rural communities with limited connectivity. Using blockchain transactions on basic 2G networks, villagers can buy clean water through transparent, corruption-free distribution.

## Architecture
- **Blockchain (Base L2)**: Processes payments
- **AI Bridge (Python)**: Listens to blockchain events, generates SMS commands
- **ESP32 (LilyGO T-Call SIM800L)**: Receives 1-5 byte commands to activate pumps

## Quick Start

### Prerequisites
- Node.js 18+
- Python 3.8+
- Arduino IDE
- Base testnet wallet

### Deployment

1. **Smart Contract**:
```bash
cd src/blockchain
npm install
npx hardhat deploy --network base-sepolia
```

2. **AI Bridge**:
```bash
cd src/ai-bridge
pip install -r requirements.txt
python main.py
```

3. **ESP32 Setup**:
- Flash `src/esp32/water_pump.ino` to LilyGO T-Call
- Configure SIM card and pump GPIO pins

4. **Web Interface**:
```bash
cd src/web
npm install
npm run dev
```

## Testing in Production (TIP)

1. **Test Payment Flow**:
   - Connect wallet to Base testnet
   - Purchase water credits via web interface
   - Verify SMS command sent to ESP32

2. **Hardware Test**:
   - Send test SMS to ESP32
   - Confirm pump activation
   - Check water flow sensor readings

## Use Cases
- Community water kiosks
- NGO deployment
- Disaster relief
- Rural water access

## Impact
Provides transparent, automated, corruption-free clean water distribution for rural Africa.

## Built with Vibe Coding
This project extensively uses AI-assisted development. See `prompts.md` and `ai_logs/` for detailed documentation of the AI development process.
# MajiSafe-DeFi-Water-Access-
