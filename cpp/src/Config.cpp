#include "bearings/Config.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace bearings {

Config Config::fromJson(const std::string& jsonStr) {
    Config config;
    config.merge(jsonStr);
    return config;
}

void Config::merge(const std::string& jsonStr) {
    json j = json::parse(jsonStr, nullptr, false);
    if (j.is_discarded()) return;

    if (j.contains("desiredAccuracy")) desiredAccuracy = j["desiredAccuracy"].get<double>();
    if (j.contains("distanceFilter")) distanceFilter = j["distanceFilter"].get<double>();
    if (j.contains("stationaryRadius")) stationaryRadius = j["stationaryRadius"].get<double>();
    if (j.contains("stopTimeout")) stopTimeout = j["stopTimeout"].get<double>();
    if (j.contains("debug")) debug = j["debug"].get<bool>();
    if (j.contains("stopOnTerminate")) stopOnTerminate = j["stopOnTerminate"].get<bool>();
    if (j.contains("startOnBoot")) startOnBoot = j["startOnBoot"].get<bool>();
    if (j.contains("url")) url = j["url"].get<std::string>();
    if (j.contains("enabled")) enabled = j["enabled"].get<bool>();
    if (j.contains("schedule")) schedule = j["schedule"].get<std::vector<std::string>>();
    if (j.contains("scheduleUseAlarmManager")) scheduleUseAlarmManager = j["scheduleUseAlarmManager"].get<bool>();
}

std::string Config::toJson() const {
    json j = {
        {"desiredAccuracy", desiredAccuracy},
        {"distanceFilter", distanceFilter},
        {"stationaryRadius", stationaryRadius},
        {"stopTimeout", stopTimeout},
        {"debug", debug},
        {"stopOnTerminate", stopOnTerminate},
        {"startOnBoot", startOnBoot},
        {"url", url},
        {"enabled", enabled},
        {"schedule", schedule},
        {"scheduleUseAlarmManager", scheduleUseAlarmManager},
    };
    return j.dump();
}

} // namespace bearings
