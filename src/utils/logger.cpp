#include <mutex>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>

namespace deribit::utils {

class Logger {
public:
    enum class LogLevel { DEBUG, INFO, ERROR, CRITICAL };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) { m_logLevel = level; }

    void log(LogLevel level, const std::string& message) {
        if (level >= m_logLevel) {
            writeLog(levelToString(level), message);
        }
    }

private:
    Logger() : m_logLevel(LogLevel::INFO) {
        m_logFile.open("deribit_trader.log", std::ios::app);
    }
    ~Logger() {
        if (m_logFile.is_open()) {
            m_logFile.close();
        }
    }

    void writeLog(const std::string& level, const std::string& message) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        char timestamp[20];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "[" << timestamp << "] " << level << ": " << message << std::endl;
        if (m_logFile.is_open()) {
            m_logFile << "[" << timestamp << "] " << level << ": " << message << std::endl;
        }
    }

    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
        }
        return "UNKNOWN";
    }

    std::mutex m_mutex;
    std::ofstream m_logFile;
    LogLevel m_logLevel;
};

} // namespace deribit::utils
