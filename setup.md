# MajiSafe Setup Guide

## Step 1: Get Base Sepolia ETH
1. Go to https://www.coinbase.com/faucets/base-ethereum-sepolia-faucet
2. Connect your wallet and get free testnet ETH

## Step 2: Configure Environment
```bash
cd src/blockchain
cp .env.example .env
```

Edit `.env` with your values:
```
PRIVATE_KEY=your_64_character_private_key_without_0x
BASE_RPC_URL=https://sepolia.base.org
```

## Step 3: Deploy Contract
```bash
cd src/blockchain
npm install
npx hardhat run scripts/deploy.js --network base-sepolia
```

## Step 4: Update Web App
Copy the deployed contract address to `src/web/app.js`

## Step 5: Test
Open `src/web/index.html` in browser and test purchase flow.
