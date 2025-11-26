#!/usr/bin/env python3
"""
MajiSafe DKG Architecture Test Suite
Tests the complete SMS → ESP32 → DKG → OriginTrail flow
"""

import requests
import json
import time
import asyncio

class MajiSafeDKGTester:
    def __init__(self):
        self.dkg_bridge_url = "http://localhost:5002"
        self.web_dashboard_url = "http://localhost:8001"
        
    def test_dkg_bridge_status(self):
        """Test 1: DKG Bridge connectivity"""
        print("🔗 Testing DKG Bridge status...")
        try:
            response = requests.get(f"{self.dkg_bridge_url}/status", timeout=5)
            if response.status_code == 200:
                data = response.json()
                print(f"✅ DKG Bridge online: {data['service']}")
                print(f"   MCP Tools: {data['mcp_tools']}")
                print(f"   Knowledge Assets: {data['knowledge_assets_created']}")
                return True
            else:
                print(f"❌ DKG Bridge error: {response.status_code}")
                return False
        except Exception as e:
            print(f"❌ DKG Bridge offline: {e}")
            return False
    
    def test_sms_payment_processing(self):
        """Test 2: SMS payment with DKG Knowledge Asset creation"""
        print("\n📱 Testing SMS payment processing...")
        
        test_sms = {
            "phone": "+250788123456",
            "amount": 5000,
            "currency": "BIF",
            "pump_id": "PUMP001",
            "coordinates": {"lat": -1.9441, "lng": 30.0619},
            "timestamp": int(time.time()),
            "device_id": "TEST_ESP32"
        }
        
        try:
            response = requests.post(
                f"{self.dkg_bridge_url}/process-sms",
                json=test_sms,
                timeout=10
            )
            
            if response.status_code == 200:
                result = response.json()
                if result["success"]:
                    print(f"✅ SMS processed successfully!")
                    print(f"   Event ID: {result['event_id']}")
                    print(f"   UAL: {result['ual']}")
                    print(f"   Verification Hash: {result['verification_hash']}")
                    return result
                else:
                    print(f"❌ SMS processing failed: {result['error']}")
                    return None
            else:
                print(f"❌ HTTP error: {response.status_code}")
                return None
                
        except Exception as e:
            print(f"❌ SMS test error: {e}")
            return None
    
    def test_knowledge_assets_retrieval(self):
        """Test 3: Knowledge Assets API"""
        print("\n📊 Testing Knowledge Assets retrieval...")
        
        try:
            response = requests.get(f"{self.dkg_bridge_url}/knowledge-assets", timeout=5)
            
            if response.status_code == 200:
                data = response.json()
                assets = data.get("assets", [])
                print(f"✅ Retrieved {len(assets)} Knowledge Assets")
                
                if assets:
                    latest = assets[0]
                    print(f"   Latest Asset: {latest['eventId']}")
                    print(f"   Pump: {latest['pumpId']}")
                    print(f"   Water: {latest['litersDispensed']}L")
                    print(f"   UAL: {latest['ual']}")
                
                return assets
            else:
                print(f"❌ Assets retrieval failed: {response.status_code}")
                return []
                
        except Exception as e:
            print(f"❌ Assets test error: {e}")
            return []
    
    def test_asset_verification(self, ual):
        """Test 4: Knowledge Asset verification"""
        print(f"\n🔐 Testing asset verification for UAL: {ual}")
        
        try:
            response = requests.get(f"{self.dkg_bridge_url}/verify-asset/{ual}", timeout=10)
            
            if response.status_code == 200:
                result = response.json()
                if result["verified"]:
                    print("✅ Asset verification successful!")
                    print(f"   Verified at: {result['timestamp']}")
                    return True
                else:
                    print(f"❌ Asset verification failed: {result['error']}")
                    return False
            else:
                print(f"❌ Verification request failed: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ Verification test error: {e}")
            return False
    
    def test_web_dashboard(self):
        """Test 5: Web dashboard accessibility"""
        print("\n🌐 Testing web dashboard...")
        
        try:
            response = requests.get(f"{self.web_dashboard_url}/dkg_dashboard.html", timeout=5)
            
            if response.status_code == 200:
                print("✅ DKG Dashboard accessible")
                print(f"   URL: {self.web_dashboard_url}/dkg_dashboard.html")
                return True
            else:
                print(f"❌ Dashboard error: {response.status_code}")
                return False
                
        except Exception as e:
            print(f"❌ Dashboard test error: {e}")
            return False
    
    def test_multiple_payments(self):
        """Test 6: Multiple SMS payments for load testing"""
        print("\n🔄 Testing multiple SMS payments...")
        
        test_payments = [
            {"phone": "+250788111111", "amount": 2000, "currency": "BIF", "pump_id": "PUMP001"},
            {"phone": "+254701222222", "amount": 15, "currency": "USD", "pump_id": "PUMP002"},
            {"phone": "+250788333333", "amount": 3000, "currency": "RWF", "pump_id": "PUMP001"},
        ]
        
        successful = 0
        for i, payment in enumerate(test_payments):
            print(f"   Payment {i+1}: {payment['amount']} {payment['currency']}")
            
            payment.update({
                "coordinates": {"lat": -1.9441, "lng": 30.0619},
                "timestamp": int(time.time()) + i,
                "device_id": f"TEST_ESP32_{i}"
            })
            
            try:
                response = requests.post(
                    f"{self.dkg_bridge_url}/process-sms",
                    json=payment,
                    timeout=10
                )
                
                if response.status_code == 200:
                    result = response.json()
                    if result["success"]:
                        successful += 1
                        print(f"   ✅ Payment {i+1} processed: {result['ual']}")
                    else:
                        print(f"   ❌ Payment {i+1} failed: {result['error']}")
                else:
                    print(f"   ❌ Payment {i+1} HTTP error: {response.status_code}")
                    
            except Exception as e:
                print(f"   ❌ Payment {i+1} error: {e}")
            
            time.sleep(1)  # Avoid overwhelming the system
        
        print(f"✅ {successful}/{len(test_payments)} payments successful")
        return successful == len(test_payments)
    
    def run_full_test_suite(self):
        """Run complete test suite"""
        print("🧪 MajiSafe DKG Architecture Test Suite")
        print("=" * 50)
        
        tests_passed = 0
        total_tests = 6
        
        # Test 1: DKG Bridge Status
        if self.test_dkg_bridge_status():
            tests_passed += 1
        
        # Test 2: SMS Payment Processing
        sms_result = self.test_sms_payment_processing()
        if sms_result:
            tests_passed += 1
        
        # Test 3: Knowledge Assets Retrieval
        assets = self.test_knowledge_assets_retrieval()
        if assets:
            tests_passed += 1
        
        # Test 4: Asset Verification (if we have assets)
        if assets and sms_result:
            ual = sms_result.get("ual") or assets[0].get("ual")
            if ual and self.test_asset_verification(ual):
                tests_passed += 1
        else:
            print("\n🔐 Skipping asset verification (no assets available)")
        
        # Test 5: Web Dashboard
        if self.test_web_dashboard():
            tests_passed += 1
        
        # Test 6: Multiple Payments
        if self.test_multiple_payments():
            tests_passed += 1
        
        # Results
        print("\n" + "=" * 50)
        print(f"🎯 Test Results: {tests_passed}/{total_tests} tests passed")
        
        if tests_passed == total_tests:
            print("🎉 ALL TESTS PASSED! MajiSafe DKG architecture is working correctly.")
        elif tests_passed >= total_tests * 0.8:
            print("✅ Most tests passed. Architecture is mostly functional.")
        else:
            print("❌ Multiple test failures. Check system components.")
        
        print("\n📋 Next Steps:")
        print("1. Open DKG Dashboard: http://localhost:8001/dkg_dashboard.html")
        print("2. Check Knowledge Assets: http://localhost:5002/knowledge-assets")
        print("3. Send real SMS to ESP32 for end-to-end testing")
        
        return tests_passed == total_tests

def main():
    """Main test execution"""
    print("🚀 Starting MajiSafe DKG Architecture Tests...")
    print("⚠️  Make sure the following services are running:")
    print("   - DKG Bridge (port 5002)")
    print("   - Web Dashboard (port 8001)")
    print("   - OriginTrail DKG Node (port 8900) [optional]")
    print()
    
    input("Press Enter to continue with tests...")
    
    tester = MajiSafeDKGTester()
    success = tester.run_full_test_suite()
    
    if success:
        print("\n🌊 MajiSafe DKG Architecture is ready for production!")
    else:
        print("\n🔧 Please fix failing components before deployment.")

if __name__ == "__main__":
    main()
