#include "order_management/order.h"
#include <stdexcept>

namespace OrderManagement {

Order::Order(const std::string& instrument, const std::string& side, 
             double amount, OrderType type, TimeInForce tif, 
             const std::string& label, double price)
    : m_instrument(instrument), 
      m_side(side), 
      m_amount(amount), 
      m_type(type), 
      m_timeInForce(tif), 
      m_label(label), 
      m_price(price) {}

std::string Order::getInstrument() const { return m_instrument; }
std::string Order::getSide() const { return m_side; }
double Order::getAmount() const { return m_amount; }
OrderType Order::getType() const { return m_type; }
TimeInForce Order::getTimeInForce() const { return m_timeInForce; }
std::string Order::getLabel() const { return m_label; }
double Order::getPrice() const { return m_price; }
std::string Order::getOrderId() const { return m_orderId; }
void Order::setOrderId(const std::string& orderId) { m_orderId = orderId; }

json Order::toJson(const std::string& accessToken) const {
    json j;
    j["instrument_name"] = m_instrument;
    j["access_token"] = accessToken;
    j["amount"] = m_amount;
    j["type"] = (m_side == "buy") ? "private/buy" : "private/sell";
    j["label"] = m_label;

    switch(m_timeInForce) {
        case TimeInForce::GOOD_TIL_CANCELLED:
            j["time_in_force"] = "good_til_cancelled";
            break;
        case TimeInForce::GOOD_TIL_DAY:
            j["time_in_force"] = "good_til_day";
            break;
        case TimeInForce::FILL_OR_KILL:
            j["time_in_force"] = "fill_or_kill";
            break;
        case TimeInForce::IMMEDIATE_OR_CANCEL:
            j["time_in_force"] = "immediate_or_cancel";
            break;
    }

    if (m_price > 0) {
        j["price"] = m_price;
    }

    return j;
}

OrderType Order::stringToOrderType(const std::string& typeStr) {
    if (typeStr == "limit") return OrderType::LIMIT;
    if (typeStr == "stop_limit") return OrderType::STOP_LIMIT;
    if (typeStr == "take_limit") return OrderType::TAKE_LIMIT;
    if (typeStr == "market") return OrderType::MARKET;
    if (typeStr == "stop_market") return OrderType::STOP_MARKET;
    if (typeStr == "take_market") return OrderType::TAKE_MARKET;
    if (typeStr == "market_limit") return OrderType::MARKET_LIMIT;
    if (typeStr == "trailing_stop") return OrderType::TRAILING_STOP;
    
    throw std::invalid_argument("Invalid order type: " + typeStr);
}

TimeInForce Order::stringToTimeInForce(const std::string& tifStr) {
    if (tifStr == "good_til_cancelled") return TimeInForce::GOOD_TIL_CANCELLED;
    if (tifStr == "good_til_day") return TimeInForce::GOOD_TIL_DAY;
    if (tifStr == "fill_or_kill") return TimeInForce::FILL_OR_KILL;
    if (tifStr == "immediate_or_cancel") return TimeInForce::IMMEDIATE_OR_CANCEL;
    
    throw std::invalid_argument("Invalid time in force: " + tifStr);
}

} // namespace OrderManagement