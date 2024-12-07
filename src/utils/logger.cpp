#include "utils/logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>

namespace deribit::utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Open log file in append mode
    m_logFile.open("deribit_trader.log", std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

void Logger::log(const std::string& category, const std::string& message) {
    writeLog("INFO", category, message);
}

void Logger::error(const std::string& category, const std::string& message) {
    writeLog("ERROR", category, message);
}

void Logger::writeLog(const std::string& level, const std::string& category, const std::string& message) {
    // Get current time
    auto now = std::time(nullptr);
    auto tm = std::localtime(&now);

    // Create timestamp
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    // Prepare log message
    std::string logMessage = "[" + std::string(timestamp) + "] " + 
                             level + " [" + category + "]: " + message;

    // Thread-safe logging
    std::lock_guard<std::mutex> lock(m_mutex);

    // Write to console
    std::cout << logMessage << std::endl;

    // Write to file if open
    if (m_logFile.is_open()) {
        m_logFile << logMessage << std::endl;
        m_logFile.flush();
    }
}

} // namespace deribit::utils