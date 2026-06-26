# Mongoose (MongoDB C++)
Simple and efficient C++ serializer/deserializer for BSON documents intended for MongoDB. The library implements work with (almost) all the basic types necessary for working with BSON and JSON structures

✅ Convenient API
- Easy to use
- Models/Schemas

📦 Dependencies
- C++ 20
- CMake 3.10
- mongocxx 4.0.0
- boost 1.90.0 (from mongocxx)

## Types Serialization from JSON and BSON
These are the main data types this library can work with, specifically parsing and converting data between C++ and the BSON data format. Unfortunately, mongoose doesn't have its own JSON module for converting native BSON types into JSON string

| C++ Types     | BSON | JSON |  About          |
|---------------|------|------|-----------------|
| int 32/64     | ✅   | ✅   | numbers         |
| bool          | ✅   | ✅   | boolean         |
| float         | ✅   | ✅   | num float       |
| double        | ✅   | ✅   | num double      |
| string        | ✅   | ✅   | basic string    |
| array         | ✅   | ✅   | fixed array     |
| vector        | ✅   | ✅   | dynamic vector  |
| enum class    | ✅   | ✅   | enum class type |
| optional      | ✅   | ✅   | value or null   |
| struct        | ✅   | ✅   | nested object   |
| date          | ✅   | ✅   | time point value |
| object id     | ✅   | ✅   | bson object id  |
| uuid          | ✅   | ✅   | binary uuid  types |
| decimal       | ...  | ...  | num 128 bit     |
| binary        | ...  | ...  | binary data     |
| variant       | ...  | ...  | variant value   |

## How to use
Download the library in any way and place it in your project directory, include it in your CMake project
```cmake
# use cmake FetchContent
# or use library local
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/mongoose)

# link library
target_link_libraries(${PROJECT_NAME} PRIVATE
    # dont link mongo::mongocxx static/shared
    # mongoose alredy link mongocxx lib
    mongoose
)
```

## Model/Schema
Models are created quite simply, it is enough to declare structures in the usual C++ form, and that's all

```cpp
// include mongoose types
#include <mongoose/types/oid.hpp>
#include <mongoose/types/date.hpp>

// nested document
struct user_info {
    int age;
    std::string name;
    std::string city;
    std::vector<mongoose::oid> tags;
};

// main user document
struct user {
    std::optional<mongoose::oid> _id;
    std::optional<user_info> info;
    mongoose::date created_at;
};
```
To serialize your data into a BSON document, you need to
```cpp
// build BSON document value from struct
auto doc = mongoose::to_bson(user_value);
```
```cpp
// build BSON document without _id field
// if you need to insert/update the entire document
auto doc = mongoose::to_bson_exclude(user_value, "_id");
auto result = collection.insert_one(doc.view());
```
If you need a projection from your struct
```cpp
// build BSON project document
auto doc = mongoose::to_projection<product_cart>();

// set options projection
mongocxx::options::find options;
options.projection(doc.view());
```

## JSON
Mongoose supports the `nlohmann_json` library by default and adapts all types specifically for it, as it remains quite popular and flexible, but without being tightly coupled to it. If you need a custom implementation of types for another JSON library, you can do it yourself

To deserialize from JSON to BSON, for example, to retrieve from your frontend, use the `mongoose::from_json<T>(string)` method. The result will be an optional value `std::optional<T>`. If the JSON string is invalid, a `null` result will be returned

```cpp
// parse to <T> model from JSON string
// return value if success parsing
// and cast BSON value to <T> 
std::optional<user> value = mongoose::from_json(json_string);

// check optional
// failed or null value
if(!value){
    return;
}
```

To serialize from your structures to JSON, you can use `mongoose::to_json<T>` to quickly export to a JSON string
```cpp
// return valid json string
std::string json = mongoose::to_json(model);
```

You can also use the `nlohmann_json` macro for convenient ADL serialization of your structures
```cpp
// nlohmann special macro
// macro format (Type, ...)
#ifndef JSON_MODEL
#define JSON_MODEL NLOHMANN_DEFINE_TYPE_INTRUSIVE
#endif

// your model struct
struct user {
    std::optional<mongoose::oid> _id;
    std::optional<user_info> info;
    mongoose::date created_at;
    JSON_MODEL(user, _id, info, created_at);
};
```

## Concept
In fact, I was tired of constantly writing the same code, so I wanted to template it and make it more accessible for writing my backend web applications without problems, I was inspired by the mongoose library in javascript and decided to write my own small version