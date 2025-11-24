#!/usr/bin/env python3
"""
Setup Real Web3 Integration for MajiChain
Configure your MetaMask private key for automatic payments
"""

import os

def setup_web3():
    print("🔐 MajiChain Real Web3 Setup")
    print("=" * 40)
    
    print("\n⚠️  SECURITY WARNING:")
    print("- Never share your private key")
    print("- Use a dedicated wallet for this project")
    print("- Keep some Base Sepolia ETH in the wallet")
    
    print("\n📋 Steps to get your private key:")
    print("1. Open MetaMask")
    print("2. Click account menu → Account Details")
    print("3. Click 'Export Private Key'")
    print("4. Enter your password")
    print("5. Copy the 64-character hex string (without 0x)")
    
    private_key = input("\n🔑 Enter your MetaMask private key (64 chars): ").strip()
    
    if len(private_key) == 64:
        # Update the AI bridge file
        ai_file = "/home/josh/Kiro/maji_chain/src/ai-bridge/majichain_ai.py"
        
        with open(ai_file, 'r') as f:
            content = f.read()
        
        # Replace placeholder with real key
        content = content.replace(
            'self.private_key = "YOUR_METAMASK_PRIVATE_KEY_HERE"',
            f'self.private_key = "{private_key}"'
        )
        
        with open(ai_file, 'w') as f:
            f.write(content)
        
        print("✅ Private key configured!")
        print("🚀 Your MajiChain will now make REAL blockchain transactions!")
        
    elif len(private_key) == 66 and private_key.startswith('0x'):
        # Remove 0x prefix
        private_key = private_key[2:]
        print("✅ Removed 0x prefix")
        
        # Update file
        ai_file = "/home/josh/Kiro/maji_chain/src/ai-bridge/majichain_ai.py"
        
        with open(ai_file, 'r') as f:
            content = f.read()
        
        content = content.replace(
            'self.private_key = "YOUR_METAMASK_PRIVATE_KEY_HERE"',
            f'self.private_key = "{private_key}"'
        )
        
        with open(ai_file, 'w') as f:
            f.write(content)
        
        print("✅ Private key configured!")
        
    else:
        print("❌ Invalid private key length. Should be 64 characters.")
        return False
    
    print("\n💰 Make sure you have Base Sepolia ETH:")
    print("- Get free testnet ETH: https://www.coinbase.com/faucets/base-ethereum-sepolia-faucet")
    print("- Minimum needed: ~0.01 ETH for gas fees")
    
    print("\n🚀 Ready to test:")
    print("1. Restart AI Bridge: python majichain_ai.py")
    print("2. Send SMS: PAY 5000 BIF PUMP001")
    print("3. Watch REAL blockchain transaction!")
    
    return True

if __name__ == "__main__":
    setup_web3()
