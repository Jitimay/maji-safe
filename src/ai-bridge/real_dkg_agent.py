#!/usr/bin/env python3
"""Real OriginTrail DKG Agent"""

import requests
import json
from datetime import datetime

class RealDKGAgent:
    def __init__(self):
        self.dkg_node_url = "http://localhost:8900"
        self.network = "otp:20430"
        
    def create_water_knowledge_asset(self, pump_data, payment_data, audit_log):
        """Create OriginTrail Knowledge Asset"""
        return {
            "@context": ["https://schema.org/", "https://www.w3.org/ns/dkg#"],
            "@type": "WaterDispenseEvent",
            "@id": f"water-{pump_data['pump_id']}-{int(datetime.now().timestamp())}",
            "name": f"Water Dispensed at {pump_data['pump_id']}",
            "description": f"Blockchain-verified water dispensing event",
            "location": {
                "@type": "Place",
                "name": f"Water Pump {pump_data['pump_id']}",
                "geo": pump_data.get('coordinates', {})
            },
            "waterDispensed": {
                "@type": "QuantitativeValue", 
                "value": pump_data['liters_dispensed'],
                "unitCode": "LTR"
            },
            "payment": {
                "@type": "PaymentEvent",
                "amount": payment_data['amount'],
                "currency": payment_data['currency'],
                "txHash": payment_data.get('tx_hash')
            },
            "timestamp": datetime.now().isoformat(),
            "auditTrail": audit_log
        }
    
    def publish_to_dkg(self, knowledge_asset):
        """Publish to real OriginTrail DKG"""
        try:
            response = requests.post(
                f"{self.dkg_node_url}/assets",
                json={
                    "public": knowledge_asset,
                    "options": {
                        "epochsNum": 5,
                        "maxNumberOfRetries": 3,
                        "frequency": 1
                    }
                },
                headers={"Content-Type": "application/json"}
            )
            
            if response.status_code == 200:
                result = response.json()
                return {
                    "success": True,
                    "ual": result["UAL"],
                    "tokenId": result["publicAssertionId"]
                }
            else:
                return {"success": False, "error": f"DKG error: {response.text}"}
                
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def get_asset(self, ual):
        """Retrieve Knowledge Asset by UAL"""
        try:
            response = requests.get(f"{self.dkg_node_url}/assets/{ual}")
            return response.json() if response.status_code == 200 else None
        except:
            return None
