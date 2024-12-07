#include "order_management/order_manager.h"
#include "utils/logger.h"
#include "websocket/websocket_client.h"

namespace deribit {

OrderManager::OrderManager(websocket::WebSocketClient& client) : m_wsClient(client) {}

nlohmann::json OrderManager::placeOrder(
    const std::string& instrumentName,
    OrderDirection direction,
    OrderType type,
    double amount,
    double price,
    TimeInForce timeInForce,
    const std::string& label
) {
    nlohmann::json request = {
        {"method", "private/buy"},  // Will be dynamically changed based on direction
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"params", {
            {"instrument_name", instrumentName},
            {"amount", amount},
            {"type", convertOrderTypeToString(type)},
            {"time_in_force", convertTimeInForceToString(timeInForce)}
        }}
    };

    // Set method based on direction
    request["method"] = (direction == OrderDirection::BUY) 
        ? "private/buy" 
        : "private/sell";

    // Add price for limit orders
    if (type == OrderType::LIMIT) {
        request["params"]["price"] = price;
    }

    // Add optional label
    if (!label.empty()) {
        request["params"]["label"] = label;
    }

    try {
        auto response = sendRequest(request);
        utils::Logger::getInstance().log(
            "Order Placed", 
            "Instrument: " + instrumentName + 
            ", Direction: " + convertDirectionToString(direction)
        );
        return response;
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Order Placement Failed", e.what());
        throw;
    }
}

nlohmann::json OrderManager::cancelOrder(const std::string& orderId) {
    nlohmann::json request = {
        {"method", "private/cancel"},
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"params", {
            {"order_id", orderId}
        }}
    };

    try {
        auto response = sendRequest(request);
        utils::Logger::getInstance().log("Order Cancelled", "Order ID: " + orderId);
        return response;
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Order Cancellation Failed", e.what());
        throw;
    }
}

nlohmann::json OrderManager::modifyOrder(
    const std::string& orderId,
    std::optional<double> newAmount,
    std::optional<double> newPrice
) {
    nlohmann::json request = {
        {"method", "private/edit"},
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"params", {
            {"order_id", orderId}
        }}
    };

    if (newAmount) {
        request["params"]["amount"] = *newAmount;
    }

    if (newPrice) {
        request["params"]["price"] = *newPrice;
    }

    try {
        auto response = sendRequest(request);
        utils::Logger::getInstance().log("Order Modified", "Order ID: " + orderId);
        return response;
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Order Modification Failed", e.what());
        throw;
    }
}

std::vector<nlohmann::json> OrderManager::getOpenOrders(const std::string& instrumentName) {
    nlohmann::json request = {
        {"method", "private/get_open_orders"},
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"params", {}}
    };

    // Optional instrument filtering
    if (!instrumentName.empty()) {
        request["params"]["instrument_name"] = instrumentName;
    }

    try {
        auto response = sendRequest(request);
        return response.value("result", nlohmann::json::array());
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Fetching Open Orders Failed", e.what());
        throw;
    }
}

std::vector<nlohmann::json> OrderManager::getPositions() {
    nlohmann::json request = {
        {"method", "private/get_positions"},
        {"jsonrpc", "2.0"},
        {"id", 1}
    };

    try {
        auto response = sendRequest(request);
        return response.value("result", nlohmann::json::array());
    } catch (const std::exception& e) {
        utils::Logger::getInstance().error("Fetching Positions Failed", e.what());
        throw;
    }
}

nlohmann::json OrderManager::sendRequest(const nlohmann::json& request) {
    // Send via WebSocket client and await response
    std::string responseStr = m_wsClient.sendMessage(request.dump());
    
    nlohmann::json parsedResponse = nlohmann::json::parse(responseStr);
    
    // Check for errors in the response
    if (parsedResponse.contains("error")) {
        throw std::runtime_error("API Error: " + parsedResponse["error"].dump());
    }

    return parsedResponse;
}

// Conversion methods
std::string OrderManager::convertOrderTypeToString(OrderType type) {
    switch(type) {
        case OrderType::LIMIT: return "limit";
        case OrderType::MARKET: return "market";
        case OrderType::STOP_LIMIT: return "stop_limit";
        case OrderType::STOP_MARKET: return "stop_market";
        default: throw std::invalid_argument("Invalid order type");
    }
}

std::string OrderManager::convertDirectionToString(OrderDirection direction) {
    return (direction == OrderDirection::BUY) ? "buy" : "sell";
}

std::string OrderManager::convertTimeInForceToString(TimeInForce tif) {
    switch(tif) {
        case TimeInForce::GOOD_TIL_CANCELLED: return "good_til_cancelled";
        case TimeInForce::IMMEDIATE_OR_CANCEL: return "immediate_or_cancel";
        case TimeInForce::FILL_OR_KILL: return "fill_or_kill";
        default: throw std::invalid_argument("Invalid time in force");
    }
}

} // namespace deribit