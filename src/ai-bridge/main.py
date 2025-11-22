#!/usr/bin/env python3
"""
Maji-Vibe AI Bridge - Blockchain to SMS Gateway
Listens to blockchain events and sends SMS commands to ESP32 pumps
"""

import asyncio
import sqlite3
from web3 import Web3
from twilio.rest import Client
import json
import os

class WaterBridge:
    def __init__(self):
        self.w3 = Web3(Web3.HTTPProvider(os.getenv('BASE_RPC_URL')))
        self.twilio = Client(os.getenv('TWILIO_SID'), os.getenv('TWILIO_TOKEN'))
        self.init_db()
    
    def init_db(self):
        """Initialize SQLite database"""
        self.conn = sqlite3.connect('water_logs.db')
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS transactions (
                id INTEGER PRIMARY KEY,
                user_address TEXT,
                pump_id TEXT,
                credits INTEGER,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        self.conn.commit()
    
    async def listen_events(self):
        """Listen for WaterPurchased events"""
        contract_address = os.getenv('CONTRACT_ADDRESS')
        contract_abi = json.loads(os.getenv('CONTRACT_ABI'))
        contract = self.w3.eth.contract(address=contract_address, abi=contract_abi)
        
        event_filter = contract.events.WaterPurchased.create_filter(fromBlock='latest')
        
        while True:
            for event in event_filter.get_new_entries():
                await self.process_purchase(event)
            await asyncio.sleep(2)
    
    async def process_purchase(self, event):
        """Process water purchase and send SMS command"""
        user = event['args']['user']
        credits = event['args']['credits']
        pump_id = event['args']['pumpId'].hex()
        
        # Log transaction
        self.conn.execute(
            'INSERT INTO transactions (user_address, pump_id, credits) VALUES (?, ?, ?)',
            (user, pump_id, credits)
        )
        self.conn.commit()
        
        # Send SMS command (1-5 bytes)
        sms_command = f"P{credits:02d}"  # P01, P02, etc.
        phone_number = os.getenv('ESP32_PHONE')
        
        self.twilio.messages.create(
            body=sms_command,
            from_=os.getenv('TWILIO_PHONE'),
            to=phone_number
        )
        
        print(f"SMS sent: {sms_command} to {phone_number}")

if __name__ == "__main__":
    bridge = WaterBridge()
    asyncio.run(bridge.listen_events())
