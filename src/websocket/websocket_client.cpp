// src/websocket/websocket_client.cpp
#include "websocket/websocket_client.h"
#include <openssl/hmac.h>
#include <iostream>
#include <algorithm>

namespace deribit {
namespace websocket {

WebSocketClient::WebSocketClient(
    const std::string& host, 
    const std::string& port, 
    const std::string& api_key,
    const std::string& secret_key
) : 
    host_(host),
    port_(port),
    api_key_(api_key),
    secret_key_(secret_key),
    ssl_context_(boost::asio::ssl::context::tlsv12_client),
    connection_state_(ConnectionState::DISCONNECTED)
{
    // Configure SSL context
    ssl_context_.set_verify_mode(boost::asio::ssl::verify_peer);
    ssl_context_.set_default_verify_paths();
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

WebSocketClient::ConnectionState WebSocketClient::getConnectionState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return connection_state_;
}

bool WebSocketClient::connect() {
    try {
        // Resolve endpoint
        boost::asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host_, port_);

        // Create secure WebSocket
        websocket_ = std::make_unique<boost::beast::websocket::stream<
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket>
        >>(io_context_, ssl_context_);

        // Connect to server
        auto ep = boost::asio::connect(websocket_->next_layer().next_layer(), endpoints);
        
        // SSL Handshake
        websocket_->next_layer().handshake(boost::asio::ssl::stream_base::client);
        
        // WebSocket Handshake
        websocket_->handshake(host_ + ":" + port_, "/ws/api/v2");

        // Authenticate
        authenticate();

        // Update connection state
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            connection_state_ = ConnectionState::CONNECTED;
        }

        // Start reading messages in background
        io_thread_ = std::thread(&WebSocketClient::runIOContext, this);
        startReading();

        // Trigger connection callback if set
        if (on_connection_callback_) {
            on_connection_callback_();
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Connection Error: " << e.what() << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            connection_state_ = ConnectionState::ERROR;
        }
        
        return false;
    }
}

void WebSocketClient::authenticate() {
    // Generate authentication payload
    auto timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    // Prepare authentication request
    nlohmann::json auth_payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/auth"},
        {"id", 1},
        {"params", {
            {"grant_type", "client_signature"},
            {"client_id", api_key_},
            // Add other authentication parameters
        }}
    };

    // Send authentication message
    sendMessage(auth_payload.dump());
}

void WebSocketClient::runIOContext() {
    io_context_.run();
}

void WebSocketClient::disconnect() {
    try {
        // Ensure thread is stopped
        if (io_thread_.joinable()) {
            io_context_.stop();
            io_thread_.join();
        }

        // Close WebSocket if it's open
        if (websocket_ && websocket_->is_open()) {
            boost::beast::error_code ec;
            websocket_->close(boost::beast::websocket::close_code::normal, ec);
            
            if (ec) {
                std::cerr << "Error closing WebSocket: " << ec.message() << std::endl;
            }
        }

        // Reset state
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            connection_state_ = ConnectionState::DISCONNECTED;
        }

        // Clear subscribed channels
        subscribed_channels_.clear();

        // Trigger disconnection callback if set
        if (on_disconnection_callback_) {
            on_disconnection_callback_();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Disconnect Error: " << e.what() << std::endl;
    }
}

void WebSocketClient::startReading() {
    // Asynchronous read operation
    websocket_->async_read(
        read_buffer_,
        [this](boost::beast::error_code ec, std::size_t bytes_transferred) {
            handleRead(ec, bytes_transferred);
        }
    );
}

void WebSocketClient::handleRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        std::cerr << "Read Error: " << ec.message() << std::endl;
        
        // Update connection state
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            connection_state_ = ConnectionState::ERROR;
        }
        
        return;
    }

    // Convert buffer to string
    std::string message = boost::beast::buffers_to_string(read_buffer_.data());
    read_buffer_.consume(read_buffer_.size());

    // Invoke message callback if set
    if (on_message_callback_) {
        on_message_callback_(message);
    }

    // Continue reading
    startReading();
}

void WebSocketClient::sendMessage(const std::string& message) {
    try {
        if (!websocket_ || !websocket_->is_open()) {
            throw std::runtime_error("WebSocket is not connected");
        }

        boost::beast::error_code ec;
        websocket_->write(boost::asio::buffer(message), ec);

        if (ec) {
            std::cerr << "Send Message Error: " << ec.message() << std::endl;
            throw std::runtime_error("Failed to send message");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Send Message Exception: " << e.what() << std::endl;
        
        // Update connection state
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            connection_state_ = ConnectionState::ERROR;
        }
    }
}

void WebSocketClient::subscribeToChannel(const std::string& channel) {
    // Construct subscription request JSON
    nlohmann::json subscribe_payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/subscribe"},
        {"id", 1},
        {"params", {
            {"channels", {channel}}
        }}
    };

    // Send subscription message
    sendMessage(subscribe_payload.dump());

    // Track subscribed channels
    subscribed_channels_.push_back(channel);
}

void WebSocketClient::unsubscribeFromChannel(const std::string& channel) {
    // Construct unsubscription request JSON
    nlohmann::json unsubscribe_payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/unsubscribe"},
        {"id", 1},
        {"params", {
            {"channels", {channel}}
        }}
    };

    // Send unsubscription message
    sendMessage(unsubscribe_payload.dump());

    // Remove from tracked channels
    auto it = std::find(subscribed_channels_.begin(), subscribed_channels_.end(), channel);
    if (it != subscribed_channels_.end()) {
        subscribed_channels_.erase(it);
    }
}

void WebSocketClient::setOnMessageCallback(std::function<void(const std::string&)> callback) {
    on_message_callback_ = callback;
}

void WebSocketClient::setOnConnectionCallback(std::function<void()> callback) {
    on_connection_callback_ = callback;
}

void WebSocketClient::setOnDisconnectionCallback(std::function<void()> callback) {
    on_disconnection_callback_ = callback;
}

} // namespace websocket
} // namespace deribit