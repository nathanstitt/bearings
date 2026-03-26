#include "geomony/GeofenceEventRecord.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace geomony {

std::string GeofenceEventRecord::toJson() const {
    json j = {
        {"identifier", identifier},
        {"action", action},
        {"latitude", latitude},
        {"longitude", longitude},
        {"accuracy", accuracy},
        {"timestamp", timestamp},
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

std::string GeofenceEventRecord::toJsonArray(const std::vector<GeofenceEventRecord>& events) {
    json arr = json::array();
    for (const auto& ev : events) {
        arr.push_back(json::parse(ev.toJson()));
    }
    return arr.dump();
}

} // namespace geomony
