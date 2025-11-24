#!/bin/bash
# MajiChain Simple Launcher

echo "🚰 Starting MajiChain - SMS to Water System"
echo "=========================================="

# Kill any existing processes
pkill -f "majichain_ai.py" 2>/dev/null || true
pkill -f "python3 -m http.server" 2>/dev/null || true

# Start AI Bridge
echo "🤖 Starting AI Bridge..."
cd src/ai-bridge
source venv/bin/activate
python majichain_ai.py &
AI_PID=$!

# Start Web UI
echo "🌐 Starting Web UI..."
cd ../web
python3 -m http.server 8000 &
WEB_PID=$!

cd ../..

echo ""
echo "🎉 MajiChain is LIVE!"
echo "🌐 Web UI: http://localhost:8000"
echo "🤖 AI Bridge: http://localhost:5001"
echo "🦊 MetaMask Monitor: Active"
echo "📱 SMS: Send to +25766303339"
echo ""
echo "💧 Ready to convert SMS to clean water!"
echo "👤 YOU manually open: http://localhost:8000"
echo "🤖 AI will auto-confirm MetaMask"
echo ""
echo "Press Ctrl+C to stop all services..."

# Wait for Ctrl+C
trap 'echo "🛑 Stopping services..."; kill $AI_PID $WEB_PID 2>/dev/null; exit' INT
wait
