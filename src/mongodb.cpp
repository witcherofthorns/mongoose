#include <mongoose/mongodb.hpp>
#include <mongoose/logger.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/document.hpp>
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using namespace mongoose;

// global mongocxx instance required for driver
static mongocxx::instance _instance;

// that class destructor is mostly needed to display
// debug message about the release of class
// mostly to track the life cycle of class
// and understand that everything is fine
mongodb::~mongodb() {
    logger::log(logger::INFO, "mongodb close");
}

// internal connection ping function
void mongodb::ping() {
    try {
        auto client = pool.acquire();
        auto admin_db = (*client)["admin"];
        auto ping_cmd = document{} << "ping" << 1 << finalize;
        auto result = admin_db.run_command(ping_cmd.view());
        logger::log(logger::INFO, "mongodb connected, ping success");
    }
    catch(const std::exception& e) {
        logger::log(logger::ERROR, "mongodb %s", e.what());
    }
}