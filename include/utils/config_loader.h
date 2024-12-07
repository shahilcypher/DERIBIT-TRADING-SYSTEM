// include/utils/config_loader.h
#pragma once

#include <string>
#include <stdexcept>
#include <cstdlib>

namespace deribit {
namespace config {

class ConfigLoader {
public:
    static std::string getClientId() {
        const char* client_id = std::getenv("DERIBIT_CLIENT_ID");
        if (!client_id) {
            throw std::runtime_error("DERIBIT_CLIENT_ID environment variable not set");
        }
        return std::string(client_id);
    }

    static std::string getClientSecret() {
        const char* client_secret = std::getenv("DERIBIT_CLIENT_SECRET");
        if (!client_secret) {
            throw std::runtime_error("DERIBIT_CLIENT_SECRET environment variable not set");
        }
        return std::string(client_secret);
    }

    // Optional: Add method to check if credentials are valid
    static bool areCredentialsSet() {
        return std::getenv("DERIBIT_CLIENT_ID") != nullptr && 
               std::getenv("DERIBIT_CLIENT_SECRET") != nullptr;
    }
};

} // namespace config
} // namespace deribit