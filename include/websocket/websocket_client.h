#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <nlohmann/json.hpp>

#include <openssl/hmac.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace deribit {
namespace websocket {

class WebSocketClient {
public:
    // Enum for connection states with more granular status tracking
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        RECONNECTING,
        ERROR
    };

    // Enhanced constructor with more configuration options
    WebSocketClient(
        const std::string& host = "test.deribit.com", 
        const std::string& port = "443", 
        const std::string& api_key = "",
        const std::string& secret_key = ""
    );
    ~WebSocketClient();

    // Connection Management
    bool connect(bool auto_reconnect = false);
    void disconnect(bool graceful = true);
    ConnectionState getConnectionState() const;
    bool isConnected() const;

    // Subscription Management
    void subscribeToChannels(const std::vector<std::string>& channels);
    void unsubscribeFromChannels(const std::vector<std::string>& channels);
    std::vector<std::string> getSubscribedChannels() const;

    // Message Handling
    std::string sendMessage(const std::string& message);
    void subscribeToChannel(const std::string& channel);
    void unsubscribeFromChannel(const std::string& channel);

    // Callback Registrations
    void setOnMessageCallback(std::function<void(const std::string&)> callback);
    void setOnConnectionCallback(std::function<void()> callback);
    void setOnDisconnectionCallback(std::function<void()> callback);

private:
    // Authentication and Security
    std::string generateSignature();
    void authenticate();

    // Internal Connection Management
    void runIOContext();
    void startReading();
    void handleRead(boost::beast::error_code ec, std::size_t bytes_transferred);

    // Configuration and Connection Parameters
    std::string host_;
    std::string port_;
    std::string api_key_;
    std::string secret_key_;

    // Networking Components
    boost::asio::io_context io_context_;
    boost::asio::ssl::context ssl_context_;
    
    // Secure WebSocket Stream Type
    using SecureWebSocket = boost::beast::websocket::stream<
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>;

    std::unique_ptr<SecureWebSocket> websocket_;

    // State and Synchronization
    std::atomic<ConnectionState> connection_state_{ConnectionState::DISCONNECTED};
    mutable std::mutex state_mutex_;
    mutable std::mutex callback_mutex_;

    // Channel and Subscription Management
    std::vector<std::string> subscribed_channels_;

    // Thread Management
    std::unique_ptr<std::thread> io_thread_;

    // Callback Handlers
    std::function<void(const std::string&)> on_message_callback_;
    std::function<void()> on_connection_callback_;
    std::function<void()> on_disconnection_callback_;

    // Message Buffers
    boost::beast::multi_buffer read_buffer_;
};

} // namespace websocket
} // namespace deribit