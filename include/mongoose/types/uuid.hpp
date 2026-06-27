#ifndef QUARKPUNK_MONGOOSE_TYPES_UUID_HPP
#define QUARKPUNK_MONGOOSE_TYPES_UUID_HPP

#include <string>
#include <stdexcept>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/time_generator_v7.hpp>
#include <boost/uuid/string_generator.hpp>
#include <iostream>

namespace mongoose {
    using uuid = boost::uuids::uuid;
}

namespace mongoose::types::uuid {
    
    // generate uuid v4
    inline mongoose::uuid generate_v4() {
        boost::uuids::random_generator generator;
        return generator();
    }

    // generate uuid v7
    inline mongoose::uuid generate_v7() {
        boost::uuids::time_generator_v7 generator;
        return generator();
    }

    // from string to uuid
    inline mongoose::uuid from_string(const std::string& str) {
        try {
            boost::uuids::string_generator gen;
            boost::uuids::uuid uuid_new = gen(str);

            if (uuid_new.version() == boost::uuids::uuid::version_unknown) {
                throw std::runtime_error("uuid bad string format: " + str);
            }

            return uuid_new;
        } 
        catch (const std::exception& e) {
            throw std::runtime_error("uuid failed parse string: " + str);
        }
    }

    // to string from uuid
    inline std::string to_string(const mongoose::uuid& value) {
        return boost::uuids::to_string(value);
    }

    // validation
    inline bool is_valid(const std::string& str) noexcept {
        try {
            boost::uuids::string_generator gen;
            auto uuid = gen(str);
            return !uuid.is_nil() && uuid.version() != boost::uuids::uuid::version_unknown;
        } catch (...) {
            return false;
        }
    }

    // validation
    inline bool is_valid(const mongoose::uuid& uuid) noexcept {
        return !uuid.is_nil();
    }
}

// NLOHMANN_JSON type adapter
#ifdef MONGOOSE_USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>

namespace nlohmann {
    template <>
    struct adl_serializer<mongoose::uuid> {
        static void to_json(json& j, const mongoose::uuid& value) {
            j = mongoose::types::uuid::to_string(value);
        }
        static void from_json(const json& j, mongoose::uuid& value) {
            try {
                if (j.is_string()) {
                    value = mongoose::types::uuid::from_string(j.get<std::string>());
                    return;
                }
            }
            catch (const std::exception&) {
                throw std::runtime_error("json failed parse uuid");
            }
        }
    };

}

#endif // MONGOOSE_USE_NLOHMANN_JSON
#endif // QUARKPUNK_MONGOOSE_TYPES_UUID_HPP