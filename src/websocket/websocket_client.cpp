#include "websocket/websocket_client.h"
#include <iostream>
#include <iomanip>
#include <sstream>

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

bool WebSocketClient::connect(bool /* auto_reconnect */) {
    try {
        // Resolve endpoint
        boost::asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host_, port_);

        // Create secure WebSocket using in-place construction
        websocket_ = std::make_unique<SecureWebSocket>(io_context_, ssl_context_);

        // Connect to server
        auto& socket = websocket_->next_layer().next_layer();
        boost::asio::connect(socket, endpoints);
        
        // SSL Handshake
        websocket_->next_layer().handshake(boost::asio::ssl::stream_base::client);
        
        // WebSocket Handshake
        websocket_->handshake(host_ + ":" + port_, "/ws/api/v2");

        // Authenticate if API key is provided
        if (!api_key_.empty() && !secret_key_.empty()) {
            authenticate();
        }

        // Update connection state
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            connection_state_ = ConnectionState::CONNECTED;
        }

        // Start reading messages in background
        io_thread_ = std::make_unique<std::thread>(&WebSocketClient::runIOContext, this);
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

void WebSocketClient::disconnect(bool /* graceful */) {
    try {
        // Ensure thread is stopped
        if (io_thread_ && io_thread_->joinable()) {
            io_context_.stop();
            io_thread_->join();
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

ConnectionState WebSocketClient::getConnectionState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return connection_state_;
}

bool WebSocketClient::isConnected() const {
    return getConnectionState() == ConnectionState::CONNECTED;
}

std::string WebSocketClient::generateSignature() {
    // Get current timestamp in milliseconds
    auto timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    // Construct the signature string
    std::string signature_data = timestamp + "\n" + api_key_ + "\n" + "client_signature";

    // Use OpenSSL HMAC to generate the signature
    unsigned char hmac_digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length;

    HMAC(EVP_sha256(), 
         secret_key_.c_str(), secret_key_.length(),
         reinterpret_cast<const unsigned char*>(signature_data.c_str()), 
         signature_data.length(),
         hmac_digest, 
         &digest_length);

    // Convert HMAC digest to hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < digest_length; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hmac_digest[i]);
    }

    return ss.str();
}

void WebSocketClient::authenticate() {
    // Generate timestamp and signature
    auto timestamp = std::to_string(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    // Generate client signature
    std::string signature = generateSignature();

    // Prepare authentication payload
    nlohmann::json auth_payload = {
        {"jsonrpc", "2.0"},
        {"method", "public/auth"},
        {"id", 1},
        {"params", {
            {"grant_type", "client_signature"},
            {"client_id", api_key_},
            {"timestamp", timestamp},
            {"signature", signature},
            {"nonce", boost::uuids::to_string(boost::uuids::random_generator()())},
            {"data", ""}
        }}
    };

    // Send authentication message
    std::string auth_message = auth_payload.dump();
    std::cout << "Authentication Request: " << auth_message << std::endl;
    
    try {
        std::string response = sendMessage(auth_message);
        std::cout << "Authentication Response: " << response << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Authentication Error: " << e.what() << std::endl;
        throw;
    }
}

void WebSocketClient::runIOContext() {
    io_context_.run();
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

void WebSocketClient::handleRead(boost::beast::error_code ec, std::size_t /* bytes_transferred */) {
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

std::string WebSocketClient::sendMessage(const std::string& message) {
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

        // Placeholder: For now, return a success message
        return R"({"result": "success", "id": 1})";
    }
    catch (const std::exception& e) {
        std::cerr << "Send Message Exception: " << e.what() << std::endl;
        throw;
    }
}

void WebSocketClient::subscribeToChannels(const std::vector<std::string>& channels) {
    for (const auto& channel : channels) {
        subscribeToChannel(channel);
    }
}

void WebSocketClient::unsubscribeFromChannels(const std::vector<std::string>& channels) {
    for (const auto& channel : channels) {
        unsubscribeFromChannel(channel);
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

std::vector<std::string> WebSocketClient::getSubscribedChannels() const {
    return subscribed_channels_;
}

void WebSocketClient::setOnMessageCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_message_callback_ = callback;
}

void WebSocketClient::setOnConnectionCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_connection_callback_ = callback;
}

void WebSocketClient::setOnDisconnectionCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_disconnection_callback_ = callback;
}

} // namespace websocket
} // namespace deribit