#!/bin/bash
# Install DKG Edge Node (Required for hackathon)
npm install -g @origintrail/dkg
dkg node create --network=otp:20430
dkg node start
