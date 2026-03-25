#include "geotrack/Location.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>

using json = nlohmann::json;

namespace geotrack {

namespace {

std::string generateUUID() {
    static std::mt19937 rng(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, 15);

    const char* hex = "0123456789abcdef";
    std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    for (auto& c : uuid) {
        if (c == 'x') {
            c = hex[dist(rng)];
        } else if (c == 'y') {
            c = hex[(dist(rng) & 0x3) | 0x8];
        }
    }
    return uuid;
}

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::gmtime(&time_t));

    std::ostringstream ss;
    ss << buf << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
    return ss.str();
}

} // anonymous namespace

Location Location::fromJson(const std::string& jsonStr) {
    Location loc;
    json j = json::parse(jsonStr, nullptr, false);
    if (j.is_discarded()) return loc;

    loc.uuid = j.value("uuid", generateUUID());
    loc.timestamp = j.value("timestamp", currentTimestamp());
    loc.latitude = j.value("latitude", 0.0);
    loc.longitude = j.value("longitude", 0.0);
    loc.altitude = j.value("altitude", 0.0);
    loc.speed = j.value("speed", -1.0);
    loc.heading = j.value("heading", -1.0);
    loc.accuracy = j.value("accuracy", -1.0);
    loc.speedAccuracy = j.value("speed_accuracy", -1.0);
    loc.headingAccuracy = j.value("heading_accuracy", -1.0);
    loc.altitudeAccuracy = j.value("altitude_accuracy", -1.0);
    loc.isMoving = j.value("is_moving", false);
    loc.activityType = j.value("activity_type", std::string("unknown"));
    loc.activityConfidence = j.value("activity_confidence", 0);
    loc.event = j.value("event", std::string());
    loc.extras = j.value("extras", std::string());

    if (loc.createdAt.empty()) loc.createdAt = currentTimestamp();

    return loc;
}

std::string Location::toJson() const {
    json j = {
        {"id", id},
        {"uuid", uuid},
        {"timestamp", timestamp},
        {"latitude", latitude},
        {"longitude", longitude},
        {"altitude", altitude},
        {"speed", speed},
        {"heading", heading},
        {"accuracy", accuracy},
        {"speed_accuracy", speedAccuracy},
        {"heading_accuracy", headingAccuracy},
        {"altitude_accuracy", altitudeAccuracy},
        {"is_moving", isMoving},
        {"activity", {{"type", activityType}, {"confidence", activityConfidence}}},
        {"activity_type", activityType},
        {"activity_confidence", activityConfidence},
        {"event", event},
        {"extras", extras},
        {"synced", synced},
        {"created_at", createdAt},
    };
    return j.dump();
}

std::string Location::toJsonArray(const std::vector<Location>& locations) {
    json arr = json::array();
    for (const auto& loc : locations) {
        arr.push_back(json::parse(loc.toJson()));
    }
    return arr.dump();
}

} // namespace geotrack
