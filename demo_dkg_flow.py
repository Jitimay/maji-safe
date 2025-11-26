#!/usr/bin/env python3
"""
MajiSafe DKG Demo - Quick Flow Test
Demonstrates the complete SMS → DKG → Blockchain flow
"""

import requests
import json
import time

def demo_sms_to_dkg():
    """Demo: SMS payment creates Knowledge Asset"""
    
    print("🌊 MajiSafe DKG Architecture Demo")
    print("=" * 40)
    
    # Step 1: Simulate SMS payment
    print("📱 Step 1: Simulating SMS payment...")
    sms_data = {
        "phone": "+250788123456",
        "amount": 5000,
        "currency": "BIF", 
        "pump_id": "PUMP001",
        "coordinates": {"lat": -1.9441, "lng": 30.0619},
        "timestamp": int(time.time()),
        "device_id": "DEMO_ESP32"
    }
    
    print(f"   SMS: PAY {sms_data['amount']} {sms_data['currency']} {sms_data['pump_id']}")
    print(f"   From: {sms_data['phone']}")
    
    # Step 2: Send to DKG Bridge
    print("\n🔗 Step 2: Processing via DKG Bridge...")
    try:
        response = requests.post(
            "http://localhost:5002/process-sms",
            json=sms_data,
            timeout=10
        )
        
        if response.status_code == 200:
            result = response.json()
            if result["success"]:
                print("✅ SMS processed successfully!")
                print(f"   Event ID: {result['event_id']}")
                print(f"   UAL: {result['ual']}")
                print(f"   Verification Hash: {result['verification_hash'][:16]}...")
                
                # Step 3: Verify Knowledge Asset
                print("\n🔐 Step 3: Verifying Knowledge Asset...")
                verify_response = requests.get(
                    f"http://localhost:5002/verify-asset/{result['ual']}",
                    timeout=5
                )
                
                if verify_response.status_code == 200:
                    verify_result = verify_response.json()
                    if verify_result["verified"]:
                        print("✅ Knowledge Asset verified!")
                        print(f"   Verified at: {verify_result['timestamp']}")
                    else:
                        print("❌ Verification failed")
                else:
                    print("❌ Verification request failed")
                
                # Step 4: Show in dashboard
                print("\n🌐 Step 4: View in DKG Dashboard...")
                print("   Open: http://localhost:8001/dkg_dashboard.html")
                print("   Your Knowledge Asset should appear in the dashboard")
                
                return True
            else:
                print(f"❌ Processing failed: {result['error']}")
                return False
        else:
            print(f"❌ HTTP error: {response.status_code}")
            return False
            
    except Exception as e:
        print(f"❌ Demo error: {e}")
        print("\n💡 Make sure DKG Bridge is running:")
        print("   python start_majisafe_dkg.py")
        return False

def show_knowledge_assets():
    """Show all created Knowledge Assets"""
    print("\n📊 Current Knowledge Assets:")
    print("-" * 30)
    
    try:
        response = requests.get("http://localhost:5002/knowledge-assets", timeout=5)
        
        if response.status_code == 200:
            data = response.json()
            assets = data.get("assets", [])
            
            if assets:
                for i, asset in enumerate(assets[:5]):  # Show last 5
                    print(f"{i+1}. Event: {asset['eventId']}")
                    print(f"   Pump: {asset['pumpId']}")
                    print(f"   Water: {asset['litersDispensed']}L")
                    print(f"   Payment: {asset['paymentAmount']} {asset['paymentCurrency']}")
                    print(f"   UAL: {asset['ual'][:30]}...")
                    print(f"   Time: {asset['timestamp']}")
                    print()
                
                print(f"Total Assets: {len(assets)}")
            else:
                print("No Knowledge Assets found")
        else:
            print("❌ Failed to retrieve assets")
            
    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    """Main demo execution"""
    print("🚀 Starting MajiSafe DKG Demo...")
    print("\n⚠️  Prerequisites:")
    print("1. DKG Bridge running (port 5002)")
    print("2. Web Dashboard running (port 8001)")
    print("3. OriginTrail DKG node (optional)")
    
    input("\nPress Enter to start demo...")
    
    # Run demo
    success = demo_sms_to_dkg()
    
    if success:
        print("\n🎉 Demo completed successfully!")
        
        # Show existing assets
        show_knowledge_assets()
        
        print("\n🔗 Next Steps:")
        print("1. Open DKG Dashboard: http://localhost:8001/dkg_dashboard.html")
        print("2. Run full test suite: python test_majisafe_dkg.py")
        print("3. Test with real ESP32 hardware")
        
    else:
        print("\n❌ Demo failed. Check system status:")
        print("   python start_majisafe_dkg.py")

if __name__ == "__main__":
    main()
