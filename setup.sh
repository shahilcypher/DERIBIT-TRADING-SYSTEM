#!/bin/bash
# setup.sh

# Install dependencies
sudo apt-get update
sudo apt-get install -y \
  libboost-all-dev \
  libssl-dev \
  nlohmann-json3-dev \
  cmake \
  g++

# Create build directory
mkdir -p build
cd build

# Configure and build project
cmake ..
make