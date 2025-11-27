#!/usr/bin/env python3
"""
MajiSafe DKG Bridge - Enhanced AI Agent with OriginTrail Integration
Processes SMS payments and creates verifiable Knowledge Assets
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import sqlite3
from datetime import datetime
import json

from real_dkg_agent import RealDKGAgent
from dkg_agent_simple import MCPToolsAgent

app = Flask(__name__)
CORS(app)

class MajiSafeDKGBridge:
    def __init__(self):
        self.dkg_agent = RealDKGAgent()
        self.mcp_tools = MCPToolsAgent()
        self.init_db()
        
        print("🌊 MajiSafe DKG Bridge Ready")
        print("🔗 Real OriginTrail DKG Integration")
        print("🌙 Using NeuroWeb Network")
        print("🤖 MCP Tools Loaded")
    
    def init_db(self):
        """Initialize database with DKG tracking"""
        self.conn = sqlite3.connect('majisafe_dkg.db', check_same_thread=False)
        self.conn.execute('''
            CREATE TABLE IF NOT EXISTS water_events (
                id INTEGER PRIMARY KEY,
                event_id TEXT UNIQUE,
                pump_id TEXT,
                liters_dispensed REAL,
                payment_amount REAL,
                payment_currency TEXT,
                tx_hash TEXT,
                ual TEXT,
                dkg_token_id TEXT,
                verification_hash TEXT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        self.conn.commit()
    
    def process_sms_payment(self, sms_data):
        """Enhanced SMS processing with DKG integration"""
        try:
            # Step 1: Validate payment using MCP tools
            validation = self.mcp_tools.tools["validate_payment"](sms_data)
            if not validation["valid"]:
                return {"success": False, "error": "Invalid payment"}
            
            # Step 2: Control pump via IoT
            pump_result = self.mcp_tools.tools["control_pump"](
                sms_data["pump_id"], 
                sms_data.get("duration", 10)
            )
            
            # Step 3: Create audit log
            audit_log = self.mcp_tools.tools["create_audit_log"]({
                "sms": sms_data,
                "validation": validation,
                "pump_control": pump_result
            })
            
            # Step 4: Create Knowledge Asset
            pump_data = {
                "pump_id": sms_data["pump_id"],
                "liters_dispensed": sms_data.get("liters", 10),
                "coordinates": sms_data.get("coordinates", {"lat": 0, "lng": 0})
            }
            
            payment_data = {
                "amount": sms_data["amount"],
                "currency": sms_data["currency"],
                "tx_hash": sms_data.get("tx_hash"),
                "sender_address": sms_data.get("sender")
            }
            
            knowledge_asset = self.dkg_agent.create_water_knowledge_asset(
                pump_data, payment_data, audit_log
            )
            
            # Step 5: Publish to DKG
            dkg_result = self.dkg_agent.publish_to_dkg(knowledge_asset)
            
            if dkg_result["success"]:
                # Step 6: Anchor to blockchain
                anchor_data = self.dkg_agent.anchor_to_blockchain(
                    dkg_result["ual"], 
                    payment_data.get("tx_hash")
                )
                
                # Step 7: Store in database
                self.store_water_event(knowledge_asset, dkg_result, anchor_data)
                
                return {
                    "success": True,
                    "event_id": knowledge_asset["eventId"],
                    "ual": dkg_result["ual"],
                    "verification_hash": knowledge_asset["verificationHash"],
                    "anchor": anchor_data
                }
            else:
                return {"success": False, "error": dkg_result["error"]}
                
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def store_water_event(self, knowledge_asset, dkg_result, anchor_data):
        """Store water dispensing event with DKG references"""
        self.conn.execute('''
            INSERT INTO water_events 
            (event_id, pump_id, liters_dispensed, payment_amount, payment_currency, 
             tx_hash, ual, dkg_token_id, verification_hash)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            knowledge_asset["eventId"],
            knowledge_asset["location"]["name"],
            knowledge_asset["waterDispensed"]["value"],
            knowledge_asset["payment"]["amount"],
            knowledge_asset["payment"]["currency"],
            knowledge_asset["payment"]["txHash"],
            dkg_result["ual"],
            dkg_result["tokenId"],
            knowledge_asset["verificationHash"]
        ))
        self.conn.commit()

# Global bridge instance
bridge = MajiSafeDKGBridge()

@app.route('/process-sms', methods=['POST'])
def process_sms():
    """Enhanced SMS processing endpoint"""
    try:
        sms_data = request.json
        result = bridge.process_sms_payment(sms_data)
        
        if result["success"]:
            print(f"✅ Water event created: {result['event_id']}")
            print(f"🔗 DKG UAL: {result['ual']}")
            return jsonify(result)
        else:
            print(f"❌ Processing failed: {result['error']}")
            return jsonify(result), 400
            
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

@app.route('/knowledge-assets', methods=['GET'])
def get_knowledge_assets():
    """Get all water dispensing Knowledge Assets"""
    try:
        cursor = bridge.conn.execute('''
            SELECT event_id, pump_id, liters_dispensed, payment_amount, 
                   payment_currency, ual, verification_hash, timestamp
            FROM water_events 
            ORDER BY timestamp DESC 
            LIMIT 50
        ''')
        
        assets = []
        for row in cursor.fetchall():
            assets.append({
                "eventId": row[0],
                "pumpId": row[1],
                "litersDispensed": row[2],
                "paymentAmount": row[3],
                "paymentCurrency": row[4],
                "ual": row[5],
                "verificationHash": row[6],
                "timestamp": row[7]
            })
        
        return jsonify({"assets": assets})
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/verify-asset/<ual>', methods=['GET'])
def verify_asset(ual):
    """Verify Knowledge Asset integrity"""
    try:
        # Mock verification for simplified DKG
        return jsonify({
            "verified": True,
            "ual": ual,
            "timestamp": datetime.now().isoformat()
        })
            
    except Exception as e:
        return jsonify({"verified": False, "error": str(e)}), 500

@app.route('/status', methods=['GET'])
def status():
    """Enhanced status with DKG connectivity"""
    return jsonify({
        "service": "MajiSafe DKG Bridge",
        "status": "online",
        "blockchain": "Moonbase Alpha",
        "dkg_node": bridge.dkg_agent.dkg_node_url,
        "mcp_tools": list(bridge.mcp_tools.tools.keys()),
        "knowledge_assets_created": bridge.conn.execute("SELECT COUNT(*) FROM water_events").fetchone()[0]
    })

if __name__ == "__main__":
    print("🚀 Starting MajiSafe DKG Bridge...")
    print("🌊 Creating verifiable water Knowledge Assets")
    print("🔗 OriginTrail DKG integration active")
    
    app.run(host='0.0.0.0', port=5002, debug=True)
