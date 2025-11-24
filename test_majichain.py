#!/usr/bin/env python3
"""
MajiChain System Test
Test SMS payment processing end-to-end
"""

import requests
import json
import time

def test_sms_payment(phone, message):
    """Test SMS payment processing"""
    
    url = "http://192.168.155.181:5000/process-sms"
    
    payload = {
        "phone": phone,
        "message": message
    }
    
    print(f"📱 Testing SMS from {phone}: {message}")
    
    try:
        response = requests.post(url, json=payload, timeout=10)
        
        if response.status_code == 200:
            result = response.json()
            status = result.get('status')
            
            if status == 'success':
                print(f"✅ Payment SUCCESS!")
                print(f"💰 TX Hash: {result.get('tx_hash')}")
                print(f"🚰 Pump: {result.get('pump_id')}")
                print(f"💧 Amount: {result.get('amount')}")
                print(f"🎯 Action: {result.get('message')}")
            else:
                print(f"❌ Payment FAILED: {result.get('message')}")
                
        else:
            print(f"❌ HTTP Error: {response.status_code}")
            
    except Exception as e:
        print(f"❌ Connection error: {e}")

def test_system_status():
    """Test system status"""
    try:
        response = requests.get("http://192.168.155.181:5000/status", timeout=5)
        if response.status_code == 200:
            status = response.json()
            print("🤖 System Status:")
            print(f"   Service: {status.get('service')}")
            print(f"   Status: {status.get('status')}")
            print(f"   Blockchain: {status.get('blockchain')}")
            print(f"   Contract: {status.get('contract')}")
            print(f"   Currencies: {status.get('supported_currencies')}")
            return True
        else:
            print("❌ Status check failed")
            return False
    except:
        print("❌ AI Bridge offline")
        return False

def main():
    print("🚰 MajiChain System Test")
    print("=" * 50)
    
    # Test system status
    if not test_system_status():
        print("\n❌ System not ready. Start AI Bridge first:")
        print("cd src/ai-bridge && source venv/bin/activate && python majichain_ai.py")
        return
    
    print("\n📱 Testing SMS Payments:")
    print("-" * 30)
    
    # Test cases
    test_cases = [
        ("+250788123456", "PAY 5000 BIF PUMP001"),    # Valid Burundi payment
        ("+254701234567", "PAY 10 USD PUMP002"),      # Valid USD payment  
        ("+25779123456", "PAY 100 BIF PUMP001"),      # Too small amount
        ("+250788999888", "PAY 2000 RWF PUMP003"),    # Valid Rwanda payment
        ("+254701111222", "HELLO WORLD"),             # Invalid format
        ("+25779555444", "PAY ABC BIF PUMP001"),      # Invalid amount
    ]
    
    for phone, message in test_cases:
        test_sms_payment(phone, message)
        print("-" * 30)
        time.sleep(1)
    
    print("\n🎯 Test Summary:")
    print("✅ Valid payments should show 'Payment SUCCESS'")
    print("❌ Invalid payments should show error messages")
    print("🚰 ESP32 should activate pump on successful payments")

if __name__ == "__main__":
    main()
