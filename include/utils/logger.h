#pragma once

#include <string>
#include <mutex>
#include <fstream>

namespace deribit::utils {
class Logger {
public:
    // Singleton pattern - get the logger instance
    static Logger& getInstance();

    // Log an informational message
    void log(const std::string& category, const std::string& message);

    // Log an error message
    void error(const std::string& category, const std::string& message);

    // Prevent copying and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    // Private constructor
    Logger();

    // Internal method to write log
    void writeLog(const std::string& level, const std::string& category, const std::string& message);

    // Mutex for thread-safe logging
    std::mutex m_mutex;

    // Log file stream
    std::ofstream m_logFile;
};
}