#pragma once

#include <string>
#include <vector>

namespace bearings {

struct ScheduleRule {
    std::vector<int> days;           // 1=Sun..7=Sat (empty for literal dates)
    int startHour = 0, startMinute = 0;
    int endHour = 0, endMinute = 0;
    bool isLiteralDate = false;
    int year = 0, month = 0, day = 0;

    static ScheduleRule parse(const std::string& ruleStr);
};

struct EvalResult {
    bool shouldTrack;
    int secondsUntilNextTransition;  // -1 if none
};

class Schedule {
public:
    void setRules(const std::vector<std::string>& rules);
    bool hasRules() const;
    EvalResult evaluate(int year, int month, int dayOfMonth, int dayOfWeek,
                        int hour, int minute, int second) const;

private:
    std::vector<ScheduleRule> rules_;
};

} // namespace bearings
