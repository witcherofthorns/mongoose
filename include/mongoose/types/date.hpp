#ifndef QUARKPUNK_MONGOOSE_TYPES_DATE_HPP
#define QUARKPUNK_MONGOOSE_TYPES_DATE_HPP

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace mongoose {
    using time_point = std::chrono::system_clock::time_point;
    using date = std::chrono::system_clock::time_point;
}

namespace mongoose::types::date {
    // get now time
    inline time_point now() {
        return time_point::clock::now();
    }

    // to string (ISO 8601)
    inline std::string to_string(const time_point& timepoint) noexcept {
        auto time = std::chrono::system_clock::to_time_t(timepoint);
        std::tm tm = *std::gmtime(&time);
        
        auto since_epoch = timepoint.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch - seconds);
        
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << milliseconds.count() << "Z";
        return oss.str();
    }

    // from string (ISO 8601)
    // if failed parsed: throw runtime_error
    inline time_point from_string(const std::string& iso_str) {
        std::tm tm = {};
        int milliseconds = 0;
        std::istringstream iss(iso_str);

        char delimiter;
        iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (iss.fail()) {
            throw std::runtime_error("failed parse date string: " + iso_str);
        }

        // try parse milliseconds
        if (iss.peek() == '.') {
            iss >> delimiter;
            iss >> milliseconds;
            // if there are less than 3 digits, fill zeros
            if (milliseconds < 10) milliseconds *= 100;
            else if (milliseconds < 100) milliseconds *= 10;
        }

        #ifdef _WIN32
            auto time = _mkgmtime(&tm);
        #else
            auto time = timegm(&tm);
        #endif

        auto timepoint = std::chrono::system_clock::from_time_t(time);

        // add milliseconds
        timepoint += std::chrono::milliseconds(milliseconds);
        return timepoint;
    }

    // to timestamp
    inline int64_t to_timestamp(const time_point& timepoint) noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            timepoint.time_since_epoch()
        ).count();
    }

    // from timestamp type milliseconds
    inline time_point from_timestamp_ms(int64_t milliseconds) noexcept {
        auto secs = std::chrono::seconds(milliseconds / 1000);
        auto ms = std::chrono::milliseconds(milliseconds % 1000);
        return time_point{secs + ms};
    }

    // from timestamp type seconds
    inline time_point from_timestamp_sec(int64_t seconds) noexcept {
        return std::chrono::system_clock::from_time_t(
            static_cast<std::time_t>(seconds)
        );
    }

    // from timestamp
    inline time_point from_timestamp(int64_t timestamp) noexcept {
        if (timestamp > 1'000'000'000'000LL) {
            return from_timestamp_ms(timestamp);
        }
        return from_timestamp_sec(timestamp);
    }

    inline void add_days(time_point& time, int days) {
        time += std::chrono::hours(24 * days);
    }

    inline void add_hours(time_point& time, int hours) {
        time += std::chrono::hours(hours);
    }

    inline void add_minutes(time_point& time, int minutes) {
        time += std::chrono::minutes(minutes);
    }

    inline void add_seconds(time_point& time, int seconds) {
        time += std::chrono::seconds(seconds);
    }

    inline void sub_days(time_point& time, int days) {
        time -= std::chrono::hours(24 * days);
    }

    inline void sub_hours(time_point& time, int hours) {
        time -= std::chrono::hours(hours);
    }

    inline void sub_minutes(time_point& time, int minutes) {
        time -= std::chrono::minutes(minutes);
    }

    inline void sub_seconds(time_point& time, int seconds) {
        time -= std::chrono::seconds(seconds);
    }
}

// NLOHMANN_JSON type adapter
#ifdef MONGOOSE_USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>

namespace nlohmann {

    template <>
    struct adl_serializer<mongoose::time_point> {
        static void to_json(json& j, const mongoose::time_point& value) {
            j = mongoose::types::date::to_string(value);
        }
        static void from_json(const json& j, mongoose::time_point& value) {
            try {
                if (j.is_string()) {
                    value = mongoose::types::date::from_string(j.get<std::string>());
                    return;
                }
                if (j.is_number()) {
                    int64_t timestamp = j.get<int64_t>();
                    value = mongoose::types::date::from_timestamp(timestamp);
                    return;
                }
            }
            catch (const std::exception& e) {}

            // fallback to default value
            value = mongoose::time_point{};
        }
    };

}

#endif // MONGOOSE_USE_NLOHMANN_JSON
#endif // QUARKPUNK_MONGOOSE_TYPES_DATE_HPP