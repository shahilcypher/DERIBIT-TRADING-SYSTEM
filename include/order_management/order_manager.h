#pragma once

#include "order.h"
#include <vector>
#include <string>
#include <memory>

namespace OrderManagement {

class OrderManager {
public:
    OrderManager(const std::string& accessToken) : m_accessToken(accessToken) {}

    // Place a new order
    std::string placeOrder(const Order& order);

    // Modify an existing order
    std::string modifyOrder(const std::string& orderId, 
                            const double price = -1, 
                            const double amount = -1);

    // Cancel a specific order
    std::string cancelOrder(const std::string& orderId);

    // Cancel all orders or filtered orders
    std::string cancelAllOrders(const std::string& instrument = "", 
                                 const std::string& currency = "");

    // Get open orders
    std::string getOpenOrders(const std::string& instrument = "", 
                               const std::string& currency = "",
                               const std::string& label = "");

private:
    std::string m_accessToken;
    std::vector<std::unique_ptr<Order>> m_orders;

    static const std::vector<std::string> SUPPORTED_CURRENCIES;
};

} // namespace OrderManagement
