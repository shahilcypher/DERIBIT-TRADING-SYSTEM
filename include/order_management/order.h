#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include <chrono>

namespace deribit {
namespace order {

class Order {
public:
    // Enum for order types
    enum class Type {
        LIMIT,
        MARKET,
        STOP_LIMIT,
        STOP_MARKET
    };

    // Enum for order directions
    enum class Direction {
        BUY,
        SELL
    };

    // Enum for order states
    enum class State {
        OPEN,
        FILLED,
        CANCELLED,
        REJECTED,
        PARTIALLY_FILLED
    };

    // Constructor with default arguments
    Order(
        const std::string& instrument_name,
        Type type,
        Direction direction,
        double amount,
        double price = 0.0,
        const std::string& label = ""
    );

    // Factory method to create an order from JSON response
    static Order fromJson(const nlohmann::json& orderJson);

    // Getters
    std::string getOrderId() const;
    std::string getInstrumentName() const;
    Type getType() const;
    Direction getDirection() const;
    State getState() const;
    double getAmount() const;
    double getPrice() const;
    double getFilledAmount() const;
    std::string getLabel() const;
    std::chrono::system_clock::time_point getCreationTime() const;

    // Update order state from API response
    void updateFromJson(const nlohmann::json& orderJson);

    // Convert enums to strings (useful for logging/API)
    static std::string typeToString(Type type);
    static std::string directionToString(Direction direction);
    static std::string stateToString(State state);

private:
    std::string m_orderId;
    std::string m_instrumentName;
    Type m_type;
    Direction m_direction;
    State m_state;
    double m_amount;
    double m_price;
    double m_filledAmount;
    std::string m_label;
    std::chrono::system_clock::time_point m_creationTime;
};

} // namespace order
} // namespace deribit
