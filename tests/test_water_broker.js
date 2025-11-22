// AI-generated tests for WaterBroker contract
const { expect } = require("chai");
const { ethers } = require("hardhat");

describe("WaterBroker", function () {
  let waterBroker;
  let owner, user1;

  beforeEach(async function () {
    [owner, user1] = await ethers.getSigners();
    const WaterBroker = await ethers.getContractFactory("WaterBroker");
    waterBroker = await WaterBroker.deploy();
  });

  it("Should allow water purchase", async function () {
    const pumpId = ethers.utils.formatBytes32String("PUMP001");
    const payment = ethers.utils.parseEther("0.001");
    
    await expect(waterBroker.connect(user1).buyWater(pumpId, { value: payment }))
      .to.emit(waterBroker, "WaterPurchased")
      .withArgs(user1.address, 1, pumpId);
    
    expect(await waterBroker.waterCredits(user1.address)).to.equal(1);
  });

  it("Should activate pump", async function () {
    const pumpId = ethers.utils.formatBytes32String("PUMP001");
    
    await expect(waterBroker.activatePump(pumpId, 10))
      .to.emit(waterBroker, "PumpActivated")
      .withArgs(pumpId, 10);
  });
});
