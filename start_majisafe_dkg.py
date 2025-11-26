#!/usr/bin/env python3
"""
MajiSafe DKG Launcher
Starts the enhanced architecture with OriginTrail DKG integration
"""

import subprocess
import threading
import time
import os
import sys
import webbrowser
from pathlib import Path

class MajiSafeDKGLauncher:
    def __init__(self):
        self.base_dir = Path(__file__).parent
        self.processes = []
        
    def start_dkg_bridge(self):
        """Start DKG Bridge with OriginTrail integration"""
        print("🔗 Starting MajiSafe DKG Bridge...")
        
        ai_dir = self.base_dir / "src" / "ai-bridge"
        
        cmd = [
            "bash", "-c", 
            f"cd {ai_dir} && source venv/bin/activate && python majisafe_dkg_bridge.py"
        ]
        
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        self.processes.append(process)
        
        def monitor_dkg():
            for line in process.stdout:
                print(f"🔗 {line.strip()}")
        
        threading.Thread(target=monitor_dkg, daemon=True).start()
        
        time.sleep(3)
        print("✅ DKG Bridge started on port 5002")
    
    def start_web_dashboard(self):
        """Start DKG Dashboard"""
        print("🌐 Starting DKG Dashboard...")
        
        web_dir = self.base_dir / "src" / "web"
        
        cmd = ["python3", "-m", "http.server", "8001"]
        
        process = subprocess.Popen(
            cmd,
            cwd=web_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        self.processes.append(process)
        
        def monitor_web():
            for line in process.stdout:
                print(f"🌐 {line.strip()}")
        
        threading.Thread(target=monitor_web, daemon=True).start()
        
        time.sleep(2)
        print("✅ DKG Dashboard started on port 8001")
    
    def check_dkg_node(self):
        """Check if OriginTrail DKG node is running"""
        print("🔍 Checking OriginTrail DKG node...")
        
        try:
            import requests
            response = requests.get("http://localhost:8900/info", timeout=5)
            if response.status_code == 200:
                print("✅ OriginTrail DKG node is running")
                return True
            else:
                print("❌ DKG node not responding")
                return False
        except:
            print("❌ DKG node not found on localhost:8900")
            print("💡 Please start OriginTrail DKG node first:")
            print("   npm install -g @origintrail/dkg")
            print("   dkg-node start")
            return False
    
    def start_all(self):
        """Start complete MajiSafe DKG architecture"""
        print("🌊 MajiSafe DKG Architecture Launcher")
        print("=" * 50)
        
        # Check DKG node first
        if not self.check_dkg_node():
            print("\n⚠️  Starting without DKG node (demo mode)")
            time.sleep(2)
        
        try:
            # Start DKG Bridge
            self.start_dkg_bridge()
            
            # Start DKG Dashboard
            self.start_web_dashboard()
            
            print("\n🎉 MajiSafe DKG Architecture is LIVE!")
            print("🔗 DKG Bridge: http://localhost:5002")
            print("🌐 DKG Dashboard: http://localhost:8001/dkg_dashboard.html")
            print("📊 Knowledge Assets: http://localhost:5002/knowledge-assets")
            print("🛰️ OriginTrail DKG: http://localhost:8900")
            print("\n💧 Architecture Flow:")
            print("   📱 SMS → 🤖 ESP32 → 🔗 DKG Bridge → 🌐 OriginTrail DKG → ⛓️ Base L2")
            print("\n🔗 Features:")
            print("   ✅ Verifiable Knowledge Assets")
            print("   ✅ Tamper-proof water dispensing records")
            print("   ✅ MCP tools for agent interactions")
            print("   ✅ On-chain anchoring for trust")
            print("\nPress Ctrl+C to stop all services...")
            
            # Keep main thread alive
            while True:
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\n🛑 Stopping MajiSafe DKG services...")
            self.stop_all()
    
    def stop_all(self):
        """Stop all processes"""
        for process in self.processes:
            try:
                process.terminate()
                process.wait(timeout=5)
            except:
                process.kill()
        
        print("✅ All services stopped")

if __name__ == "__main__":
    launcher = MajiSafeDKGLauncher()
    launcher.start_all()
