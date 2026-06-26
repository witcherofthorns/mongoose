#ifndef QUARKPUNK_MONGOOSE_MONGODB_HPP
#define QUARKPUNK_MONGOOSE_MONGODB_HPP

#include <mongocxx/pool.hpp>

namespace mongoose {

class mongodb {
public:
    mongodb(const mongocxx::uri& uri) : pool{uri}{ ping(); }
    mongodb(const std::string& uri_str) : pool{mongocxx::uri{uri_str}}{ ping(); }
    mongodb(const mongodb&) = delete;
    ~mongodb();
    mongodb& operator=(const mongodb&) = delete;
public:
    mongocxx::pool pool;
private:
    void ping();
};

}

#endif