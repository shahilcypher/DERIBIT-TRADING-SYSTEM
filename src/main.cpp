#include "websocket/websocket_client.h"
#include "order_management/order_manager.h"
#include "market_data/market_data_handler.h"
#include "utils/config_loader.h"
#include "utils/logger.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using namespace OrderManagement;
using namespace deribit;

int main() {
    try {
        // Initialize logging
        utils::Logger::getInstance().setLogLevel(utils::LogLevel::INFO);
        utils::Logger::getInstance().log(utils::LogLevel::INFO, "Application started");

        // Load credentials securely
        std::string client_id = config::ConfigLoader::getClientId();
        std::string client_secret = config::ConfigLoader::getClientSecret();

        // Initialize WebSocket Client
        websocket::WebSocketClient client(
            "test.deribit.com",  // Host
            "443",               // Port
            client_id,           // API Key
            client_secret        // Secret Key
        );

        // Market Data Handler
        market_data::MarketDataHandler marketDataHandler(client);

        // Set up message callback with logging and performance tracking
        client.setOnMessageCallback([&](const std::string& message) {
            auto start = std::chrono::high_resolution_clock::now();
            
            utils::Logger::getInstance().log(utils::LogLevel::DEBUG, 
                "Received WebSocket message: " + message);
            
            // Process market data
            marketDataHandler.processMessage(message);
        });

        // Attempt to connect
        if (client.connect()) {
            utils::Logger::getInstance().log(utils::LogLevel::INFO, 
                "Successfully connected to Deribit!");

            // Create OrderManager
            OrderManager orderManager(client_id);

            // Subscribe to market data channels
            std::vector<std::string> channels = {
                "trades.BTC-PERPETUAL.raw",
                "ticker.BTC-PERPETUAL.raw",
                "book.BTC-PERPETUAL.raw"
            };
            marketDataHandler.subscribeToChannels(channels);

            // Demonstrate comprehensive trading workflow
            try {
                // Create a complex limit order
                Order buyOrder(
                    "BTC-PERPETUAL",  // Instrument
                    "buy",             // Side
                    0.01,               // Amount
                    Order::stringToOrderType("LIMIT"),  // Order Type
                    Order::stringToTimeInForce("GOOD_TIL_CANCELLED"),  // Time in Force
                    "Test BTC Buy Order", // Label
                    50000.0             // Price
                );

                // Place the order
                auto orderResponse = orderManager.placeOrder(buyOrder);
                
                // Check and log order response
                if (orderResponse.contains("result") && 
                    orderResponse["result"].contains("order")) {
                    auto order = orderResponse["result"]["order"];
                    utils::Logger::getInstance().log(utils::LogLevel::INFO, 
                        "Order placed successfully: " + order.dump(4));
                    
                    // Retrieve open orders
                    auto openOrders = orderManager.getOpenOrders("BTC-PERPETUAL");
                    
                    // Log open orders
                    for (const auto& openOrder : openOrders) {
                        utils::Logger::getInstance().log(utils::LogLevel::INFO, 
                            "Open Order: " + openOrder.dump(4));
                    }
                }

                // Demonstrate order modification and cancellation
                auto openOrdersResponse = orderManager.getOpenOrders();
                if (!openOrdersResponse.empty()) {
                    auto firstOrderId = openOrdersResponse[0]["order_id"].get<std::string>();
                    
                    // Modify order
                    auto modifyResponse = orderManager.modifyOrder(
                        firstOrderId, 
                        51000.0,  // New price 
                        0.02      // New amount
                    );
                    
                    // Cancel order
                    auto cancelResponse = orderManager.cancelOrder(firstOrderId);
                }
            }
            catch (const std::exception& orderEx) {
                utils::Logger::getInstance().log(utils::LogLevel::ERROR, 
                    std::string("Order Error: ") + orderEx.what());
            }

            // Keep application running to process WebSocket messages
            std::this_thread::sleep_for(std::chrono::minutes(5));
        } 
        else {
            utils::Logger::getInstance().log(utils::LogLevel::ERROR, 
                "Failed to connect to Deribit");
            return 1;
        }

    } 
    catch (const std::exception& e) {
        utils::Logger::getInstance().log(utils::LogLevel::CRITICAL, 
            std::string("Unhandled Exception: ") + e.what());
        return 1;
    }

    return 0;
}