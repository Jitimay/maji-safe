// MajiSafe Web Interface - Futuristic Water Control System

const contractAddress = "0x4933781A5DDC86bdF9c9C9795647e763E0429E28";
const contractABI = [
    "function buyWater(bytes32 pumpId) payable",
    "function waterCredits(address) view returns (uint256)",
    "function creditPrice() view returns (uint256)",
    "function activatePump(bytes32 pumpId, uint256 liters) external",
    "event WaterPurchased(address indexed user, uint256 credits, bytes32 pumpId)",
    "event PumpActivated(bytes32 indexed pumpId, uint256 liters)"
];

const MOONBASE_ALPHA_CONFIG = {
    chainId: '0x507', // 1287 Moonbase Alpha
    chainName: 'Moonbase Alpha',
    nativeCurrency: { name: 'DEV', symbol: 'DEV', decimals: 18 },
    rpcUrls: ['https://rpc.api.moonbase.moonbeam.network'],
    blockExplorerUrls: ['https://moonbase.moonscan.io']
};

let provider = null;
let signer = null;
let contract = null;
let pumpActive = false;
let smsPaymentReceived = false; // Track SMS payment status

// Sci-fi status messages
const statusMessages = [
    "QUANTUM ENTANGLEMENT ESTABLISHED...",
    "BLOCKCHAIN SYNCHRONIZATION COMPLETE",
    "AI NEURAL NETWORKS ONLINE",
    "SATELLITE UPLINK CONFIRMED",
    "WATER DISTRIBUTION MATRIX READY"
];

// Wait for page to load
window.addEventListener('load', async () => {
    await initializeSystem();
});

async function initializeSystem() {
    // Animated startup sequence
    document.getElementById('status').innerHTML = 'BOOTING MAJISAFE PROTOCOL...';
    
    await sleep(1000);
    
    // Check if ethers is loaded
    if (typeof ethers === 'undefined') {
        document.getElementById('status').innerHTML = 'ERROR: BLOCKCHAIN LIBRARY OFFLINE';
        return;
    }

    // Check for MetaMask
    if (typeof window.ethereum !== 'undefined') {
        document.getElementById('status').innerHTML = 'METAMASK DETECTED - READY FOR CONNECTION';
        document.getElementById('connectBtn').style.display = 'block';
        
        // Check if already connected
        const accounts = await window.ethereum.request({ method: 'eth_accounts' });
        if (accounts.length > 0) {
            await connectWallet();
        }
    } else {
        document.getElementById('status').innerHTML = 'ERROR: METAMASK NOT DETECTED<br>PLEASE INSTALL METAMASK EXTENSION';
    }
    
    // Start telemetry updates
    updateTelemetry();
}

async function connectWallet() {
    try {
        document.getElementById('status').innerHTML = 'ESTABLISHING QUANTUM LINK...';
        
        // Request account access
        await window.ethereum.request({ method: 'eth_requestAccounts' });
        
        // Check network
        const chainId = await window.ethereum.request({ method: 'eth_chainId' });
        if (chainId !== MOONBASE_ALPHA_CONFIG.chainId) {
            await switchToNetwork();
        }
        
        provider = new ethers.providers.Web3Provider(window.ethereum);
        signer = provider.getSigner();
        
        const address = await signer.getAddress();
        document.getElementById('status').innerHTML = `CONNECTED: ${address.slice(0,8)}...${address.slice(-6)}<br>STATUS: QUANTUM LINK ESTABLISHED`;
        
        contract = new ethers.Contract(contractAddress, contractABI, signer);
        
        // Show purchase section with disabled button
        document.getElementById('purchaseSection').style.display = 'block';
        document.getElementById('connectBtn').style.display = 'none';
        
        // Initially disable purchase button until SMS payment
        updatePurchaseButton();
        
        // Start checking for SMS payments
        checkForSMSPayments();
        
        // Animate connection success
        animateSuccess();
        
    } catch (error) {
        console.error('Connection failed:', error);
        document.getElementById('status').innerHTML = `CONNECTION FAILED: ${error.message}`;
    }
}

async function switchToNetwork() {
    try {
        await window.ethereum.request({
            method: 'wallet_switchEthereumChain',
            params: [{ chainId: MOONBASE_ALPHA_CONFIG.chainId }]
        });
    } catch (error) {
        if (error.code === 4902) {
            await window.ethereum.request({
                method: 'wallet_addEthereumChain',
                params: [MOONBASE_ALPHA_CONFIG]
            });
        }
    }
}

async function checkForSMSPayments() {
    // Check AI Bridge for SMS payment status
    console.log('Starting SMS payment monitoring...');
    
    setInterval(async () => {
        try {
            console.log('Checking SMS status...');
            const response = await fetch('http://localhost:5001/sms-status');
            const data = await response.json();
            
            console.log('SMS Status:', data);
            
            if (data.payment_received && !smsPaymentReceived) {
                console.log('SMS Payment detected!');
                smsPaymentReceived = true;
                updatePurchaseButton();
                
                document.getElementById('status').innerHTML = 
                    `SMS PAYMENT RECEIVED! 📱<br>FROM: ${data.phone}<br>AMOUNT: ${data.amount}<br>READY FOR BLOCKCHAIN CONFIRMATION`;
            }
        } catch (error) {
            console.error('SMS check error:', error);
            // AI Bridge not responding, keep checking
        }
    }, 2000); // Check every 2 seconds
}

function updatePurchaseButton() {
    const buyButton = document.getElementById('buyWaterBtn');
    const statusDiv = document.getElementById('purchaseStatus');
    
    if (smsPaymentReceived) {
        // Enable button - SMS payment received
        buyButton.disabled = false;
        buyButton.className = 'btn btn-success';
        buyButton.innerHTML = '🚰 PURCHASE WATER';
        statusDiv.innerHTML = 'SMS PAYMENT CONFIRMED<br>READY FOR BLOCKCHAIN TRANSACTION';
    } else {
        // Disable button - waiting for SMS
        buyButton.disabled = true;
        buyButton.className = 'btn btn-disabled';
        buyButton.innerHTML = '⏳ WAITING FOR SMS PAYMENT';
        statusDiv.innerHTML = 'SEND SMS TO: +25766303339<br>FORMAT: PAY 5000 BIF PUMP001';
    }
}

async function buyWater() {
    if (!contract || !smsPaymentReceived) {
        document.getElementById('status').innerHTML = 'ERROR: SMS PAYMENT REQUIRED FIRST';
        return;
    }
    
    try {
        document.getElementById('status').innerHTML = 'PROCESSING BLOCKCHAIN TRANSACTION...';
        
        const pumpId = ethers.utils.formatBytes32String("PUMP001");
        const tx = await contract.buyWater(pumpId, {
            value: ethers.utils.parseEther("0.001")
        });
        
        document.getElementById('status').innerHTML = `TRANSACTION BROADCAST: ${tx.hash.slice(0,12)}...<br>AWAITING BLOCKCHAIN CONFIRMATION`;
        
        await tx.wait();
        
        document.getElementById('status').innerHTML = 'BLOCKCHAIN CONFIRMED! ✅<br>WATER CREDITS ACQUIRED<br>READY FOR PUMP ACTIVATION';
        
        // Reset button to gray immediately after transaction
        smsPaymentReceived = false;
        updatePurchaseButton();
        
        // Notify AI Bridge that blockchain is confirmed
        fetch('http://localhost:5001/blockchain-confirmed', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ tx_hash: tx.hash })
        });
        
        // Animate success
        animateSuccess();
        
    } catch (error) {
        console.error('Purchase failed:', error);
        document.getElementById('status').innerHTML = `TRANSACTION FAILED: ${error.message}`;
    }
}

async function activatePump() {
    if (pumpActive) return;
    
    pumpActive = true;
    document.getElementById('pumpStatusText').innerHTML = 'ACTIVATING...';
    document.getElementById('pumpIndicator').classList.add('active');
    document.getElementById('flowAnimation').classList.add('active');
    
    document.getElementById('status').innerHTML = 'PUMP ACTIVATION INITIATED...<br>ESTABLISHING CONNECTION TO HARDWARE';
    
    // Simulate pump activation
    await sleep(2000);
    
    document.getElementById('pumpStatusText').innerHTML = 'ONLINE - DISPENSING WATER';
    document.getElementById('status').innerHTML = 'PUMP ACTIVATED SUCCESSFULLY! 🚰<br>WATER FLOW: 2.5 L/MIN<br>PRESSURE: OPTIMAL';
    
    // Auto-stop after 10 seconds
    setTimeout(() => {
        if (pumpActive) deactivatePump();
    }, 10000);
}

async function deactivatePump() {
    pumpActive = false;
    document.getElementById('pumpStatusText').innerHTML = 'OFFLINE';
    document.getElementById('pumpIndicator').classList.remove('active');
    document.getElementById('flowAnimation').classList.remove('active');
    
    document.getElementById('status').innerHTML = 'PUMP DEACTIVATED ⏹️<br>WATER FLOW: STOPPED<br>SYSTEM STANDBY MODE';
}

function refreshTelemetry() {
    const randomMessage = statusMessages[Math.floor(Math.random() * statusMessages.length)];
    document.getElementById('telemetry').innerHTML = `
        NETWORK: BASE SEPOLIA (CHAIN ID: 84532)<br>
        CONTRACT: ${contractAddress.slice(0,8)}...${contractAddress.slice(-6)}<br>
        BLOCK HEIGHT: ${Math.floor(Math.random() * 1000000) + 14332217}<br>
        ${randomMessage}
    `;
}

function updateTelemetry() {
    setInterval(() => {
        const timestamp = new Date().toLocaleTimeString();
        document.getElementById('telemetry').innerHTML = `
            NETWORK: BASE SEPOLIA (84532)<br>
            CONTRACT: DEPLOYED & VERIFIED<br>
            SENSORS: ONLINE<br>
            AI BRIDGE: ACTIVE<br>
            LAST UPDATE: ${timestamp}
        `;
    }, 5000);
}

function animateSuccess() {
    // Add success animation effects
    document.body.style.animation = 'none';
    setTimeout(() => {
        document.body.style.animation = '';
    }, 100);
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}
