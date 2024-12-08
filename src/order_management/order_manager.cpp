#include "order_management/order_manager.h"
#include "nlohmann/json.hpp"

namespace OrderManagement {

const std::vector<std::string> OrderManager::SUPPORTED_CURRENCIES = {
    "BTC", "ETH", "SOL", "XRP", "MATIC",
    "USDC", "USDT", "JPY", "CAD", "AUD", "GBP", 
    "EUR", "USD", "CHF", "BRL", "MXN", "COP", 
    "CLP", "PEN", "ECS", "ARS"
};

OrderManager::OrderManager(const std::string& accessToken) 
    : m_accessToken(accessToken) {}

std::string OrderManager::placeOrder(const Order& order) {
    json j = order.toJson(m_accessToken);
    j["jsonrpc"] = "2.0";
    j["method"] = (order.getSide() == "buy") ? "private/buy" : "private/sell";
    
    // Generate a random ID for the request
    srand(time(NULL));
    j["id"] = rand();

    // Store the order
    m_orders.push_back(std::make_unique<Order>(order));

    return j.dump();
}

std::string OrderManager::modifyOrder(const std::string& orderId, 
                                      const double price, 
                                      const double amount) {
    json j;
    j["jsonrpc"] = "2.0";
    j["method"] = "private/edit";
    j["id"] = rand();
    
    j["params"] = {{"order_id", orderId}};
    
    if (amount >= 0) j["params"]["amount"] = amount;
    if (price >= 0) j["params"]["price"] = price;

    return j.dump();
}

std::string OrderManager::cancelOrder(const std::string& orderId) {
    json j;
    j["jsonrpc"] = "2.0";
    j["method"] = "private/cancel";
    j["id"] = rand();
    
    j["params"]["order_id"] = orderId;

    return j.dump();
}

std::string OrderManager::cancelAllOrders(const std::string& instrument, 
                                          const std::string& currency) {
    json j;
    j["jsonrpc"] = "2.0";
    j["id"] = rand();

    // Determine the appropriate method based on provided filters
    if (instrument.empty() && currency.empty()) {
        j["method"] = "private/cancel_all";
    } 
    else if (!instrument.empty()) {
        j["method"] = "private/cancel_all_by_instrument";
        j["params"]["instrument"] = instrument;
    } 
    else if (!currency.empty()) {
        // Validate currency
        if (std::find(SUPPORTED_CURRENCIES.begin(), 
                      SUPPORTED_CURRENCIES.end(), 
                      currency) == SUPPORTED_CURRENCIES.end()) {
            throw std::invalid_argument("Unsupported currency: " + currency);
        }
        j["method"] = "private/cancel_all_by_currency";
        j["params"]["currency"] = currency;
    }

    return j.dump();
}

std::string OrderManager::getOpenOrders(const std::string& instrument, 
                                        const std::string& currency,
                                        const std::string& label) {
    json j;
    j["jsonrpc"] = "2.0";
    j["id"] = rand();

    // Determine the appropriate method based on provided filters
    if (instrument.empty() && currency.empty() && label.empty()) {
        j["method"] = "private/get_open_orders";
    } 
    else if (!instrument.empty()) {
        j["method"] = "private/get_open_orders_by_instrument";
        j["params"]["instrument"] = instrument;
    } 
    else if (!currency.empty()) {
        // Validate currency
        if (std::find(SUPPORTED_CURRENCIES.begin(), 
                      SUPPORTED_CURRENCIES.end(), 
                      currency) == SUPPORTED_CURRENCIES.end()) {
            throw std::invalid_argument("Unsupported currency: " + currency);
        }
        
        if (label.empty()) {
            j["method"] = "private/get_open_orders_by_currency";
            j["params"]["currency"] = currency;
        } else {
            j["method"] = "private/get_open_orders_by_label";
            j["params"]["currency"] = currency;
            j["params"]["label"] = label;
        }
    }

    return j.dump();
}

} // namespace OrderManagement