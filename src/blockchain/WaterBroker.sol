// SPDX-License-Identifier: MIT
pragma solidity ^0.8.19;

contract WaterBroker {
    mapping(address => uint256) public waterCredits;
    mapping(bytes32 => bool) public pumpActivations;
    
    address public owner;
    uint256 public creditPrice = 0.001 ether; // 1 credit = 0.001 ETH
    
    event WaterPurchased(address indexed user, uint256 credits, bytes32 pumpId);
    event PumpActivated(bytes32 indexed pumpId, uint256 liters);
    
    constructor() {
        owner = msg.sender;
    }
    
    function buyWater(bytes32 pumpId) external payable {
        require(msg.value >= creditPrice, "Insufficient payment");
        uint256 credits = msg.value / creditPrice;
        waterCredits[msg.sender] += credits;
        emit WaterPurchased(msg.sender, credits, pumpId);
    }
    
    function activatePump(bytes32 pumpId, uint256 liters) external {
        require(msg.sender == owner, "Only owner");
        pumpActivations[pumpId] = true;
        emit PumpActivated(pumpId, liters);
    }
}
