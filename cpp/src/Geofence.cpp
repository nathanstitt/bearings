#include "geomony/Geofence.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace geomony {

Geofence Geofence::fromJson(const std::string& jsonStr) {
    Geofence g;
    json j = json::parse(jsonStr, nullptr, false);
    if (j.is_discarded()) return g;

    g.identifier = j.value("identifier", std::string());
    g.latitude = j.value("latitude", 0.0);
    g.longitude = j.value("longitude", 0.0);
    g.radius = j.value("radius", 150.0);
    g.notifyOnEntry = j.value("notifyOnEntry", true);
    g.notifyOnExit = j.value("notifyOnExit", true);
    g.notifyOnDwell = j.value("notifyOnDwell", false);
    g.loiteringDelay = j.value("loiteringDelay", 30000);
    if (j.contains("extras")) {
        if (j["extras"].is_object() || j["extras"].is_array()) {
            g.extras = j["extras"].dump();
        } else if (j["extras"].is_string()) {
            g.extras = j["extras"].get<std::string>();
        }
    }

    return g;
}

std::string Geofence::toJson() const {
    json j = {
        {"identifier", identifier},
        {"latitude", latitude},
        {"longitude", longitude},
        {"radius", radius},
        {"notifyOnEntry", notifyOnEntry},
        {"notifyOnExit", notifyOnExit},
        {"notifyOnDwell", notifyOnDwell},
        {"loiteringDelay", loiteringDelay},
    };
    if (!extras.empty()) {
        auto parsed = json::parse(extras, nullptr, false);
        if (!parsed.is_discarded()) {
            j["extras"] = parsed;
        } else {
            j["extras"] = extras;
        }
    }
    return j.dump();
}

std::string Geofence::toJsonArray(const std::vector<Geofence>& geofences) {
    json arr = json::array();
    for (const auto& g : geofences) {
        arr.push_back(json::parse(g.toJson()));
    }
    return arr.dump();
}

} // namespace geomony
