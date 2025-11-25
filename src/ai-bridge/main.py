#!/usr/bin/env python3
"""
MajiSafe AI Bridge - SMS Payment Processor
Receives SMS payments from ESP32, validates, processes blockchain, activates pump
"""

import asyncio
import sqlite3
from web3 import Web3
import requests
import json
import os
from flask import Flask, request, jsonify
import re
from datetime import datetime

app = Flask(__name__)

class MajiSafeAI:
    def __init__(self):
        # Blockchain setup
        self.w3 = Web3(Web3.HTTPProvider('https://sepolia.base.org'))
        self.contract_address = '0x4933781A5DDC86bdF9c9C9795647e763E0429E28'
        self.contract_abi = [
            "function buyWater(bytes32 pumpId) payable",
            "function activatePump(bytes32 pumpId, uint256 liters) external",
            "event WaterPurchased(address indexed user, uint256 credits, bytes32 pumpId)"
        ]
        
        # Payment rates (Burundi Francs to ETH)
        self.exchange_rates = {
            'BIF': 0.000000347,  # 1 BIF = 0.000000347 ETH (example rate)
            'USD': 0.0004,       # 1 USD = 0.0004 ETH
        }
        
        self.init_db()
        print("🤖 MajiSafe AI Bridge Started")
        print("💧 Ready to process SMS payments from rural Africa")
    
    def init_db(self):
        """Initialize payment database"""
        self.conn = sqlite3.connect('payments.db', check_same_thread=False)
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS sms_payments (
                id INTEGER PRIMARY KEY,
                phone_number TEXT,
                amount REAL,
                currency TEXT,
                pump_id TEXT,
                status TEXT,
                blockchain_tx TEXT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        self.conn.commit()
    
    def parse_sms_payment(self, sms_text, phone_number):
        """Parse SMS: 'PAY 1000 BIF PUMP001' or 'PAY 5 USD PUMP001'"""
        try:
            # Extract payment info using regex
            pattern = r'PAY\s+(\d+)\s+(\w+)\s+(\w+)'
            match = re.search(pattern, sms_text.upper())
            
            if not match:
                return None
                
            amount = float(match.group(1))
            currency = match.group(2)
            pump_id = match.group(3)
            
            return {
                'phone': phone_number,
                'amount': amount,
                'currency': currency,
                'pump_id': pump_id,
                'eth_equivalent': amount * self.exchange_rates.get(currency, 0)
            }
        except Exception as e:
            print(f"❌ SMS parsing error: {e}")
            return None
    
    def validate_payment(self, payment_data):
        """Validate payment amount and pump ID"""
        min_payment_eth = 0.001  # Minimum 0.001 ETH
        
        if payment_data['eth_equivalent'] < min_payment_eth:
            return False, f"Insufficient payment. Minimum: {min_payment_eth} ETH"
        
        if not payment_data['pump_id'].startswith('PUMP'):
            return False, "Invalid pump ID"
            
        return True, "Payment valid"
    
    def process_blockchain_transaction(self, payment_data):
        """Process payment on blockchain (simulated for demo)"""
        try:
            # In real implementation, this would:
            # 1. Convert local currency to ETH
            # 2. Execute smart contract transaction
            # 3. Wait for confirmation
            
            # For demo, simulate blockchain transaction
            fake_tx_hash = f"0x{''.join([f'{ord(c):02x}' for c in payment_data['phone'][-10:]])}"
            
            print(f"💰 Processing {payment_data['amount']} {payment_data['currency']} from {payment_data['phone']}")
            print(f"🔗 Blockchain TX: {fake_tx_hash}")
            
            return True, fake_tx_hash
            
        except Exception as e:
            print(f"❌ Blockchain error: {e}")
            return False, str(e)
    
    def send_pump_activation(self, pump_id, duration=10):
        """Send activation command back to ESP32"""
        try:
            # In real implementation, send HTTP request to ESP32
            esp32_ip = "192.168.1.100"  # ESP32 IP address
            activation_url = f"http://{esp32_ip}/activate"
            
            payload = {
                'pump_id': pump_id,
                'duration': duration,
                'command': 'ACTIVATE'
            }
            
            # For demo, just print the command
            print(f"🚰 Sending activation to {pump_id}: {duration} seconds")
            print(f"📡 Command sent to ESP32: {payload}")
            
            return True
            
        except Exception as e:
            print(f"❌ Pump activation error: {e}")
            return False

@app.route('/sms-payment', methods=['POST'])
def handle_sms_payment():
    """Receive SMS payment from ESP32"""
    try:
        data = request.json
        sms_text = data.get('payment', '')
        phone_number = data.get('phone', '')
        
        print(f"📱 SMS Payment received from {phone_number}: {sms_text}")
        
        # Parse SMS payment
        payment_data = ai_bridge.parse_sms_payment(sms_text, phone_number)
        if not payment_data:
            return jsonify({'status': 'error', 'message': 'Invalid SMS format'}), 400
        
        # Validate payment
        is_valid, message = ai_bridge.validate_payment(payment_data)
        if not is_valid:
            print(f"❌ Payment validation failed: {message}")
            return jsonify({'status': 'error', 'message': message}), 400
        
        # Process blockchain transaction
        success, tx_hash = ai_bridge.process_blockchain_transaction(payment_data)
        if not success:
            return jsonify({'status': 'error', 'message': tx_hash}), 500
        
        # Log payment
        ai_bridge.conn.execute('''
            INSERT INTO sms_payments (phone_number, amount, currency, pump_id, status, blockchain_tx)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (phone_number, payment_data['amount'], payment_data['currency'], 
              payment_data['pump_id'], 'confirmed', tx_hash))
        ai_bridge.conn.commit()
        
        # Activate pump
        pump_activated = ai_bridge.send_pump_activation(payment_data['pump_id'])
        
        response = {
            'status': 'success',
            'message': 'Payment processed and pump activated',
            'tx_hash': tx_hash,
            'pump_activated': pump_activated
        }
        
        print(f"✅ Payment processed successfully: {tx_hash}")
        return jsonify(response)
        
    except Exception as e:
        print(f"❌ Error processing SMS payment: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/status', methods=['GET'])
def get_status():
    """Get AI Bridge status"""
    return jsonify({
        'status': 'online',
        'service': 'MajiSafe AI Bridge',
        'blockchain': 'Base Sepolia',
        'contract': ai_bridge.contract_address
    })

if __name__ == "__main__":
    ai_bridge = MajiSafeAI()
    
    print("🚀 Starting MajiSafe AI Bridge Server...")
    print("📱 Listening for SMS payments from ESP32...")
    print("🔗 Connected to Base Sepolia blockchain")
    print("💧 Ready to activate water pumps!")
    
    app.run(host='0.0.0.0', port=5000, debug=True)
