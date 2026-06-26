#ifndef QUARKPUNK_MONGOOSE_BSON_HPP
#define QUARKPUNK_MONGOOSE_BSON_HPP

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <chrono>
#include <boost/pfr.hpp>
#include <boost/uuid/uuid.hpp>

// fix for bsoncxx ADL search impl trouble shooting
// compiler finds this stub and the linker is happy
// if you have problems, include this header file
// as the very first header file in your main .cpp file
// please, dont edit this and no touch
namespace bsoncxx::v_noabi::document {
    class view;

    template<typename T>
    void from_bson_please_ignore_me(T&, const view&){}
}

#define from_bson from_bson_please_ignore_me
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/json.hpp>
#undef from_bson

// namespace mongoose
// forward decl main functions
namespace mongoose {
    template<typename T>
    bsoncxx::document::value to_bson(const T& obj);

    template<typename T>
    T from_bson(const bsoncxx::document::view& doc);
}

// namespace mongoose details
// internal detail specific functions
namespace mongoose::details {
    template<typename... Args>
    constexpr bool exclude_contains(std::string_view value, Args... args) {
        return ((value == args) || ...);
    }
}

// namespace mongoose utils
// internal utils functions
namespace mongoose::utils {
    inline bsoncxx::types::b_date to_bson_date(const std::chrono::system_clock::time_point& tp) {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()
        );
        return bsoncxx::types::b_date{ms};
    }

    inline std::chrono::system_clock::time_point from_bson_date(int64_t date) {
        return std::chrono::system_clock::time_point{
            std::chrono::milliseconds{date}
        };
    }

    inline bsoncxx::types::b_binary to_bson_uuid(const boost::uuids::uuid& value){
        return bsoncxx::types::b_binary{
            bsoncxx::binary_sub_type::k_uuid, 
            static_cast<std::uint32_t>(value.size()), 
            value.data()
        };
    }

    inline boost::uuids::uuid from_bson_uuid(const bsoncxx::types::b_binary& value) {
        if (value.sub_type != bsoncxx::binary_sub_type::k_uuid){
            throw std::runtime_error(
                std::string("field binary not valid for uuid type")
            );
        }

        if (value.size != 16) {
            throw std::runtime_error("field binary size is not valid for uuid");
        }

        boost::uuids::uuid uuid_new;
        std::memcpy(uuid_new.data, value.bytes, 16);
        return uuid_new;
    }
}

// namespace mongoose traits
// main traits templates and concepts
namespace mongoose::traits {

// helper template for static_assert in constexpr if
template<typename T>
struct always_false : std::false_type {};

template<typename T>
constexpr bool always_false_v = always_false<T>::value;

// empty template - does nothing by default
template<typename T, typename = void>
struct is_custom_serializable : std::false_type {};

// check for a primitive BSON type
template<typename T>
concept PrimitiveBsonType = 
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, int32_t> ||
    std::is_same_v<T, int64_t> ||
    std::is_same_v<T, float> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, bool> ||
    std::is_same_v<T, bsoncxx::oid>;

// checking for BSON Date (time_point)
template<typename T>
concept BsonDateType = 
    std::is_same_v<T, std::chrono::system_clock::time_point>;

// checking for BSON uuid
template<typename T>
concept BsonUuidType =
    std::is_same_v<T, boost::uuids::uuid>;

// check for vector
template<typename T>
concept VectorType = 
    std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>>;

// checking for a fixed size array
template<typename T>
concept ArrayType = 
    std::is_bounded_array_v<typename std::remove_reference_t<T>::type>;

// check for optional
template<typename T>
concept OptionalType = 
    std::is_same_v<T, std::optional<typename T::value_type>>;

// check for enum (including enum class)
template<typename T>
concept EnumType = std::is_enum_v<T>;

// checking if a type is an aggregate structure
template<typename T>
concept AggregateStruct = 
    std::is_aggregate_v<T> && 
    !PrimitiveBsonType<T> && 
    !BsonDateType<T> &&
    !BsonUuidType<T> &&
    !VectorType<T> && 
    !ArrayType<T> && 
    !OptionalType<T>;

template<typename B, typename T>
void serialize_field_impl(B& builder, std::string_view name, const T& value);

template<typename T>
void serialize_array_element(bsoncxx::builder::stream::array& array_builder, const T& item);

template<PrimitiveBsonType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value);

template<EnumType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value);

template<OptionalType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& opt);

template<BsonDateType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value);

template<BsonUuidType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value);

template<VectorType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& vec);

template<ArrayType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& arr);

template<AggregateStruct T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value);

template<typename T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name);

template<typename T>
T extract_array_element(const bsoncxx::array::element& element);

template<typename T>
void serialize_dispatch(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value);

template<typename T>
T extract_dispatch(const bsoncxx::document::view& doc, std::string_view name);

template<typename T>
bsoncxx::document::value serialize_custom(const T& value) {
    static_assert(always_false_v<T>, "serialize_custom not implemented for this type");
    return bsoncxx::builder::stream::document{} << bsoncxx::builder::stream::finalize;
}

template<typename T>
T deserialize_custom(const bsoncxx::document::view& doc, std::string_view name) {
    static_assert(always_false_v<T>, "deserialize_custom not implemented for this type");
    return T{};
}

template<PrimitiveBsonType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value) {
    builder << name << value;
}

template<EnumType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value) {
    builder << name << static_cast<int32_t>(value);
}

template<OptionalType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& opt) {
    if(opt.has_value()){
        serialize_dispatch(builder, name, opt.value());
    } else {
        builder << name << bsoncxx::types::b_null{};
    }
}

template<BsonDateType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value) {
    builder << name << mongoose::utils::to_bson_date(value);
}

template<BsonUuidType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value) {
    builder << name << mongoose::utils::to_bson_uuid(value);
}

template<VectorType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& vec) {
    auto array_builder = bsoncxx::builder::stream::array{};
    for(const auto& item : vec){
        serialize_array_element(array_builder, item);
    }
    builder << name << array_builder;
}

template<ArrayType T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& arr) {
    auto array_builder = bsoncxx::builder::stream::array{};
    for(const auto& item : arr){
        serialize_array_element(array_builder, item);
    }
    builder << name << array_builder;
}

template<AggregateStruct T>
void serialize_field_impl(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value) {
    auto sub_doc = bsoncxx::builder::stream::document{};
    
    boost::pfr::for_each_field(value, [&sub_doc](const auto& field, auto index) {
        constexpr std::string_view field_name = boost::pfr::get_name<index(), std::decay_t<decltype(value)>>();
        if constexpr (is_custom_serializable<std::decay_t<decltype(field)>>::value) {
            auto custom_doc = serialize_custom(field);
            sub_doc << field_name << custom_doc.view();
        }
        else {
            serialize_dispatch(sub_doc, field_name, field);
        }
    });
    
    builder << name << sub_doc;
}

template<typename T>
void serialize_array_element(bsoncxx::builder::stream::array& array_builder, const T& item) {
    if constexpr (PrimitiveBsonType<T>) {
        array_builder << item;
    }
    else if constexpr (BsonDateType<T>) {
        array_builder << mongoose::utils::to_bson_date(item);
    }
    else if constexpr (is_custom_serializable<T>::value) {
        auto custom_doc = serialize_custom(item);
        array_builder << custom_doc.view();
    }
    else if constexpr (AggregateStruct<T>) {
        auto sub_doc = bsoncxx::builder::stream::document{};
        boost::pfr::for_each_field(item, [&sub_doc](const auto& field, auto index) {
            constexpr std::string_view field_name = boost::pfr::get_name<index(), std::decay_t<decltype(item)>>();
            serialize_dispatch(sub_doc, field_name, field);
        });
        array_builder << sub_doc;
    }
    else if constexpr (VectorType<T> || ArrayType<T>) {
        auto nested_array = bsoncxx::builder::stream::array{};
        for (const auto& nested_item : item) {
            serialize_array_element(nested_array, nested_item);
        }
        array_builder << nested_array;
    }
    else {
        static_assert(always_false_v<T>, "bson unsupported type in array");
    }
}

template<PrimitiveBsonType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if (!element) {
        throw std::runtime_error(std::string("bson field not found: ") + std::string(name));
    }
    if constexpr (std::is_same_v<T, std::string>) {
        if(element.type() == bsoncxx::type::k_oid){
            return std::string(element.get_oid().value.to_string());
        }
        return std::string(element.get_string().value);
    }
    else if constexpr (std::is_same_v<T, int32_t>) {
        return element.get_int32();
    }
    else if constexpr (std::is_same_v<T, int64_t>) {
        return element.get_int64();
    }
    else if constexpr (std::is_same_v<T, float>) {
        return static_cast<float>(element.get_double());
    }
    else if constexpr (std::is_same_v<T, double>) {
        return element.get_double();
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return element.get_bool();
    }
    else if constexpr (std::is_same_v<T, bsoncxx::oid>) {
        if(element.type() == bsoncxx::type::k_string){
            return bsoncxx::oid{element.get_string().value};
        }
        return element.get_oid().value;
    }
    else {
        static_assert(always_false_v<T>, "bson unsupported primitive type");
    }
}

template<EnumType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if (!element) {
        throw std::runtime_error(
            std::string("bson field not found: ") + std::string(name)
        );
    }
    int32_t value = element.get_int32();
    return static_cast<T>(value);
}

template<BsonDateType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if (!element) {
        throw std::runtime_error(
            std::string("bson field not found: ") + std::string(name)
        );
    }
    if (element.type() == bsoncxx::type::k_date) {
        return mongoose::utils::from_bson_date(element.get_date().value.count());
    }
    if (element.type() == bsoncxx::type::k_int64) {
        return mongoose::utils::from_bson_date(element.get_int64().value);
    }
    throw std::runtime_error(
        std::string("bson bad date format: ") + std::string(name)
    );
}

template<BsonUuidType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if (!element) {
        throw std::runtime_error(
            std::string("bson field not found: ") + std::string(name)
        );
    }

    if (element.type() != bsoncxx::type::k_binary){
        throw std::runtime_error(
            std::string("bson field must be binary: ") + std::string(name)
        );
    }

    return mongoose::utils::from_bson_uuid(element.get_binary());
}


template<VectorType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if (!element) {
        return T{};
    }
    
    auto array = element.get_array().value;
    T result;
    
    for (const auto& item : array) {
        using ItemType = typename T::value_type;
        result.push_back(extract_array_element<ItemType>(item));
    }
    
    return result;
}

template<ArrayType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if(!element) {
        return T{};
    }
    
    auto array = element.get_array().value;
    T result{};
    
    size_t i = 0;
    for (const auto& item : array) {
        if(i >= result.size()){
            throw std::runtime_error("bson array size mismatch");
        }
        result[i++] = extract_array_element<std::remove_extent_t<T>>(item);
    }
    
    return result;
}

template<OptionalType T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if(!element || element.type() == bsoncxx::type::k_null) {
        return std::nullopt;
    }
    
    using ValueType = typename T::value_type;
    
    if constexpr (AggregateStruct<ValueType>) {
        auto sub_doc = element.get_document().view();
        return T{from_bson<ValueType>(sub_doc)};
    } else {
        return T{extract_dispatch<ValueType>(doc, name)};
    }
}

template<AggregateStruct T>
T extract_field(const bsoncxx::document::view& doc, std::string_view name) {
    auto element = doc[name];
    if (!element) {
        throw std::runtime_error(std::string("nested bson field not found: ") + std::string(name));
    }
    auto sub_doc = element.get_document().view();
    return from_bson<T>(sub_doc);
}

template<typename T>
T extract_array_element(const bsoncxx::array::element& element) {
    if constexpr (std::is_same_v<T, std::string>) {
        return std::string(element.get_string().value);
    }
    else if constexpr (std::is_same_v<T, int32_t>) {
        return element.get_int32();
    }
    else if constexpr (std::is_same_v<T, int64_t>) {
        return element.get_int64();
    }
    else if constexpr (std::is_same_v<T, float>) {
        return static_cast<float>(element.get_double());
    }
    else if constexpr (std::is_same_v<T, double>) {
        return element.get_double();
    }
    else if constexpr (std::is_same_v<T, bool>) {
        return element.get_bool();
    }
    else if constexpr (std::is_same_v<T, bsoncxx::oid>) {
        if(element.type() == bsoncxx::type::k_string){
            return bsoncxx::oid{element.get_string().value};
        }
        return element.get_oid().value;
    }
    else if constexpr (BsonDateType<T>) {
        if (element.type() == bsoncxx::type::k_date) {
            return mongoose::utils::from_bson_date(element.get_date().value.count());
        }
        if (element.type() == bsoncxx::type::k_int64) {
            return mongoose::utils::from_bson_date(element.get_int64().value);
        }
    }
    else if constexpr (BsonUuidType<T>) {
        if (element.type() == bsoncxx::type::k_binary){
            bsoncxx::types::b_binary binary_value = element.get_binary();
            if (binary_value.sub_type == bsoncxx::binary_sub_type::k_uuid){
                return mongoose::utils::from_bson_uuid(element.get_binary());
            }
        }
    }
    else if constexpr (AggregateStruct<T>) {
        auto sub_doc = element.get_document().view();
        return from_bson<T>(sub_doc);
    }
    else if constexpr (EnumType<T>) {
        return static_cast<T>(element.get_int32());
    }
    else if constexpr (VectorType<T>) {
        auto nested_array = element.get_array().value;
        T result;
        for (const auto& nested_item : nested_array) {
            using ItemType = typename T::value_type;
            result.push_back(extract_array_element<ItemType>(nested_item));
        }
        return result;
    }
    else {
        static_assert(always_false_v<T>, "bson unsupported type in array element");
    }
}

template<typename T>
void serialize_dispatch(bsoncxx::builder::stream::document& builder, std::string_view name, const T& value) {
    if constexpr (is_custom_serializable<T>::value) {
        auto custom_doc = traits::serialize_custom(value);
        builder << name << custom_doc.view();
    } else {
        serialize_field_impl(builder, name, value);
    }
}

template<typename T>
T extract_dispatch(const bsoncxx::document::view& doc, std::string_view name) {
    if constexpr (is_custom_serializable<T>::value) {
        return traits::deserialize_custom<T>(doc, name);
    } else {
        return extract_field<T>(doc, name);
    }
}

}

// main serialize/deserialize functions
namespace mongoose {

    template<typename T>
    bsoncxx::document::value to_bson(const T& obj) {
        static_assert(std::is_aggregate_v<T>, "T must be an aggregate type");

        auto builder = bsoncxx::builder::stream::document{};

        boost::pfr::for_each_field(obj, [&builder](const auto& field, auto index) {
            constexpr std::string_view field_name = boost::pfr::get_name<index(), T>();
            traits::serialize_dispatch(builder, field_name, field);
        });

        return builder << bsoncxx::builder::stream::finalize;
    }

    template<typename T>
    T from_bson(const bsoncxx::document::view& doc) {
        static_assert(std::is_aggregate_v<T>, "T must be an aggregate type");

        T result;

        boost::pfr::for_each_field(result, [&doc](auto& field, auto index) {
            constexpr std::string_view field_name = boost::pfr::get_name<index(), T>();
            field = traits::extract_dispatch<std::decay_t<decltype(field)>>(doc, field_name);
        });

        return result;
    }

    template<typename T, typename... ExcludeFields>
    bsoncxx::document::value to_bson_exclude(const T& obj, ExcludeFields... exclude_fields) {
        static_assert(std::is_aggregate_v<T>, "T must be an aggregate type");
        static_assert(
            (std::is_convertible_v<ExcludeFields, std::string_view> && ...),
            "all exclude fields must be convertible to string_view"
        );
        
        auto builder = bsoncxx::builder::stream::document{};
        
        boost::pfr::for_each_field(obj, [&](const auto& field, auto index) {
            constexpr std::string_view field_name = boost::pfr::get_name<index(), T>();
            // check field name, skip loop if contain
            if (details::exclude_contains(field_name, exclude_fields...)) {
                return;
            }
            traits::serialize_dispatch(builder, field_name, field);
        });
        
        return builder << bsoncxx::builder::stream::finalize;
    }

    template<typename T, typename... ExcludeFields>
    T from_bson_exclude(const bsoncxx::document::view& doc, ExcludeFields... exclude_fields) {
        static_assert(std::is_aggregate_v<T>, "T must be an aggregate type");
        static_assert(
            (std::is_convertible_v<ExcludeFields, std::string_view> && ...),
            "all exclude fields must be convertible to string_view"
        );
        
        T result;
        
        boost::pfr::for_each_field(result, [&](auto& field, auto index) {
            constexpr std::string_view field_name = boost::pfr::get_name<index(), T>();
            // check field name, skip loop if contain
            if (details::exclude_contains(field_name, exclude_fields...)) {
                return;
            }
            field = traits::extract_dispatch<std::decay_t<decltype(field)>>(doc, field_name);
        });
        
        return result;
    }
    
}

// serialize to projection bson
namespace mongoose {

    template<typename T, typename... ExcludeFields>
    bsoncxx::document::value to_projection(ExcludeFields... exclude_fields) {
        static_assert(std::is_aggregate_v<T>, "T must be an aggregate type");
        static_assert(
            (std::is_convertible_v<ExcludeFields, std::string_view> && ...), 
            "all exclude fields must be convertible to string_view"
        );

        auto builder = bsoncxx::builder::stream::document{};
        constexpr std::size_t field_count = boost::pfr::tuple_size_v<T>;

        // fold expression to iterate over indices
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ( [&]() {
                constexpr std::string_view field_name = boost::pfr::get_name<Is, T>();
                // check field name, skip loop if contain
                if (!details::exclude_contains(field_name, exclude_fields...)) {
                    builder << field_name << 1;
                }
            }(), ...);
        }
        (std::make_index_sequence<field_count>{});

        return builder << bsoncxx::builder::stream::finalize;
    }

}

#endif