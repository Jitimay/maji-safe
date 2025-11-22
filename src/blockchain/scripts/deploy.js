// AI-generated deployment script
async function main() {
  const WaterBroker = await ethers.getContractFactory("WaterBroker");
  const waterBroker = await WaterBroker.deploy();
  
  await waterBroker.deployed();
  
  console.log("WaterBroker deployed to:", waterBroker.address);
  console.log("Network:", network.name);
  
  // Verify on Basescan if on mainnet
  if (network.name !== "hardhat") {
    console.log("Waiting for block confirmations...");
    await waterBroker.deployTransaction.wait(6);
    
    await hre.run("verify:verify", {
      address: waterBroker.address,
      constructorArguments: [],
    });
  }
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
