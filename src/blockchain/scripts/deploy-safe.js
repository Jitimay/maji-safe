import { ethers } from "hardhat";

async function safeDeploy() {
  try {
    console.log("🚰 MajiSafe Safe Deployment Starting...");
    
    // Pre-deployment checks
    const [deployer] = await ethers.getSigners();
    const balance = await ethers.provider.getBalance(deployer.address);
    const network = await ethers.provider.getNetwork();
    
    console.log("📋 Pre-deployment Check:");
    console.log("   Network:", network.name, `(${network.chainId})`);
    console.log("   Deployer:", deployer.address);
    console.log("   Balance:", ethers.formatEther(balance), "ETH");
    
    // Check minimum balance
    const minBalance = ethers.parseEther("0.005");
    if (balance < minBalance) {
      throw new Error(`Insufficient balance. Need at least 0.005 ETH, have ${ethers.formatEther(balance)} ETH`);
    }
    
    // Deploy contract
    console.log("\n🔨 Deploying WaterBroker...");
    const WaterBroker = await ethers.getContractFactory("WaterBroker");
    
    // Estimate gas first
    const deployTx = WaterBroker.getDeployTransaction();
    const gasEstimate = await ethers.provider.estimateGas(deployTx);
    console.log("   Estimated gas:", gasEstimate.toString());
    
    // Deploy with safety margins
    const waterBroker = await WaterBroker.deploy({
      gasLimit: gasEstimate * 120n / 100n, // 20% buffer
    });
    
    console.log("   Transaction hash:", waterBroker.deploymentTransaction().hash);
    console.log("   Waiting for confirmation...");
    
    await waterBroker.waitForDeployment();
    const address = await waterBroker.getAddress();
    
    console.log("\n✅ Deployment Successful!");
    console.log("   Contract Address:", address);
    console.log("   Network:", network.name);
    console.log("   Deployer:", deployer.address);
    
    // Test contract
    console.log("\n🧪 Testing contract...");
    const creditPrice = await waterBroker.creditPrice();
    console.log("   Credit Price:", ethers.formatEther(creditPrice), "ETH");
    
    // Save deployment info
    const deploymentInfo = {
      address: address,
      network: network.name,
      chainId: network.chainId,
      deployer: deployer.address,
      creditPrice: ethers.formatEther(creditPrice),
      timestamp: new Date().toISOString(),
      txHash: waterBroker.deploymentTransaction().hash
    };
    
    console.log("\n📄 Deployment Info:");
    console.log(JSON.stringify(deploymentInfo, null, 2));
    
    return deploymentInfo;
    
  } catch (error) {
    console.error("\n❌ Deployment Failed:");
    console.error("Error:", error.message);
    
    // Provide specific help based on error type
    if (error.message.includes("insufficient funds")) {
      console.log("\n💡 Solution: Get testnet ETH from:");
      console.log("   https://www.coinbase.com/faucets/base-ethereum-sepolia-faucet");
    }
    
    if (error.message.includes("could not detect network")) {
      console.log("\n💡 Solution: Check your .env file:");
      console.log("   PRIVATE_KEY should be 64 characters (no 0x prefix)");
    }
    
    if (error.message.includes("nonce")) {
      console.log("\n💡 Solution: Reset MetaMask account or wait a few minutes");
    }
    
    throw error;
  }
}

safeDeploy()
  .then(() => process.exit(0))
  .catch((error) => {
    console.error(error);
    process.exit(1);
  });