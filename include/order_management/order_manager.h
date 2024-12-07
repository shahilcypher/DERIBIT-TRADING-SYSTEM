#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "../websocket/websocket_client.h"
#include "order.h"

namespace deribit {
class OrderManager {
public:
    // Enum to represent different order types supported by Deribit
    enum class OrderType {
        LIMIT,
        MARKET,
        STOP_LIMIT,
        STOP_MARKET
    };

    // Enum to represent order directions
    enum class OrderDirection {
        BUY,
        SELL
    };

    // Enum to represent time in force options
    enum class TimeInForce {
        GOOD_TIL_CANCELLED,
        IMMEDIATE_OR_CANCEL,
        FILL_OR_KILL
    };

    OrderManager(deribit::websocket::WebSocketClient& client);

    // Place a new order
    nlohmann::json placeOrder(
        const std::string& instrumentName,
        OrderDirection direction,
        OrderType type,
        double amount,
        double price = 0.0,
        TimeInForce timeInForce = TimeInForce::GOOD_TIL_CANCELLED,
        const std::string& label = ""
    );

    // Cancel an existing order
    nlohmann::json cancelOrder(const std::string& orderId);

    // Modify an existing order
    nlohmann::json modifyOrder(
        const std::string& orderId,
        std::optional<double> newAmount = std::nullopt,
        std::optional<double> newPrice = std::nullopt
    );

    // Retrieve current open orders
    std::vector<nlohmann::json> getOpenOrders(const std::string& instrumentName = "");

    // Get current positions
    std::vector<nlohmann::json> getPositions();

private:
    deribit::websocket::WebSocketClient& m_wsClient;

    // Convert internal enums to Deribit API string representations
    std::string convertOrderTypeToString(OrderType type);
    std::string convertDirectionToString(OrderDirection direction);
    std::string convertTimeInForceToString(TimeInForce tif);

    // Internal method to send requests and handle responses
    nlohmann::json sendRequest(const nlohmann::json& request);
};
}
