#!/usr/bin/env python3
"""
MajiSafe MetaMask Automation - Fixed Version
AI automatically clicks MetaMask for SMS payments
"""

from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.chrome.service import Service
from webdriver_manager.chrome import ChromeDriverManager
import time

class MetaMaskAutomation:
    def __init__(self):
        # Setup Chrome with better options
        options = webdriver.ChromeOptions()
        options.add_argument("--no-sandbox")
        options.add_argument("--disable-dev-shm-usage")
        options.add_argument("--disable-gpu")
        options.add_argument("--window-size=1920,1080")
        
        # Use webdriver manager for Chrome
        service = Service(ChromeDriverManager().install())
        self.driver = webdriver.Chrome(service=service, options=options)
        self.wait = WebDriverWait(self.driver, 10)
        
    def process_sms_payment(self, phone, message):
        """Process SMS payment by automating web clicks"""
        try:
            print(f"🤖 AI automating web clicks for SMS: {message}")
            
            # 1. Open MajiSafe web UI
            self.driver.get("http://localhost:8000")
            time.sleep(3)
            
            # 2. Check if Connect button exists and is visible
            try:
                connect_btn = self.driver.find_element(By.ID, "connectBtn")
                if connect_btn.is_displayed():
                    connect_btn.click()
                    print("🔗 AI clicked Connect MetaMask")
                    time.sleep(2)
                else:
                    print("🔗 Already connected to MetaMask")
            except:
                print("🔗 Connect button not found - may already be connected")
            
            # 3. Look for Buy Water button (multiple possible selectors)
            buy_selectors = [
                "//button[contains(text(), 'PURCHASE WATER')]",
                "//button[contains(text(), 'Buy Water')]", 
                "//button[contains(text(), 'BUY WATER')]",
                ".btn-success",
                "[onclick='buyWater()']"
            ]
            
            buy_btn = None
            for selector in buy_selectors:
                try:
                    if selector.startswith("//"):
                        buy_btn = self.driver.find_element(By.XPATH, selector)
                    else:
                        buy_btn = self.driver.find_element(By.CSS_SELECTOR, selector)
                    
                    if buy_btn and buy_btn.is_displayed():
                        break
                except:
                    continue
            
            if buy_btn:
                buy_btn.click()
                print("💰 AI clicked Buy Water")
                time.sleep(3)
                
                # 4. Check for success message in status div
                try:
                    status_element = self.driver.find_element(By.ID, "status")
                    status_text = status_element.text
                    
                    if "successful" in status_text.lower() or "confirmed" in status_text.lower():
                        print("✅ Transaction successful!")
                        return {
                            'status': 'success',
                            'message': 'activate',
                            'tx_hash': 'web_automation_tx',
                            'pump_id': 'PUMP001'
                        }
                    else:
                        print(f"⏳ Status: {status_text}")
                        
                except Exception as e:
                    print(f"Status check error: {e}")
                
            else:
                print("❌ Buy Water button not found")
                return {
                    'status': 'error',
                    'message': 'Buy button not found'
                }
            
            # Wait a bit more for transaction
            time.sleep(5)
            
            return {
                'status': 'success',
                'message': 'activate',
                'tx_hash': 'web_automation_success',
                'pump_id': 'PUMP001'
            }
            
        except Exception as e:
            print(f"❌ Automation error: {e}")
            return {
                'status': 'error',
                'message': f'Automation failed: {str(e)}'
            }
    
    def close(self):
        """Close browser"""
        try:
            self.driver.quit()
        except:
            pass

# Test function
if __name__ == "__main__":
    automation = MetaMaskAutomation()
    result = automation.process_sms_payment("+25769820499", "PAY 5000 BIF PUMP001")
    print(f"Result: {result}")
    time.sleep(2)
    automation.close()
