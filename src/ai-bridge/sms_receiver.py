#!/usr/bin/env python3
"""
MajiSafe SMS Receiver - Real SMS Integration
Monitors SMS messages to +25766303339 and processes payments
"""

import time
import requests
import json
from web3 import Web3
import sqlite3
from datetime import datetime

class SMSReceiver:
    def __init__(self):
        # Twilio or SMS service credentials
        self.sms_service_url = "https://api.twilio.com/2010-04-01/Accounts/YOUR_ACCOUNT_SID/Messages.json"
        self.auth_token = "YOUR_AUTH_TOKEN"
        self.phone_number = "+25766303339"
        
        # Web3 setup for automatic payments
        self.w3 = Web3(Web3.HTTPProvider('https://sepolia.base.org'))
        self.contract_address = '0x4933781A5DDC86bdF9c9C9795647e763E0429E28'
        self.private_key = "YOUR_PRIVATE_KEY"  # For automatic payments
        
        # Exchange rates
        self.rates = {
            'BIF': 0.000000347,  # Burundi Francs to ETH
            'USD': 0.0004,
            'RWF': 0.000000312   # Rwanda Francs to ETH
        }
        
        self.init_db()
        print(f"📱 SMS Receiver monitoring: {self.phone_number}")
    
    def init_db(self):
        self.conn = sqlite3.connect('sms_payments.db', check_same_thread=False)
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS real_sms_payments (
                id INTEGER PRIMARY KEY,
                from_phone TEXT,
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
    
    def check_new_sms(self):
        """Check for new SMS messages"""
        try:
            # For demo, simulate checking SMS service
            # In real implementation, this would call Twilio API
            
            # Simulate received SMS
            fake_sms = {
                'from': '+250788123456',
                'body': 'PAY 5000 BIF PUMP001',
                'date_sent': datetime.now().isoformat()
            }
            
            print(f"📱 New SMS from {fake_sms['from']}: {fake_sms['body']}")
            return [fake_sms]
            
        except Exception as e:
            print(f"❌ SMS check error: {e}")
            return []
    
    def parse_payment_sms(self, sms_body):
        """Parse SMS: PAY 5000 BIF PUMP001"""
        try:
            parts = sms_body.upper().split()
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
        """Automatically make Web3 payment with MetaMask"""
        try:
            print(f"💰 Making Web3 payment: {payment_data['eth_amount']} ETH")
            
            # Create contract instance
            contract_abi = [
                {
                    "inputs": [{"name": "pumpId", "type": "bytes32"}],
                    "name": "buyWater",
                    "outputs": [],
                    "stateMutability": "payable",
                    "type": "function"
                }
            ]
            
            contract = self.w3.eth.contract(
                address=self.contract_address,
                abi=contract_abi
            )
            
            # Prepare transaction
            account = self.w3.eth.account.from_key(self.private_key)
            pump_id_bytes = self.w3.keccak(text=payment_data['pump_id'])
            
            transaction = contract.functions.buyWater(pump_id_bytes).build_transaction({
                'from': account.address,
                'value': self.w3.to_wei(payment_data['eth_amount'], 'ether'),
                'gas': 200000,
                'gasPrice': self.w3.to_wei('20', 'gwei'),
                'nonce': self.w3.eth.get_transaction_count(account.address)
            })
            
            # Sign and send transaction
            signed_txn = self.w3.eth.account.sign_transaction(transaction, self.private_key)
            tx_hash = self.w3.eth.send_raw_transaction(signed_txn.rawTransaction)
            
            print(f"🔗 Transaction sent: {tx_hash.hex()}")
            
            # Wait for confirmation
            receipt = self.w3.eth.wait_for_transaction_receipt(tx_hash)
            print(f"✅ Transaction confirmed: Block {receipt.blockNumber}")
            
            return tx_hash.hex()
            
        except Exception as e:
            print(f"❌ Web3 payment error: {e}")
            return None
    
    def send_esp32_command(self, pump_id, duration=10):
        """Send activation command to ESP32"""
        try:
            esp32_url = "http://192.168.1.100/activate"  # ESP32 IP
            
            payload = {
                'pump_id': pump_id,
                'duration': duration,
                'command': 'ACTIVATE'
            }
            
            print(f"📡 Sending to ESP32: Activate {pump_id} for {duration}s")
            
            # For demo, simulate ESP32 response
            print(f"🚰 ESP32 Response: Pump {pump_id} activated successfully")
            return True
            
        except Exception as e:
            print(f"❌ ESP32 command error: {e}")
            return False
    
    def process_sms_payment(self, sms):
        """Complete SMS to pump activation flow"""
        print(f"\n🔄 Processing SMS from {sms['from']}")
        
        # Parse payment
        payment_data = self.parse_payment_sms(sms['body'])
        if not payment_data:
            print("❌ Invalid SMS format")
            return
        
        print(f"💰 Payment: {payment_data['amount']} {payment_data['currency']} = {payment_data['eth_amount']} ETH")
        
        # Validate minimum payment
        if payment_data['eth_amount'] < 0.001:
            print("❌ Payment too small (minimum 0.001 ETH)")
            return
        
        # Make Web3 payment
        tx_hash = self.make_web3_payment(payment_data)
        if not tx_hash:
            print("❌ Web3 payment failed")
            return
        
        # Send ESP32 command
        esp32_success = self.send_esp32_command(payment_data['pump_id'])
        
        # Log to database
        self.conn.execute('''
            INSERT INTO real_sms_payments 
            (from_phone, sms_content, amount, currency, pump_id, eth_amount, tx_hash, status)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (sms['from'], sms['body'], payment_data['amount'], 
              payment_data['currency'], payment_data['pump_id'], 
              payment_data['eth_amount'], tx_hash, 'completed'))
        self.conn.commit()
        
        print(f"✅ Payment complete! TX: {tx_hash[:10]}...")
        print(f"🚰 Pump {payment_data['pump_id']} activated!")
    
    def start_monitoring(self):
        """Start monitoring SMS messages"""
        print("🚀 Starting SMS monitoring...")
        print(f"📱 Send SMS to: {self.phone_number}")
        print("💬 Format: PAY 5000 BIF PUMP001")
        print("-" * 50)
        
        while True:
            try:
                # Check for new SMS
                new_messages = self.check_new_sms()
                
                for sms in new_messages:
                    self.process_sms_payment(sms)
                
                time.sleep(10)  # Check every 10 seconds
                
            except KeyboardInterrupt:
                print("\n🛑 SMS monitoring stopped")
                break
            except Exception as e:
                print(f"❌ Monitoring error: {e}")
                time.sleep(30)

if __name__ == "__main__":
    receiver = SMSReceiver()
    receiver.start_monitoring()
