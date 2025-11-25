#!/usr/bin/env python3
"""
SMS Payment Simulator - Test MajiSafe SMS payments
Simulates rural African users sending SMS payments
"""

import requests
import json
import time

def simulate_sms_payment(phone, payment_text):
    """Simulate SMS payment from rural user"""
    
    ai_bridge_url = "http://localhost:5000/sms-payment"
    
    payload = {
        "payment": payment_text,
        "phone": phone
    }
    
    print(f"📱 Simulating SMS from {phone}: {payment_text}")
    
    try:
        response = requests.post(ai_bridge_url, json=payload)
        
        if response.status_code == 200:
            result = response.json()
            print(f"✅ Payment processed: {result['message']}")
            print(f"🔗 Blockchain TX: {result.get('tx_hash', 'N/A')}")
            print(f"🚰 Pump activated: {result.get('pump_activated', False)}")
        else:
            print(f"❌ Payment failed: {response.json().get('message', 'Unknown error')}")
            
    except Exception as e:
        print(f"❌ Connection error: {e}")

def main():
    print("🌍 MajiSafe SMS Payment Simulator")
    print("💧 Simulating payments from rural Africa\n")
    
    # Test cases
    test_payments = [
        ("+25779123456", "PAY 5000 BIF PUMP001"),  # Burundi Francs
        ("+254701234567", "PAY 10 USD PUMP002"),   # Kenya USD
        ("+250788123456", "PAY 2000 RWF PUMP001"), # Rwanda Francs (will fail - not supported)
        ("+25779987654", "PAY 100 BIF PUMP001"),   # Too small amount (will fail)
    ]
    
    for phone, payment in test_payments:
        simulate_sms_payment(phone, payment)
        print("-" * 50)
        time.sleep(2)

if __name__ == "__main__":
    main()
