// Maji-Vibe Web Interface - Minimal Water Purchase App
import { ethers } from 'ethers';

class WaterApp {
    constructor() {
        this.provider = null;
        this.signer = null;
        this.contract = null;
        this.init();
    }

    async init() {
        if (typeof window.ethereum !== 'undefined') {
            this.provider = new ethers.providers.Web3Provider(window.ethereum);
            await this.setupContract();
            this.setupUI();
        }
    }

    async setupContract() {
        const contractAddress = '0x...'; // Deploy address
        const abi = [
            "function buyWater(bytes32 pumpId) payable",
            "function waterCredits(address) view returns (uint256)",
            "event WaterPurchased(address indexed user, uint256 credits, bytes32 pumpId)"
        ];
        
        this.contract = new ethers.Contract(contractAddress, abi, this.provider);
    }

    setupUI() {
        document.getElementById('connectWallet').onclick = () => this.connectWallet();
        document.getElementById('buyWater').onclick = () => this.buyWater();
    }

    async connectWallet() {
        try {
            await window.ethereum.request({ method: 'eth_requestAccounts' });
            this.signer = this.provider.getSigner();
            const address = await this.signer.getAddress();
            document.getElementById('walletAddress').textContent = address;
            
            // Get water credits
            const credits = await this.contract.waterCredits(address);
            document.getElementById('waterCredits').textContent = credits.toString();
        } catch (error) {
            console.error('Wallet connection failed:', error);
        }
    }

    async buyWater() {
        if (!this.signer) return;
        
        const pumpId = document.getElementById('pumpId').value;
        const amount = document.getElementById('amount').value;
        
        try {
            const tx = await this.contract.connect(this.signer).buyWater(
                ethers.utils.formatBytes32String(pumpId),
                { value: ethers.utils.parseEther((amount * 0.001).toString()) }
            );
            
            document.getElementById('status').textContent = 'Transaction sent...';
            await tx.wait();
            document.getElementById('status').textContent = 'Water purchased! SMS sent to pump.';
        } catch (error) {
            document.getElementById('status').textContent = 'Transaction failed: ' + error.message;
        }
    }
}

// Initialize app
new WaterApp();
