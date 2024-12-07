#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <nlohmann/json.hpp>

namespace deribit {
namespace websocket {

class WebSocketClient {
public:
    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };

    // Constructor and Destructor
    WebSocketClient(
        const std::string& host, 
        const std::string& port, 
        const std::string& api_key,
        const std::string& secret_key
    );
    ~WebSocketClient();

    // Connection Management
    bool connect();
    void disconnect();
    ConnectionState getConnectionState() const;

    // Subscription Methods
    void subscribeToChannel(const std::string& channel);
    void unsubscribeFromChannel(const std::string& channel);

    // Message Handling
    void sendMessage(const std::string& message);
    
    // Callback Registrations
    void setOnMessageCallback(std::function<void(const std::string&)> callback);
    void setOnConnectionCallback(std::function<void()> callback);
    void setOnDisconnectionCallback(std::function<void()> callback);

private:
    // Internal Connection Management
    void runIOContext();
    void startReading();
    void handleRead(boost::beast::error_code ec, std::size_t bytes_transferred);

    // Authentication
    std::string generateSignature();
    void authenticate();

    // Member Variables
    std::string host_;
    std::string port_;
    std::string api_key_;
    std::string secret_key_;

    boost::asio::io_context io_context_;
    boost::asio::ssl::context ssl_context_;
    
    // Secure WebSocket Stream
    std::unique_ptr<boost::beast::websocket::stream<
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
    >> websocket_;

    // Tracking and State Management
    std::vector<std::string> subscribed_channels_;
    mutable std::mutex state_mutex_;
    ConnectionState connection_state_;

    // Thread Management
    std::thread io_thread_;

    // Callbacks
    std::function<void(const std::string&)> on_message_callback_;
    std::function<void()> on_connection_callback_;
    std::function<void()> on_disconnection_callback_;

    // Message Buffer
    boost::beast::multi_buffer read_buffer_;
};

} // namespace websocket
} // namespace deribit