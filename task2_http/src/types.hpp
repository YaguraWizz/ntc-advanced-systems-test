#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cmath>
#include <nlohmann/json.hpp>

namespace types {

    enum class CoordFormat {
        UNKNOWN,
        DECIMAL,        // e.g. 12.2112 -32.434
        HEMI_DECIMAL,   // e.g. N12.2112 W32.434
        DEG_MIN,        // e.g. 51°12.32'
        DEG_MIN_SEC,    // e.g. 51°12'32.212''
        COMPACT,        // e.g. N405229 E087182 or 5401N 15531W
        GOOGLE_STYLE,   // 55,755831°, 37,617673°
        OTHER
    };

    enum class CoordSetType {
        SINGLE_POINT,
        LINE,
        CLOSED_POLYGON
    };

    struct Coordinate {
        double lat = 0.0;
        double lon = 0.0;
        bool valid = false;
        CoordFormat format = CoordFormat::UNKNOWN;
      
        // Helper: compare two coords with epsilon
        bool equals_eps(const Coordinate& other, double eps = 1e-6) const {
            return std::fabs(lat - other.lat) <= eps && std::fabs(lon - other.lon) <= eps;
        }

        // Валидация координат
        static inline bool check_lat_lon_valid(double lat, double lon) {
            return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
        }
    };

    struct CoordinateMetadata {
        std::string raw_match;            // matched substring
        std::string sentence_snippet;     // <= 200 chars
        std::optional<std::string> name;  // optional label if detected
        std::vector<std::string> errors;  // validation errors or warnings
	};


    struct CoordinateSet {
        std::vector<std::pair<Coordinate, CoordinateMetadata>> coords;
        CoordSetType set_type = CoordSetType::SINGLE_POINT;
    };



    // nlohmann serialization
    inline void to_json(nlohmann::json& j, const Coordinate& c) {
        j = nlohmann::json{
         {"lat", c.lat},
         {"lon", c.lon},
         {"valid", c.valid},
         {"format", static_cast<int>(c.format)}
        };
    }
    inline void to_json(nlohmann::json& j, const CoordinateMetadata& m) {
        j = nlohmann::json{
            {"raw_match", m.raw_match},
            {"sentence_snippet", m.sentence_snippet}
        };
        if (m.name) j["name"] = *m.name;
        if (!m.errors.empty()) j["errors"] = m.errors;
    }
    inline void to_json(nlohmann::json& j, const CoordinateSet& s) {
        nlohmann::json coord_array = nlohmann::json::array();
        for (const auto& [cor, meta] : s.coords) {
            nlohmann::json full_coord_obj;
            to_json(full_coord_obj, cor);
            to_json(full_coord_obj, meta);
            coord_array.push_back(full_coord_obj);
        }
        j["coords"] = coord_array;
        j["set_type"] = static_cast<int>(s.set_type);
    }
} // namespace domain

