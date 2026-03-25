#include "bearings/Logger.h"

namespace bearings {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setHandler(LogHandler handler) {
    handler_ = std::move(handler);
}

void Logger::setMinLevel(LogLevel level) {
    minLevel_ = level;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < minLevel_) return;
    if (handler_) {
        handler_(level, message);
    }
}

} // namespace bearings
