#!/bin/bash

# MajiSafe Complete System Launcher
# Starts DKG Bridge + Web Server + All Services

echo "🌊 Starting MajiSafe DKG System"
echo "==============================="

# Kill any existing processes
echo "🧹 Cleaning up existing processes..."
pkill -f "majisafe_dkg_bridge.py" 2>/dev/null
pkill -f "python3 -m http.server" 2>/dev/null
sleep 2

# Start DKG Bridge in background
echo "🔗 Starting DKG Bridge..."
cd src/ai-bridge
source venv/bin/activate
python majisafe_dkg_bridge.py &
DKG_PID=$!
cd ../..

# Wait for DKG Bridge to start
sleep 3

# Start Web Server in background
echo "🌐 Starting Web Server..."
cd src/web
python3 -m http.server 8002 &
WEB_PID=$!
cd ../..

# Wait for services to initialize
sleep 2

echo ""
echo "🎉 MajiSafe System Started!"
echo "=========================="
echo "🔗 DKG Bridge:     http://localhost:5002"
echo "🌐 Web Interface:  http://localhost:8002"
echo "🦊 MetaMask Test:  http://localhost:8002/metamask_test_interface.html"
echo "📊 DKG Dashboard:  http://localhost:8002/dkg_dashboard.html"
echo "🧪 Test Interface: http://localhost:8002/test_interface.html"
echo ""
echo "🌙 Network: Moonbase Alpha"
echo "💰 Get DEV tokens: https://apps.moonbeam.network/moonbase-alpha/faucet/"
echo ""
echo "📱 ESP32 Commands (Serial Monitor):"
echo "   test                    - Test relay"
echo "   sms:PAY 5000 BIF PUMP001 - Simulate SMS"
echo "   status                  - Show status"
echo ""
echo "Press Ctrl+C to stop all services..."

# Function to cleanup on exit
cleanup() {
    echo ""
    echo "🛑 Stopping MajiSafe services..."
    kill $DKG_PID 2>/dev/null
    kill $WEB_PID 2>/dev/null
    pkill -f "majisafe_dkg_bridge.py" 2>/dev/null
    pkill -f "python3 -m http.server" 2>/dev/null
    echo "✅ All services stopped"
    exit 0
}

# Set trap to cleanup on Ctrl+C
trap cleanup SIGINT SIGTERM

# Keep script running
while true; do
    sleep 1
done
