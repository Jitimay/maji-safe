// MajiSafe MetaMask Integration for Real Transactions

const MAJISAFE_CONTRACT_ADDRESS = "0x4933781A5DDC86bdF9c9C9795647e763E0429E28";
const MAJISAFE_ABI = [
    "function buyWater(bytes32 pumpId) payable",
    "function waterCredits(address) view returns (uint256)",
    "function creditPrice() view returns (uint256)",
    "event WaterPurchased(address indexed user, uint256 credits, bytes32 pumpId)"
];

const MOONBASE_ALPHA_CONFIG = {
    chainId: '0x507', // 1287 Moonbase Alpha
    chainName: 'Moonbase Alpha',
    nativeCurrency: { name: 'DEV', symbol: 'DEV', decimals: 18 },
    rpcUrls: ['https://rpc.api.moonbase.moonbeam.network'],
    blockExplorerUrls: ['https://moonbase.moonscan.io']
};

class MajiSafeMetaMask {
    constructor() {
        this.provider = null;
        this.signer = null;
        this.contract = null;
        this.account = null;
    }

    async connectWallet() {
        if (!window.ethereum) {
            throw new Error('MetaMask not installed');
        }

        try {
            // Request account access
            const accounts = await window.ethereum.request({
                method: 'eth_requestAccounts'
            });

            this.account = accounts[0];
            this.provider = new ethers.providers.Web3Provider(window.ethereum);
            this.signer = this.provider.getSigner();

            // Switch to Moonbase Alpha
            await this.switchToMoonbase();

            // Initialize contract
            this.contract = new ethers.Contract(
                MAJISAFE_CONTRACT_ADDRESS,
                MAJISAFE_ABI,
                this.signer
            );

            return {
                success: true,
                account: this.account,
                network: 'Moonbase Alpha'
            };

        } catch (error) {
            throw new Error(`Connection failed: ${error.message}`);
        }
    }

    async switchToMoonbase() {
        try {
            await window.ethereum.request({
                method: 'wallet_switchEthereumChain',
                params: [{ chainId: MOONBASE_ALPHA_CONFIG.chainId }]
            });
        } catch (switchError) {
            // Network doesn't exist, add it
            if (switchError.code === 4902) {
                await window.ethereum.request({
                    method: 'wallet_addEthereumChain',
                    params: [MOONBASE_ALPHA_CONFIG]
                });
            } else {
                throw switchError;
            }
        }
    }

    async buyWaterCredits(pumpId, ethAmount) {
        if (!this.contract) {
            throw new Error('Wallet not connected');
        }

        try {
            // Convert pump ID to bytes32
            const pumpIdBytes32 = ethers.utils.formatBytes32String(pumpId);
            
            // Convert ETH amount to wei
            const valueWei = ethers.utils.parseEther(ethAmount.toString());

            // Send transaction
            const tx = await this.contract.buyWater(pumpIdBytes32, {
                value: valueWei,
                gasLimit: 100000
            });

            return {
                success: true,
                txHash: tx.hash,
                pumpId: pumpId,
                amount: ethAmount
            };

        } catch (error) {
            throw new Error(`Transaction failed: ${error.message}`);
        }
    }

    async getWaterCredits() {
        if (!this.contract || !this.account) {
            return 0;
        }

        try {
            const credits = await this.contract.waterCredits(this.account);
            return ethers.utils.formatUnits(credits, 0);
        } catch (error) {
            console.error('Error getting credits:', error);
            return 0;
        }
    }

    async getCreditPrice() {
        if (!this.contract) {
            return '0.001';
        }

        try {
            const price = await this.contract.creditPrice();
            return ethers.utils.formatEther(price);
        } catch (error) {
            console.error('Error getting price:', error);
            return '0.001';
        }
    }

    onAccountsChanged(callback) {
        if (window.ethereum) {
            window.ethereum.on('accountsChanged', callback);
        }
    }

    onChainChanged(callback) {
        if (window.ethereum) {
            window.ethereum.on('chainChanged', callback);
        }
    }
}

// Global instance
const majiSafeWallet = new MajiSafeMetaMask();
