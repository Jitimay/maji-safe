#!/usr/bin/env python3
"""
Simple SMS AI Bridge for ESP32
Receives SMS from ESP32, makes Web3 payment, sends back activation command
"""

from flask import Flask, request, jsonify
from web3 import Web3
import sqlite3
import re

app = Flask(__name__)

class SimpleSMSAI:
    def __init__(self):
        # Web3 setup
        self.w3 = Web3(Web3.HTTPProvider('https://sepolia.base.org'))
        self.contract_address = '0x4933781A5DDC86bdF9c9C9795647e763E0429E28'
        
        # Your MetaMask private key for automatic payments
        self.private_key = "YOUR_PRIVATE_KEY_HERE"  # Replace with your key
        
        # Currency rates (local currency to ETH)
        self.rates = {
            'BIF': 0.000000347,  # Burundi Francs
            'USD': 0.0004,
            'RWF': 0.000000312   # Rwanda Francs
        }
        
        self.init_db()
        print("🤖 Simple SMS AI Bridge Ready")
        print("📱 Waiting for SMS from ESP32...")
    
    def init_db(self):
        self.conn = sqlite3.connect('sms_payments.db', check_same_thread=False)
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS payments (
                id INTEGER PRIMARY KEY,
                phone TEXT,
                message TEXT,
                amount REAL,
                currency TEXT,
                pump_id TEXT,
                eth_amount REAL,
                tx_hash TEXT,
                status TEXT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        self.conn.commit()
    
    def parse_sms(self, message):
        """Parse: PAY 5000 BIF PUMP001"""
        try:
            parts = message.upper().strip().split()
            if len(parts) != 4 or parts[0] != 'PAY':
                return None
            
            amount = float(parts[1])
            currency = parts[2]
            pump_id = parts[3]
            eth_amount = amount * self.rates.get(currency, 0)
            
            return {
                'amount': amount,
                'currency': currency,
                'pump_id': pump_id,
                'eth_amount': eth_amount
            }
        except:
            return None
    
    def make_web3_payment(self, payment_data):
        """Make automatic Web3 payment"""
        try:
            print(f"💰 Making payment: {payment_data['eth_amount']} ETH")
            
            # For demo, simulate successful payment
            fake_tx = f"0x{''.join([f'{i:02x}' for i in range(32)])}"
            
            print(f"🔗 Simulated TX: {fake_tx}")
            return fake_tx
            
        except Exception as e:
            print(f"❌ Payment error: {e}")
            return None

# Global AI instance
sms_ai = SimpleSMSAI()

@app.route('/process-sms', methods=['POST'])
def process_sms():
    """Receive SMS from ESP32"""
    try:
        data = request.json
        phone = data.get('phone', '')
        message = data.get('message', '')
        
        print(f"\n📱 SMS from {phone}: {message}")
        
        # Parse payment
        payment_data = sms_ai.parse_sms(message)
        if not payment_data:
            return jsonify({'status': 'error', 'message': 'Invalid SMS format'})
        
        print(f"💰 Payment: {payment_data['amount']} {payment_data['currency']} = {payment_data['eth_amount']} ETH")
        
        # Check minimum payment
        if payment_data['eth_amount'] < 0.001:
            return jsonify({'status': 'error', 'message': 'Payment too small'})
        
        # Make Web3 payment
        tx_hash = sms_ai.make_web3_payment(payment_data)
        if not tx_hash:
            return jsonify({'status': 'error', 'message': 'Payment failed'})
        
        # Log payment
        sms_ai.conn.execute('''
            INSERT INTO payments (phone, message, amount, currency, pump_id, eth_amount, tx_hash, status)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (phone, message, payment_data['amount'], payment_data['currency'],
              payment_data['pump_id'], payment_data['eth_amount'], tx_hash, 'completed'))
        sms_ai.conn.commit()
        
        print(f"✅ Payment successful: {tx_hash}")
        print(f"🚰 Activating pump: {payment_data['pump_id']}")
        
        return jsonify({
            'status': 'success',
            'message': 'activate',  # This tells ESP32 to activate pump
            'tx_hash': tx_hash,
            'pump_id': payment_data['pump_id']
        })
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return jsonify({'status': 'error', 'message': str(e)})

@app.route('/status', methods=['GET'])
def status():
    return jsonify({'status': 'online', 'service': 'Simple SMS AI Bridge'})

if __name__ == "__main__":
    print("🚀 Starting Simple SMS AI Bridge...")
    print("📱 ESP32 will send SMS data here")
    print("💰 AI will make Web3 payments automatically")
    print("🚰 Pump activation commands sent back to ESP32")
    
    app.run(host='0.0.0.0', port=5000, debug=True)
