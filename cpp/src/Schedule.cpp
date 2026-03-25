#include "bearings/Schedule.h"
#include "bearings/Logger.h"

#include <algorithm>
#include <sstream>

namespace bearings {

namespace {

// Parse a comma-separated list of day specifiers (single digits or ranges like "2-6")
std::vector<int> parseDays(const std::string& dayStr) {
    std::vector<int> days;
    std::string token;
    std::istringstream stream(dayStr);

    while (std::getline(stream, token, ',')) {
        auto dash = token.find('-');
        if (dash != std::string::npos) {
            int from = std::stoi(token.substr(0, dash));
            int to = std::stoi(token.substr(dash + 1));
            for (int d = from; d <= to; d++) {
                days.push_back(d);
            }
        } else {
            days.push_back(std::stoi(token));
        }
    }
    return days;
}

// Parse "HH:mm" into hour and minute
bool parseTime(const std::string& timeStr, int& hour, int& minute) {
    auto colon = timeStr.find(':');
    if (colon == std::string::npos) return false;
    try {
        hour = std::stoi(timeStr.substr(0, colon));
        minute = std::stoi(timeStr.substr(colon + 1));
        return true;
    } catch (...) {
        return false;
    }
}

// Convert time-of-day to seconds since midnight
int timeToSeconds(int hour, int minute, int second = 0) {
    return hour * 3600 + minute * 60 + second;
}

} // anonymous namespace

ScheduleRule ScheduleRule::parse(const std::string& ruleStr) {
    ScheduleRule rule;

    // Split on space to get day part and time part
    auto spacePos = ruleStr.find(' ');
    if (spacePos == std::string::npos) {
        Logger::instance().warning("Invalid schedule rule (no space): " + ruleStr);
        return rule;
    }

    std::string dayPart = ruleStr.substr(0, spacePos);
    std::string timePart = ruleStr.substr(spacePos + 1);

    // Check for literal date (contains '-' in a date-like pattern YYYY-MM-DD)
    if (dayPart.size() >= 10 && dayPart[4] == '-' && dayPart[7] == '-') {
        // Check for date-span format (two dates separated by space)
        // e.g. "2018-01-01-09:00 2019-01-01-17:00" — not supported
        if (timePart.find('-') != std::string::npos && timePart.find(':') != std::string::npos
            && timePart.find('-') < timePart.find(':')) {
            Logger::instance().warning("Date-span schedule format not supported, skipping: " + ruleStr);
            return rule;
        }

        rule.isLiteralDate = true;
        try {
            rule.year = std::stoi(dayPart.substr(0, 4));
            rule.month = std::stoi(dayPart.substr(5, 2));
            rule.day = std::stoi(dayPart.substr(8, 2));
        } catch (...) {
            Logger::instance().warning("Failed to parse literal date: " + dayPart);
            return rule;
        }
    } else {
        // Day-of-week format
        try {
            rule.days = parseDays(dayPart);
        } catch (...) {
            Logger::instance().warning("Failed to parse days: " + dayPart);
            return rule;
        }
    }

    // Parse time range "HH:mm-HH:mm"
    auto dashPos = timePart.find('-');
    if (dashPos == std::string::npos) {
        Logger::instance().warning("Invalid time range (no dash): " + timePart);
        return rule;
    }

    std::string startTime = timePart.substr(0, dashPos);
    std::string endTime = timePart.substr(dashPos + 1);

    if (!parseTime(startTime, rule.startHour, rule.startMinute)) {
        Logger::instance().warning("Failed to parse start time: " + startTime);
        return rule;
    }
    if (!parseTime(endTime, rule.endHour, rule.endMinute)) {
        Logger::instance().warning("Failed to parse end time: " + endTime);
        return rule;
    }

    return rule;
}

void Schedule::setRules(const std::vector<std::string>& rules) {
    rules_.clear();
    for (const auto& ruleStr : rules) {
        ScheduleRule rule = ScheduleRule::parse(ruleStr);
        // Only add valid rules (must have days or be literal date, and valid time)
        if (!rule.days.empty() || rule.isLiteralDate) {
            rules_.push_back(rule);
        }
    }
    Logger::instance().info("Schedule: loaded " + std::to_string(rules_.size()) + " rules");
}

bool Schedule::hasRules() const {
    return !rules_.empty();
}

EvalResult Schedule::evaluate(int year, int month, int dayOfMonth, int dayOfWeek,
                              int hour, int minute, int second) const {
    EvalResult result{false, -1};

    if (rules_.empty()) {
        return result;
    }

    int nowSeconds = timeToSeconds(hour, minute, second);
    int bestSecondsUntilTransition = -1;

    for (const auto& rule : rules_) {
        bool dayMatches = false;

        if (rule.isLiteralDate) {
            dayMatches = (rule.year == year && rule.month == month && rule.day == dayOfMonth);
        } else {
            for (int d : rule.days) {
                if (d == dayOfWeek) {
                    dayMatches = true;
                    break;
                }
            }
        }

        int startSec = timeToSeconds(rule.startHour, rule.startMinute);
        int endSec = timeToSeconds(rule.endHour, rule.endMinute);
        bool crossesMidnight = (endSec <= startSec);

        if (dayMatches) {
            bool inWindow = false;
            int secondsUntilEnd = 0;

            if (crossesMidnight) {
                // Active when time >= start OR time < end
                if (nowSeconds >= startSec) {
                    inWindow = true;
                    // Seconds until midnight + end time
                    secondsUntilEnd = (86400 - nowSeconds) + endSec;
                } else if (nowSeconds < endSec) {
                    inWindow = true;
                    secondsUntilEnd = endSec - nowSeconds;
                }
            } else {
                // Normal window: start <= time < end
                if (nowSeconds >= startSec && nowSeconds < endSec) {
                    inWindow = true;
                    secondsUntilEnd = endSec - nowSeconds;
                }
            }

            if (inWindow) {
                result.shouldTrack = true;
                // Track the nearest end-of-window
                if (bestSecondsUntilTransition < 0 || secondsUntilEnd < bestSecondsUntilTransition) {
                    bestSecondsUntilTransition = secondsUntilEnd;
                }
            }
        }

        // If not currently tracking, compute seconds until next window opens
        if (!result.shouldTrack) {
            int secondsUntilStart = -1;

            if (dayMatches) {
                if (crossesMidnight) {
                    if (nowSeconds < startSec && nowSeconds >= endSec) {
                        secondsUntilStart = startSec - nowSeconds;
                    }
                } else {
                    if (nowSeconds < startSec) {
                        secondsUntilStart = startSec - nowSeconds;
                    }
                }
            }

            // Check if tomorrow (or a future day) matches
            if (secondsUntilStart < 0 && !rule.isLiteralDate) {
                // Find the next matching day
                for (int offset = 1; offset <= 7; offset++) {
                    int futureDay = ((dayOfWeek - 1 + offset) % 7) + 1;
                    for (int d : rule.days) {
                        if (d == futureDay) {
                            // Seconds until midnight, then full days, then start time
                            secondsUntilStart = (86400 - nowSeconds) + (offset - 1) * 86400 + startSec;
                            break;
                        }
                    }
                    if (secondsUntilStart >= 0) break;
                }
            }

            if (secondsUntilStart >= 0) {
                if (bestSecondsUntilTransition < 0 || secondsUntilStart < bestSecondsUntilTransition) {
                    bestSecondsUntilTransition = secondsUntilStart;
                }
            }
        }
    }

    // If we are tracking, bestSecondsUntilTransition = time until tracking should stop
    // If not tracking, bestSecondsUntilTransition = time until tracking should start
    result.secondsUntilNextTransition = bestSecondsUntilTransition;

    return result;
}

} // namespace bearings
