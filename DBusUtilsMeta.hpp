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
    udbus_bool_t() noexcept = default;
    udbus_bool_t(dbus_bool_t dbus) noexcept;
    operator bool() const noexcept;

    dbus_bool_t b{};
};

namespace UDBus
{
    // Casual forward declarations
    struct Variant;
    class Iterator;

    template<typename T>
    struct ContainerVariantTemplate;

    // Go around the C++ type system using Tag dispatching. This struct will have specialisations for all types we
    // want to support. Check the macro below
    template<typename T, typename = void>
    struct Tag;

    // Disallow types
    #define DISALLOW_TAG(x, y)                            \
    template<typename T>                                  \
    struct Tag<T, std::enable_if_t<std::is_same_v<T, x>>> \
    {                                                     \
        static_assert(!std::is_same_v<T, x>, y);          \
    };

    DISALLOW_TAG(float, "DBus does not support 4-byte floating point numbers. Use doubles instead.");
    DISALLOW_TAG(bool, "We do not support normal boolean types. Use udbus_bool_t instead.");

    // Specialises the Tag struct with a type definition and a constexpr string that represents the type
    #define MAKE_TAG(x, y) template<> struct Tag<x> { using Type = x; static constexpr char TypeString = y; }

    MAKE_TAG(const char*,                    DBUS_TYPE_STRING    );
    MAKE_TAG(char*,                          DBUS_TYPE_STRING    );
    MAKE_TAG(int8_t,                         DBUS_TYPE_BYTE      );
    MAKE_TAG(uint8_t,                        DBUS_TYPE_BYTE      );
    MAKE_TAG(dbus_int16_t,                   DBUS_TYPE_INT16     );
    MAKE_TAG(dbus_uint16_t,                  DBUS_TYPE_UINT16    );
    MAKE_TAG(dbus_int32_t,                   DBUS_TYPE_INT32     );
    MAKE_TAG(dbus_uint32_t,                  DBUS_TYPE_UINT32    );
    // uint32_t before bool because bool is an uint32!??!?!?!?!?
    MAKE_TAG(udbus_bool_t,                   DBUS_TYPE_BOOLEAN   );
    MAKE_TAG(dbus_int64_t,                   DBUS_TYPE_INT64     );
    MAKE_TAG(dbus_uint64_t,                  DBUS_TYPE_UINT64    );
    
    // There is no floating point type, instead use doubles
    MAKE_TAG(double,                         DBUS_TYPE_DOUBLE    );

    MAKE_TAG(std::vector<const char*>,       DBUS_TYPE_STRING    );
    MAKE_TAG(std::vector<char*>,             DBUS_TYPE_STRING    );
    MAKE_TAG(std::vector<int8_t>,            DBUS_TYPE_BYTE      );
    MAKE_TAG(std::vector<uint8_t>,           DBUS_TYPE_BYTE      );
    MAKE_TAG(std::vector<dbus_int16_t>,      DBUS_TYPE_INT16     );
    MAKE_TAG(std::vector<dbus_uint16_t>,     DBUS_TYPE_UINT16    );
    MAKE_TAG(std::vector<dbus_int32_t>,      DBUS_TYPE_INT32     );
    MAKE_TAG(std::vector<dbus_uint32_t>,     DBUS_TYPE_UINT32    );
    // Bool should always be after uint32_t
    MAKE_TAG(std::vector<udbus_bool_t>,      DBUS_TYPE_BOOLEAN   );
    MAKE_TAG(std::vector<dbus_int64_t>,      DBUS_TYPE_INT64     );
    MAKE_TAG(std::vector<dbus_uint64_t>,     DBUS_TYPE_UINT64    );

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

    // These macros are needed because I couldn't find a clean way of creating a function that has
    // the same body but supports getting both a Struct and a Type
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

    struct IgnoreType
    {
        char data = 0;
    };

    // Use this if you don't want or need the information in a certain field of the message
    IgnoreType& ignore() noexcept;

    struct BumpType
    {
        char data = 0;
    };

    // Hack to support single-field messages
    BumpType& bump() noexcept;

// Simplifies the code a bit
#define UDBUS_FREE_TYPE(x) decltype(x)::destroy(x)

    // The Type compile-time tree which is used to build the schema
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
            bool bWasInArrayOrMap = false;
            bool bDestroyEverything = false;
            destroy_i(t, bWasInArrayOrMap, bDestroyEverything);
        }

        template<typename TT>
        static void destroyComplex(TT& t) noexcept
        {
            bool bWasInArrayOrMap = false;
            bool bDestroyEverything = false;
            routeDestroy(t, bWasInArrayOrMap, bDestroyEverything);
        }

        T* data = nullptr;
        Type<T2...> n{};
    private:
        template<typename TT>
        static void routeDestroy(TT& t, bool& bWasInArrayOrMap, bool& bDestroyEverything) noexcept
        {
            const bool bParentIsArrayOrMap = bWasInArrayOrMap;
            const bool bParentShouldDestroyEverything = bDestroyEverything;
            if constexpr (is_specialisation_of<Struct, TT>{})
            {
                if (t.bShouldBeFreed || t.bIsOrigin)
                    bDestroyEverything = true;

                destroy_iStruct(t, bWasInArrayOrMap, bDestroyEverything);
                if (!bWasInArrayOrMap || (bDestroyEverything && !t.bIsOrigin))
                    delete &t;

                if (!bParentShouldDestroyEverything)
                    bDestroyEverything = false;
            }
            else if constexpr (std::is_same_v<Variant, TT>)
            {
                if (!bWasInArrayOrMap || bDestroyEverything)
                    delete &t;
            }
            else if constexpr (is_specialisation_of<ContainerVariantTemplate, TT>{})
            {
                delete t.v;
                if (!bWasInArrayOrMap || bDestroyEverything)
                    delete &t;
            }
            else if constexpr (is_array_type<TT>())
            {
                bWasInArrayOrMap = true;
                for (auto& a : t)
                    routeDestroy(a, bWasInArrayOrMap, bDestroyEverything);
                t.clear();
                if (!bParentIsArrayOrMap)
                    bWasInArrayOrMap = false;

                if (bDestroyEverything)
                    delete &t;
            }
            else if constexpr (is_map_type<TT>())
            {
                bWasInArrayOrMap = true;
                for (auto& [key, val] : t)
                    routeDestroy(val, bWasInArrayOrMap, bDestroyEverything);
                t.clear();
                if (!bParentIsArrayOrMap)
                    bWasInArrayOrMap = false;

                if (bDestroyEverything)
                    delete &t;
            }
            else if constexpr (!std::is_same_v<IgnoreType, TT> || !std::is_same_v<BumpType, TT>)
                if (bDestroyEverything)
                    delete &t;
        }

        template<typename TT, typename ...TT2>
        static void destroy_i(Type<TT, TT2...>& t, bool& bWasInArrayOrMap, bool& bDestroyEverything) noexcept
        {
            routeDestroy(*t.data, bWasInArrayOrMap, bDestroyEverything);
            if (t.next() != nullptr)
                destroy_i(*t.next(), bWasInArrayOrMap, bDestroyEverything);
        }

        template<typename TT, typename ...TT2>
        static void destroy_iStruct(Struct<TT, TT2...>& t, bool& bWasInArrayOrMap, bool& bDestroyEverything) noexcept
        {
            routeDestroy(*t.data, bWasInArrayOrMap, bDestroyEverything);
            if (t.next() != nullptr)
                destroy_i(*t.next(), bWasInArrayOrMap, bDestroyEverything);
        }
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

    // The Struct compile-time tree that is used for nested structures
    template<typename T, typename... T2>
    class Struct : public Type<T, T2...>
    {
    public:
        Struct() noexcept : Type<T, T2...>() {};
        Struct(T& t, T2&... args) noexcept : Type<T, T2...>(t, args...) {};

        bool bShouldBeFreed = false;
        bool bIsOrigin = false;
    };

    template<typename T>
    class Struct<T> : public Type<T>
    {
    public:
        Struct() noexcept = default;
        Struct(T& t) noexcept : Type<T>(t) {};

        bool bShouldBeFreed = false;
        bool bIsOrigin = false;
    };

    // Allocates a new structure for building inline schemas
    template<typename T, typename... T2>
    Struct<T, T2...>& makeStruct(Struct<T, T2...>&& t) noexcept
    {
        auto* tmp = new Struct<T, T2...>{t};
        return *tmp;
    }

    class Message;

    struct Variant
    {
        std::function<bool(Message&, Iterator&, void**, void*)> parse = [](Message&, Iterator&, void**, void*) -> bool { return true; };
        void* data = nullptr;
    };

    Variant& makeVariant(Variant&& v) noexcept;

    // Since variants can be part of a container it's important that we allow for a dynamic way
    // to allocate these variants but still associate them with some user-defined function.
    // This type allows us to push a variant template that we can allocate for every element
    // of the container
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

        RESULT_NOT_CALLED,

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