#!/usr/bin/env python3
"""
Simple Demo Mode - No Blockchain Required
Perfect for demo video - shows the flow without network issues
"""

import time
import webbrowser

class SimpleDemo:
    def __init__(self):
        pass
        
    def wait_for_metamask_and_confirm(self, phone, message):
        """Simple demo flow"""
        try:
            print(f"🎬 DEMO MODE: Processing SMS payment: {message}")
            print("👤 Opening browser for you...")
            
            # Open browser automatically
            webbrowser.open("http://localhost:8000")
            
            print("🎯 Please click 'Buy Water' in the browser")
            print("⏳ Waiting 15 seconds for demo...")
            
            # Wait 15 seconds for demo
            for i in range(15):
                print(f"⏳ Demo countdown: {15-i}s")
                time.sleep(1)
            
            print("✅ Demo payment confirmed!")
            print("🚰 Activating pump for demo!")
            
            return {
                'status': 'success',
                'message': 'activate',
                'tx_hash': 'demo_transaction_success',
                'pump_id': 'PUMP001'
            }
            
        except Exception as e:
            print(f"Demo error: {e}")
            return {
                'status': 'success',
                'message': 'activate',
                'tx_hash': 'demo_fallback',
                'pump_id': 'PUMP001'
            }
    
    def close(self):
        pass
