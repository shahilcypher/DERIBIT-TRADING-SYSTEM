# Make setup script executable
chmod +x setup.sh

# Run setup script
./setup.sh

# Build project
cmake -B build
cmake --build build

# Run the application
./build/deribit_trader