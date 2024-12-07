// src/main.cpp
#include "websocket/websocket_client.h"
#include "utils/config_loader.h"
#include <iostream>

int main() {
    try {
        // Load credentials securely
        std::string client_id = deribit::config::ConfigLoader::getClientId();
        std::string client_secret = deribit::config::ConfigLoader::getClientSecret();

        // Initialize WebSocket Client
        deribit::websocket::WebSocketClient client(
            "test.deribit.com",  // Host
            "443",               // Port
            client_id,           // API Key
            client_secret        // Secret Key
        );

        // Attempt to connect
        if (client.connect()) {
            std::cout << "Successfully connected to Deribit!" << std::endl;
            
            // Subscribe to sample channels
            client.subscribeToChannel("trades.BTC-PERPETUAL.raw");
            client.subscribeToChannel("book.BTC-PERPETUAL.none.20.100ms");
        } else {
            std::cerr << "Failed to connect to Deribit" << std::endl;
            return 1;
        }

        // Keep application running (in a real app, you'd have proper event loop)
        std::this_thread::sleep_for(std::chrono::minutes(10));

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}