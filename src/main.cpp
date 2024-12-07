#include "websocket/websocket_client.h"
#include "order_management/order_manager.h"
#include "utils/config_loader.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

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

        // Set up message callback
        client.setOnMessageCallback([](const std::string& message) {
            std::cout << "Received message: " << message << std::endl;
        });

        // Attempt to connect
        if (client.connect()) {
            std::cout << "Successfully connected to Deribit!" << std::endl;

            // Create OrderManager
            deribit::OrderManager orderManager(client);

            // Demonstrate order placement
            try {
                // Place a buy limit order
                auto orderResponse = orderManager.placeOrder(
                    "ADA_USDC-PERPETUAL",  // Instrument name
                    deribit::OrderManager::OrderDirection::BUY,
                    deribit::OrderManager::OrderType::LIMIT,
                    100,  // Amount
                    0.50,  // Price
                    deribit::OrderManager::TimeInForce::GOOD_TIL_CANCELLED,
                    "Test ADA Buy Order"
                );

                // Check order response structure
                if (orderResponse.contains("result") && orderResponse["result"].contains("order")) {
                    auto order = orderResponse["result"]["order"];
                    std::cout << "Order placed successfully:\n"
                              << orderResponse.dump(4) << std::endl;

                    // Optional: Display order details
                    std::cout << "Order ID: " << order["order_id"] << std::endl;
                    std::cout << "Order State: " << order["order_state"] << std::endl;

                    // Retrieve and display open orders
                    auto openOrdersResponse = orderManager.getOpenOrders("ADA_USDC-PERPETUAL");

                    // Iterate through the vector of JSON objects
                    if (!openOrdersResponse.empty()) {
                        for (const auto& openOrder : openOrdersResponse) {
                            std::cout << "Open Order: " << openOrder.dump(4) << std::endl;
                        }
                    } else {
                        std::cerr << "No open orders found." << std::endl;
                    }
                } else {
                    std::cerr << "Unexpected order response structure:\n"
                              << orderResponse.dump(4) << std::endl;
                }
            }
            catch (const std::exception& orderEx) {
                std::cerr << "Order Error: " << orderEx.what() << std::endl;
            }
        } 
        else {
            std::cerr << "Failed to connect to Deribit" << std::endl;
            return 1;
        }

        // Keep application running to process WebSocket messages
        std::this_thread::sleep_for(std::chrono::minutes(1));

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
