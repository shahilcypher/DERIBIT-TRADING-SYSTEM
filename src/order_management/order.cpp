#include "order_management/order.h"
#include <stdexcept>
#include <unordered_map>

namespace deribit {
namespace order {

Order::Order(
    const std::string& instrument_name,
    Type type,
    Direction direction,
    double amount,
    double price,
    const std::string& label
) : 
    m_instrumentName(instrument_name),
    m_type(type),
    m_direction(direction),
    m_state(State::OPEN),       // m_state first
    m_amount(amount),           // m_amount second
    m_filledAmount(0.0),        // m_filledAmount third
    m_label(label),             // m_label fourth
    m_price(price),             // m_price fifth
    m_creationTime(std::chrono::system_clock::now()) 
{
}

Order Order::fromJson(const nlohmann::json& orderJson) {
    if (!orderJson.contains("instrument_name") || 
        !orderJson.contains("order_type") || 
        !orderJson.contains("direction") || 
        !orderJson.contains("amount")) {
        throw std::invalid_argument("Incomplete order JSON");
    }

    // Map Deribit API order types to internal enum
    static const std::unordered_map<std::string, Type> typeMap = {
        {"limit", Type::LIMIT},
        {"market", Type::MARKET},
        {"stop_limit", Type::STOP_LIMIT},
        {"stop_market", Type::STOP_MARKET}
    };

    // Map Deribit API directions to internal enum
    static const std::unordered_map<std::string, Direction> directionMap = {
        {"buy", Direction::BUY},
        {"sell", Direction::SELL}
    };

    // Map Deribit API states to internal enum
    static const std::unordered_map<std::string, State> stateMap = {
        {"open", State::OPEN},
        {"filled", State::FILLED},
        {"cancelled", State::CANCELLED},
        {"rejected", State::REJECTED},
        {"partially_filled", State::PARTIALLY_FILLED}
    };

    Order order(
        orderJson.value("instrument_name", ""),
        typeMap.at(orderJson.value("order_type", "limit")),
        directionMap.at(orderJson.value("direction", "buy")),
        orderJson.value("amount", 0.0),
        orderJson.value("price", 0.0),
        orderJson.value("label", "")
    );

    order.updateFromJson(orderJson);
    return order;
}

void Order::updateFromJson(const nlohmann::json& orderJson) {
    // Update order details from JSON response
    if (orderJson.contains("order_id")) {
        m_orderId = orderJson["order_id"].get<std::string>();
    }

    if (orderJson.contains("order_state")) {
        static const std::unordered_map<std::string, State> stateMap = {
            {"open", State::OPEN},
            {"filled", State::FILLED},
            {"cancelled", State::CANCELLED},
            {"rejected", State::REJECTED},
            {"partially_filled", State::PARTIALLY_FILLED}
        };

        m_state = stateMap.at(orderJson["order_state"].get<std::string>());
    }

    if (orderJson.contains("filled_amount")) {
        m_filledAmount = orderJson["filled_amount"].get<double>();
    }
}

// Getters
std::string Order::getOrderId() const { return m_orderId; }
std::string Order::getInstrumentName() const { return m_instrumentName; }
Order::Type Order::getType() const { return m_type; }
Order::Direction Order::getDirection() const { return m_direction; }
Order::State Order::getState() const { return m_state; }
double Order::getAmount() const { return m_amount; }
double Order::getPrice() const { return m_price; }
double Order::getFilledAmount() const { return m_filledAmount; }
std::string Order::getLabel() const { return m_label; }
std::chrono::system_clock::time_point Order::getCreationTime() const { return m_creationTime; }

// Static conversion methods
std::string Order::typeToString(Type type) {
    switch(type) {
        case Type::LIMIT: return "limit";
        case Type::MARKET: return "market";
        case Type::STOP_LIMIT: return "stop_limit";
        case Type::STOP_MARKET: return "stop_market";
        default: throw std::invalid_argument("Unknown order type");
    }
}

std::string Order::directionToString(Direction direction) {
    return (direction == Direction::BUY) ? "buy" : "sell";
}

std::string Order::stateToString(State state) {
    switch(state) {
        case State::OPEN: return "open";
        case State::FILLED: return "filled";
        case State::CANCELLED: return "cancelled";
        case State::REJECTED: return "rejected";
        case State::PARTIALLY_FILLED: return "partially_filled";
        default: throw std::invalid_argument("Unknown order state");
    }
}

} // namespace order
} // namespace deribit