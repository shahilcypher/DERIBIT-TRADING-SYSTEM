#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

namespace OrderManagement {

enum class OrderType {
    LIMIT,
    STOP_LIMIT,
    TAKE_LIMIT,
    MARKET,
    STOP_MARKET,
    TAKE_MARKET,
    MARKET_LIMIT,
    TRAILING_STOP
};

enum class TimeInForce {
    GOOD_TIL_CANCELLED,
    GOOD_TIL_DAY,
    FILL_OR_KILL,
    IMMEDIATE_OR_CANCEL
};

class Order {
public:
    Order() = default;
    Order(const std::string& instrument, const std::string& side, 
          double amount, OrderType type, TimeInForce tif, 
          const std::string& label = "", double price = 0.0);

    std::string getInstrument() const { return m_instrument; }
    std::string getSide() const { return m_side; }
    double getAmount() const { return m_amount; }
    OrderType getType() const { return m_type; }
    TimeInForce getTimeInForce() const { return m_timeInForce; }
    std::string getLabel() const { return m_label; }
    double getPrice() const { return m_price; }
    std::string getOrderId() const { return m_orderId; }
    void setOrderId(const std::string& orderId) { m_orderId = orderId; }

    json toJson(const std::string& accessToken) const;
    static OrderType stringToOrderType(const std::string& typeStr);
    static TimeInForce stringToTimeInForce(const std::string& tifStr);

private:
    std::string m_instrument;
    std::string m_side;
    double m_amount;
    OrderType m_type;
    TimeInForce m_timeInForce;
    std::string m_label;
    double m_price;
    std::string m_orderId;
};

} // namespace OrderManagement
