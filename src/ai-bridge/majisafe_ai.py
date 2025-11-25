#!/usr/bin/env python3
"""
MajiSafe AI Bridge - SMS Payment Processor
Receives SMS from ESP32, processes payments, activates pumps
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from web3 import Web3
import sqlite3
import re
from datetime import datetime

app = Flask(__name__)
CORS(app)  # Enable CORS for web UI communication

from metamask_only import MetaMaskOnly

class MajiSafeAI:
    def __init__(self):
        # Use MOONBASE ALPHA (same as web UI)
        self.w3 = Web3(Web3.HTTPProvider('https://rpc.api.moonbase.moonbeam.network'))
        self.contract_address = '0x4933781A5DDC86bdF9c9C9795647e763E0429E28'
        
        # MetaMask-only automation
        self.metamask_only = None
        
        # Currency exchange rates (for validation only)
        self.rates = {
            'BIF': 0.000000347,  # Burundi Francs
            'USD': 0.0004,       # US Dollars
            'RWF': 0.000000312,  # Rwanda Francs
            'KES': 0.0000065,    # Kenya Shillings
        }
        
        self.init_db()
        print("🤖 MajiSafe AI Bridge Ready")
        print("🔗 Using Base Sepolia (same as web UI)")
        print("🦊 Will auto-confirm MetaMask when you click Buy Water")
    
    def init_db(self):
        self.conn = sqlite3.connect('majisafe_payments.db', check_same_thread=False)
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS sms_payments (
                id INTEGER PRIMARY KEY,
                phone TEXT,
                sms_content TEXT,
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
    
    def parse_payment_sms(self, message):
        """Parse: PAY 5000 BIF PUMP001"""
        try:
            parts = message.upper().strip().split()
            if len(parts) != 4 or parts[0] != 'PAY':
                return None
            
            amount = float(parts[1])
            currency = parts[2]
            pump_id = parts[3]
            
            # Convert to ETH
            eth_amount = amount * self.rates.get(currency, 0)
            
            return {
                'amount': amount,
                'currency': currency,
                'pump_id': pump_id,
                'eth_amount': eth_amount
            }
        except Exception as e:
            print(f"❌ SMS parse error: {e}")
            return None
    
    def validate_payment(self, payment_data):
        """Validate payment amount and pump ID"""
        min_eth = 0.001  # Minimum payment
        
        if payment_data['eth_amount'] < min_eth:
            return False, f"Minimum payment: {min_eth} ETH ({int(min_eth / self.rates.get(payment_data['currency'], 1))} {payment_data['currency']})"
        
        if not payment_data['pump_id'].startswith('PUMP'):
            return False, "Invalid pump ID format"
        
        return True, "Valid payment"
    
    def process_blockchain_payment(self, payment_data):
        """Process payment with MetaMask-only automation"""
        try:
            print(f"🦊 Starting MetaMask automation for: {payment_data['amount']} {payment_data['currency']}")
            print("👤 Please open http://localhost:8000 and click 'Buy Water'")
            
            # Initialize MetaMask automation if not already done
            if not self.metamask_only:
                self.metamask_only = MetaMaskOnly()
            
            # Wait for user to click Buy Water, then auto-confirm MetaMask
            result = self.metamask_only.wait_for_metamask_and_confirm(
                payment_data.get('phone', ''),
                f"PAY {payment_data['amount']} {payment_data['currency']} {payment_data['pump_id']}"
            )
            
            if result['status'] == 'success':
                print(f"✅ MetaMask confirmation successful!")
                return True, result.get('tx_hash', 'metamask_confirmed')
            else:
                print(f"❌ MetaMask confirmation failed: {result['message']}")
                return False, result['message']
            
        except Exception as e:
            print(f"❌ MetaMask automation error: {e}")
            return False, str(e)

# Global AI instance
ai = MajiSafeAI()

# Track current SMS payment
current_sms_payment = {
    'payment_received': False,
    'phone': '',
    'amount': '',
    'blockchain_confirmed': False
}

@app.route('/sms-status', methods=['GET'])
def get_sms_status():
    """Get current SMS payment status for web UI"""
    print(f"🌐 Web UI checking SMS status: {current_sms_payment}")
    return jsonify(current_sms_payment)

@app.route('/test', methods=['GET'])
def test_connection():
    """Test endpoint for web UI"""
    return jsonify({'status': 'AI Bridge online', 'message': 'Connection working'})

@app.route('/blockchain-confirmed', methods=['POST'])
def blockchain_confirmed():
    """Web UI notifies that blockchain transaction is confirmed"""
    global current_sms_payment
    data = request.json
    
    print(f"✅ Blockchain confirmed: {data.get('tx_hash')}")
    
    # Reset payment status for next SMS
    current_sms_payment = {
        'payment_received': False,
        'phone': '',
        'amount': '',
        'blockchain_confirmed': True
    }
    
    return jsonify({'status': 'confirmed'})

@app.route('/process-sms', methods=['POST'])
def process_sms():
    """Main SMS processing endpoint"""
    try:
        global current_sms_payment
        
        data = request.json
        phone = data.get('phone', '')
        message = data.get('message', '')
        
        print(f"\n📱 SMS from {phone}: {message}")
        
        # Parse payment SMS
        payment_data = ai.parse_payment_sms(message)
        if not payment_data:
            return jsonify({
                'status': 'error',
                'message': 'Invalid format. Send: PAY [amount] [currency] [pump]\nExample: PAY 5000 BIF PUMP001'
            })
        
        print(f"💰 Parsed: {payment_data['amount']} {payment_data['currency']} = {payment_data['eth_amount']} ETH")
        
        # Validate payment
        is_valid, validation_msg = ai.validate_payment(payment_data)
        if not is_valid:
            print(f"❌ Validation failed: {validation_msg}")
            return jsonify({
                'status': 'error',
                'message': validation_msg
            })
        
        # Set SMS payment received status for web UI
        current_sms_payment = {
            'payment_received': True,
            'phone': phone,
            'amount': f"{payment_data['amount']} {payment_data['currency']}",
            'blockchain_confirmed': False
        }
        
        print(f"✅ SMS payment received - web UI button will activate")
        print(f"👤 User must now click 'Purchase Water' in web UI")
        
        # Log SMS payment
        ai.conn.execute('''
            INSERT INTO sms_payments (phone, sms_content, amount, currency, pump_id, eth_amount, tx_hash, status)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (phone, message, payment_data['amount'], payment_data['currency'],
              payment_data['pump_id'], payment_data['eth_amount'], 'sms_received', 'pending_blockchain'))
        ai.conn.commit()
        
        return jsonify({
            'status': 'success',
            'message': 'SMS payment received - activate web UI button',
            'phone': phone,
            'amount': f"{payment_data['amount']} {payment_data['currency']}"
        })
        
    except Exception as e:
        print(f"❌ Processing error: {e}")
        return jsonify({
            'status': 'error',
            'message': 'System error. Please try again.'
        })

@app.route('/status', methods=['GET'])
def status():
    """Health check endpoint"""
    return jsonify({
        'status': 'online',
        'service': 'MajiSafe AI Bridge',
        'blockchain': 'Base Sepolia',
        'contract': ai.contract_address,
        'supported_currencies': list(ai.rates.keys())
    })

@app.route('/payments', methods=['GET'])
def get_payments():
    """Get recent payments for monitoring"""
    try:
        cursor = ai.conn.execute('''
            SELECT phone, amount, currency, pump_id, tx_hash, status, timestamp 
            FROM sms_payments 
            ORDER BY timestamp DESC 
            LIMIT 10
        ''')
        
        payments = []
        for row in cursor.fetchall():
            payments.append({
                'phone': row[0],
                'amount': row[1],
                'currency': row[2],
                'pump_id': row[3],
                'tx_hash': row[4],
                'status': row[5],
                'timestamp': row[6]
            })
        
        return jsonify({'payments': payments})
        
    except Exception as e:
        return jsonify({'error': str(e)})

if __name__ == "__main__":
    print("🚀 Starting MajiSafe AI Bridge...")
    print("📱 Ready to process SMS payments")
    print("💧 Converting crypto to clean water")
    print("🌍 Serving rural Africa")
    
    app.run(host='0.0.0.0', port=5001, debug=True)
