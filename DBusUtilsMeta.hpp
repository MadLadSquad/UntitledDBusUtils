#pragma once
#include <dbus/dbus.h>

#include <typeinfo>
#include <type_traits>

#include <map>
#include <vector>
#include <string>
#include <array>
#include <deque>
#include <functional>

#include <cstdint>
#include <cstdarg>
#include <cstring>

// Use this instead of dbus_bool_t
struct udbus_bool_t
{
    udbus_bool_t(dbus_bool_t dbus) noexcept;
    operator bool() noexcept;
    dbus_bool_t b;
};

namespace UDBus
{
    // Casual forward declarations
    struct Variant;
    struct Iterator;

    template<typename T>
    struct ContainerVariantTemplate;

    // Go around the C++ type system using Tag dispatching. This struct will have specialisations for all types we
    // want to support. Check the macro below
    template<typename T>
    struct Tag;

    // Specialises the Tag struct with a type definition and a constexpr string that represents the type
    #define MAKE_FUNC_BASIC(x, y) template<> struct Tag<x> { using Type = x; static constexpr char TypeString = y; }

    MAKE_FUNC_BASIC(const char*,                    DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(char*,                          DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(int8_t,                         DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(uint8_t,                        DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(dbus_int16_t,                   DBUS_TYPE_INT16     );
    MAKE_FUNC_BASIC(dbus_uint16_t,                  DBUS_TYPE_UINT16    );
    MAKE_FUNC_BASIC(dbus_int32_t,                   DBUS_TYPE_INT32     );
    MAKE_FUNC_BASIC(dbus_uint32_t,                  DBUS_TYPE_UINT32    );
    // uint32_t before bool because bool is an uint32. Retarded af fr
    MAKE_FUNC_BASIC(udbus_bool_t,                   DBUS_TYPE_BOOLEAN   );
    MAKE_FUNC_BASIC(dbus_int64_t,                   DBUS_TYPE_INT64     );
    MAKE_FUNC_BASIC(dbus_uint64_t,                  DBUS_TYPE_UINT64    );

    MAKE_FUNC_BASIC(std::vector<const char*>,       DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(std::vector<char*>,             DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(std::vector<int8_t>,            DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(std::vector<uint8_t>,           DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(std::vector<dbus_int16_t>,      DBUS_TYPE_INT16     );
    MAKE_FUNC_BASIC(std::vector<dbus_uint16_t>,     DBUS_TYPE_UINT16    );
    MAKE_FUNC_BASIC(std::vector<dbus_int32_t>,      DBUS_TYPE_INT32     );
    MAKE_FUNC_BASIC(std::vector<dbus_uint32_t>,     DBUS_TYPE_UINT32    );
    // uint32_t before bool again. I hate this fucking library
    MAKE_FUNC_BASIC(std::vector<udbus_bool_t>,      DBUS_TYPE_BOOLEAN   );
    MAKE_FUNC_BASIC(std::vector<dbus_int64_t>,      DBUS_TYPE_INT64     );
    MAKE_FUNC_BASIC(std::vector<dbus_uint64_t>,     DBUS_TYPE_UINT64    );

    template <template <typename...> class Base, typename T>
    struct is_specialisation_of : std::false_type {};

    // Specialization that matches the template we are interested in.
    template <template <typename...> class Base, typename... Args>
    struct is_specialisation_of<Base, Base<Args...>> : std::true_type {};

    template <typename T, typename = void>
    struct is_complete : std::false_type {};

    // Used to check if a type exists
    template <typename T>
    struct is_complete<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};

    template<typename T, typename... T2>
    class Struct;

    // Forward declarations to allow the consumer to plug custom types in
    template<typename T>
    constexpr bool is_map_type() noexcept;

    template<typename T>
    constexpr bool is_array_type() noexcept;

    #define DECLARE_TYPE_AND_STRUCT_ADDITIONAL(return_type, name, arg_name, additional_arg, body)   \
    template<typename TT, typename... TT2>                                                          \
    return_type name##Struct(UDBus::Struct<TT, TT2...>& arg_name, additional_arg) noexcept body     \
                                                                                                    \
    template<typename TT, typename... TT2>                                                          \
    return_type name(UDBus::Type<TT, TT2...>& arg_name, additional_arg) noexcept body

    #define DECLARE_TYPE_AND_STRUCT(return_type, name, arg_name, body)                              \
    template<typename TT, typename... TT2>                                                          \
    return_type name##Struct(UDBus::Struct<TT, TT2...>& arg_name) noexcept body                     \
                                                                                                    \
    template<typename TT, typename... TT2>                                                          \
    return_type name(UDBus::Type<TT, TT2...>& arg_name) noexcept body

    template<typename T, typename... T2>
    class Type
    {
    public:
        Type() noexcept = default;
        Type(T& t, T2&... args) noexcept
        {
            init(t, args...);
        }

        void init(T& t, T2&... args) noexcept
        {
            data = &t;
            n = Type<T2...>(args...);
        }

        Type<T2...>* next() noexcept
        {
            return &n;
        }

        template<typename TT, typename... TT2>
        static void destroy(Type<TT, TT2...>& t) noexcept
        {
            destroy_i(t);
        }

        T* data = nullptr;
        Type<T2...> n{};
    private:
        DECLARE_TYPE_AND_STRUCT(static void, destroy_i, t, {
            if constexpr (is_specialisation_of<Struct, TT>{})
            {
                destroy_iStruct(*t.data);
                delete t.data;
            }
            else if constexpr (std::is_same<Variant, TT>::value)
                delete t.data;
            else if constexpr (is_specialisation_of<ContainerVariantTemplate, TT>{})
            {
                delete t.data->v;
                delete t.data;
            }

            if (t.next() != nullptr)
                destroy_i(*t.next());
        })
    };

    template<typename T>
    class Type<T>
    {
    public:
        Type() noexcept = default;
        Type(T& t) noexcept
        {
            init(t);
        }

        void init(T& t) noexcept
        {
            data = &t;
        }

        constexpr Type<T>* next() noexcept
        {
            return nullptr;
        }

        T* data = nullptr;
    };

    template<typename T, typename... T2>
    class Struct : public Type<T, T2...>
    {
    public:
        Struct() noexcept : Type<T, T2...>() {};
        Struct(T& t, T2&... args) noexcept : Type<T, T2...>(t, args...) {};
    };

    template<typename T>
    struct Struct<T> : public Type<T>
    {
    public:
        Struct() noexcept = default;
        Struct(T& t) noexcept : Type<T>(t) {};
    };

    template<typename T, typename... T2>
    Struct<T, T2...>& makeStruct(Struct<T, T2...>&& t) noexcept
    {
        auto* tmp = new Struct<T, T2...>{t};
        return *tmp;
    }

    struct Variant
    {
        std::function<bool(Iterator&, void*)> f = [](Iterator&, void*) -> bool { return true; };
    };

    Variant& makeVariant(Variant&& v) noexcept;

    struct IgnoreType
    {
        char data = 0;
    };

    IgnoreType& ignore() noexcept;

    template<typename T>
    struct ContainerVariantTemplate
    {
        T* t = nullptr;
        Variant* v = nullptr;
    };

    template<typename T>
    ContainerVariantTemplate<T>& associateWithVariant(T& t, Variant& v) noexcept
    {
        auto* tmp = new ContainerVariantTemplate<T>{ &t, &v };
        return *tmp;
    }

    enum MessageGetResult
    {
        RESULT_SUCCESS,

        RESULT_MORE_FIELDS_THAN_REQUIRED,
        RESULT_LESS_FIELDS_THAN_REQUIRED,

        RESULT_INVALID_BASIC_TYPE,
        RESULT_INVALID_STRUCT_TYPE,
        RESULT_INVALID_ARRAY_TYPE,
        RESULT_INVALID_DICTIONARY_TYPE,
        RESULT_INVALID_DICTIONARY_KEY,
        RESULT_INVALID_VARIANT_TYPE,
        RESULT_INVALID_VARIANT_PARSING,
    };

#ifndef UDBUS_USING_CUSTOM_DICT_TYPES
    template<typename T>
    constexpr bool is_map_type() noexcept
    {
        if constexpr (is_specialisation_of<std::unordered_map, T>{} || is_specialisation_of<std::map, T>{})
            return true;
        return false;
    }
#endif

#ifndef UDBUS_USING_CUSTOM_ARRAY_TYPES
    template<typename T>
    constexpr bool is_array_type() noexcept
    {
        if constexpr (is_specialisation_of<std::vector, T>{})
            return true;
        return false;
    }
#endif
}