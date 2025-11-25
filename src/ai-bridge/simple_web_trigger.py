#!/usr/bin/env python3
"""
Simple Web Trigger - No Selenium
Just opens browser and triggers web UI via HTTP
"""

import requests
import webbrowser
import time

class SimpleWebTrigger:
    def __init__(self):
        self.web_url = "http://localhost:8000"
        
    def process_sms_payment(self, phone, message):
        """Trigger web payment without complex automation"""
        try:
            print(f"🌐 Triggering web payment for SMS: {message}")
            
            # 1. Open browser to MajiSafe (user will see it)
            webbrowser.open(self.web_url)
            print("🌐 Browser opened to MajiSafe")
            
            # 2. Give user time to see and click (or auto-trigger)
            time.sleep(2)
            
            # 3. Simulate successful payment for demo
            print("✅ Web payment triggered successfully")
            
            return {
                'status': 'success',
                'message': 'activate',
                'tx_hash': 'web_triggered_payment',
                'pump_id': 'PUMP001'
            }
            
        except Exception as e:
            print(f"❌ Web trigger error: {e}")
            return {
                'status': 'error',
                'message': f'Web trigger failed: {str(e)}'
            }
    
    def close(self):
        """Nothing to close"""
        pass

# Test function
if __name__ == "__main__":
    trigger = SimpleWebTrigger()
    result = trigger.process_sms_payment("+25769820499", "PAY 5000 BIF PUMP001")
    print(f"Result: {result}")
    trigger.close()
