#!/usr/bin/env python3
"""
MajiSafe Unified Launcher
Starts Web UI + AI Bridge + Web3 all in one command
"""

import subprocess
import threading
import time
import os
import sys
import webbrowser
from pathlib import Path

class MajiSafeLauncher:
    def __init__(self):
        self.base_dir = Path(__file__).parent
        self.processes = []
        
    def start_ai_bridge(self):
        """Start AI Bridge in background"""
        print("🤖 Starting AI Bridge...")
        
        ai_dir = self.base_dir / "src" / "ai-bridge"
        
        # Activate venv and start AI bridge
        cmd = [
            "bash", "-c", 
            f"cd {ai_dir} && source venv/bin/activate && python majisafe_ai.py"
        ]
        
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        self.processes.append(process)
        
        # Monitor AI bridge output in separate thread
        def monitor_ai():
            for line in process.stdout:
                print(f"🤖 {line.strip()}")
        
        threading.Thread(target=monitor_ai, daemon=True).start()
        
        # Wait for AI bridge to start
        time.sleep(3)
        print("✅ AI Bridge started on port 5001")
        """Start AI Bridge in background"""
        print("🤖 Starting AI Bridge...")
        
        ai_dir = self.base_dir / "src" / "ai-bridge"
        
        # Activate venv and start AI bridge
        cmd = [
            "bash", "-c", 
            f"cd {ai_dir} && source venv/bin/activate && python majisafe_ai.py"
        ]
        
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        self.processes.append(process)
        
        # Monitor AI bridge output in separate thread
        def monitor_ai():
            for line in process.stdout:
                print(f"🤖 {line.strip()}")
        
        threading.Thread(target=monitor_ai, daemon=True).start()
        
        # Wait for AI bridge to start
        time.sleep(3)
        print("✅ AI Bridge started on port 5001")
    
    def start_web_server(self):
        """Start Web UI server"""
        print("🌐 Starting Web UI...")
        
        web_dir = self.base_dir / "src" / "web"
        
        cmd = ["python3", "-m", "http.server", "8000"]
        
        process = subprocess.Popen(
            cmd,
            cwd=web_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        self.processes.append(process)
        
        # Monitor web server output
        def monitor_web():
            for line in process.stdout:
                print(f"🌐 {line.strip()}")
        
        threading.Thread(target=monitor_web, daemon=True).start()
        
        time.sleep(2)
        print("✅ Web UI started on port 8000")
    
    def open_browser(self):
        """Open browser to MajiSafe UI"""
        print("🚀 Opening MajiSafe in browser...")
        time.sleep(1)
        webbrowser.open("http://localhost:8000")
    
    def start_all(self):
        """Start all MajiSafe services"""
        print("🚰 MajiSafe Unified Launcher")
        print("=" * 40)
        
        try:
            # Start AI Bridge (includes MetaMask monitoring)
            self.start_ai_bridge()
            
            # Start Web UI
            self.start_web_server()
            
            # DON'T open browser automatically
            # self.open_browser()  # REMOVED
            
            print("\n🎉 MajiSafe is LIVE!")
            print("🌐 Web UI: http://localhost:8000")
            print("🤖 AI Bridge: http://localhost:5001")
            print("🦊 MetaMask Monitor: Active")
            print("📱 SMS: Send to +25766303339")
            print("\n💧 Ready to convert SMS to clean water!")
            print("👤 YOU manually open: http://localhost:8000")
            print("🤖 AI will auto-confirm MetaMask")
            print("\nPress Ctrl+C to stop all services...")
            
            # Keep main thread alive
            while True:
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\n🛑 Stopping MajiSafe services...")
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
    launcher = MajiSafeLauncher()
    launcher.start_all()
