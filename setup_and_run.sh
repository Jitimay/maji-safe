#!/bin/bash
# MajiSafe Complete Setup and Launcher

echo "🚰 MajiSafe Complete Setup & Launch"
echo "===================================="

# Check if Python3 is installed
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 not found. Please install Python3 first."
    exit 1
fi

# Setup AI Bridge
echo "🤖 Setting up AI Bridge..."
cd src/ai-bridge

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    echo "📦 Creating Python virtual environment..."
    python3 -m venv venv
fi

# Activate and install dependencies
echo "📦 Installing Python dependencies..."
source venv/bin/activate
pip install --upgrade pip
pip install -r requirements.txt

# Kill any existing processes
echo "🧹 Cleaning up existing processes..."
pkill -f "majisafe_ai.py" 2>/dev/null || true
pkill -f "python3 -m http.server" 2>/dev/null || true

# Start AI Bridge
echo "🤖 Starting AI Bridge..."
python majisafe_ai.py &
AI_PID=$!

# Start Web UI
echo "🌐 Starting Web UI..."
cd ../web
python3 -m http.server 8000 &
WEB_PID=$!

cd ../..

echo ""
echo "🎉 MajiSafe is LIVE!"
echo "🌐 Web UI: http://localhost:8000"
echo "🤖 AI Bridge: http://localhost:5001"
echo "📱 SMS Test: python3 test_majisafe.py"
echo ""
echo "💧 Ready to convert SMS to clean water!"
echo "👤 Open: http://localhost:8000"
echo ""
echo "Press Ctrl+C to stop all services..."

# Wait for Ctrl+C
trap 'echo "🛑 Stopping services..."; kill $AI_PID $WEB_PID 2>/dev/null; exit' INT
wait
