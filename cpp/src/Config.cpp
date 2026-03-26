#include "geomony/Config.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace geomony {

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
    if (j.contains("syncThreshold")) syncThreshold = j["syncThreshold"].get<int>();
    if (j.contains("maxBatchSize")) maxBatchSize = j["maxBatchSize"].get<int>();
    if (j.contains("syncRetryBaseSeconds")) syncRetryBaseSeconds = j["syncRetryBaseSeconds"].get<int>();
    if (j.contains("headers") && j["headers"].is_object()) {
        headers.clear();
        for (auto& [key, val] : j["headers"].items()) {
            if (val.is_string()) headers[key] = val.get<std::string>();
        }
    }
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
        {"syncThreshold", syncThreshold},
        {"maxBatchSize", maxBatchSize},
        {"syncRetryBaseSeconds", syncRetryBaseSeconds},
        {"headers", headers},
        {"enabled", enabled},
        {"schedule", schedule},
        {"scheduleUseAlarmManager", scheduleUseAlarmManager},
    };
    return j.dump();
}

} // namespace geomony
