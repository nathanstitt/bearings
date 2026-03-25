#pragma once

#include <string>
#include <functional>

namespace geotrack {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    using LogHandler = std::function<void(LogLevel level, const std::string& message)>;

    static Logger& instance();

    void setHandler(LogHandler handler);
    void setMinLevel(LogLevel level);

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

private:
    Logger() = default;
    void log(LogLevel level, const std::string& message);

    LogHandler handler_;
    LogLevel minLevel_ = LogLevel::Info;
};

} // namespace geotrack
