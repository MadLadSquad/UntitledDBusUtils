// The purpose of this file is to contain all default allowed and forbidden types.
// The file is designed to be user-readable so that users can adapt their custom types
// to the format of DBus and UntitledDBusUtils
#pragma once
#include <dbus/dbus.h>

#include <type_traits>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

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

#define DISALLOW_STRING_CONTAINER_TAG(x) DISALLOW_TAG(x, "String wrappers are not able to be serialised. Convert them to either const char* or char*")
#define DISALLOW_STRING_TAG(x) DISALLOW_TAG(x, "DBus only accepts strings with UTF-8 encoding. Use either char* or const char* instead.")

    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::string>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::string_view>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::wstring>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::wstring_view>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::u8string>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::u8string_view>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::u16string>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::u16string_view>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::u32string>);
    DISALLOW_STRING_CONTAINER_TAG(std::vector<std::u32string_view>);

    DISALLOW_STRING_CONTAINER_TAG(std::string_view);
    DISALLOW_STRING_CONTAINER_TAG(std::string);
    DISALLOW_STRING_CONTAINER_TAG(std::wstring);
    DISALLOW_STRING_CONTAINER_TAG(std::wstring_view);
    DISALLOW_STRING_CONTAINER_TAG(std::u16string);
    DISALLOW_STRING_CONTAINER_TAG(std::u16string_view);
    DISALLOW_STRING_CONTAINER_TAG(std::u32string);
    DISALLOW_STRING_CONTAINER_TAG(std::u32string_view);

    DISALLOW_STRING_TAG(wchar_t*)
    DISALLOW_STRING_TAG(const wchar_t*)
    DISALLOW_STRING_TAG(char16_t*)
    DISALLOW_STRING_TAG(const char16_t*)
    DISALLOW_STRING_TAG(char32_t*)
    DISALLOW_STRING_TAG(const char32_t*)


    // Specialises the Tag struct with a type definition and a constexpr string that represents the type
    #define MAKE_TAG(x, y) template<> struct Tag<x> { using Type = x; static constexpr char TypeString = y; }

    MAKE_TAG(const char*,                   DBUS_TYPE_STRING    );
    MAKE_TAG(char*,                         DBUS_TYPE_STRING    );

    MAKE_TAG(int8_t,                        DBUS_TYPE_BYTE      );
    MAKE_TAG(uint8_t,                       DBUS_TYPE_BYTE      );
    MAKE_TAG(dbus_int16_t,                  DBUS_TYPE_INT16     );
    MAKE_TAG(dbus_uint16_t,                 DBUS_TYPE_UINT16    );
    MAKE_TAG(dbus_int32_t,                  DBUS_TYPE_INT32     );
    MAKE_TAG(dbus_uint32_t,                 DBUS_TYPE_UINT32    );
    // uint32_t before bool because bool is an uint32!??!?!?!?!?
    MAKE_TAG(udbus_bool_t,                  DBUS_TYPE_BOOLEAN   );
    MAKE_TAG(dbus_int64_t,                  DBUS_TYPE_INT64     );
    MAKE_TAG(dbus_uint64_t,                 DBUS_TYPE_UINT64    );

    // There is no floating point type, instead use doubles
    MAKE_TAG(double,                        DBUS_TYPE_DOUBLE    );

    MAKE_TAG(std::vector<const char*>,      DBUS_TYPE_STRING    );
    MAKE_TAG(std::vector<char*>,            DBUS_TYPE_STRING    );
    MAKE_TAG(std::vector<int8_t>,           DBUS_TYPE_BYTE      );
    MAKE_TAG(std::vector<uint8_t>,          DBUS_TYPE_BYTE      );
    MAKE_TAG(std::vector<dbus_int16_t>,     DBUS_TYPE_INT16     );
    MAKE_TAG(std::vector<dbus_uint16_t>,    DBUS_TYPE_UINT16    );
    MAKE_TAG(std::vector<dbus_int32_t>,     DBUS_TYPE_INT32     );
    MAKE_TAG(std::vector<dbus_uint32_t>,    DBUS_TYPE_UINT32    );
    // Bool should always be after uint32_t
    MAKE_TAG(std::vector<udbus_bool_t>,     DBUS_TYPE_BOOLEAN   );
    MAKE_TAG(std::vector<dbus_int64_t>,     DBUS_TYPE_INT64     );
    MAKE_TAG(std::vector<dbus_uint64_t>,    DBUS_TYPE_UINT64    );

    // Forward declarations to allow the consumer to plug custom types in
    template<typename T>
    constexpr bool is_map_type() noexcept;

    template<typename T>
    constexpr bool is_array_type() noexcept;

    template <template <typename...> class Base, typename T>
    struct is_specialisation_of : std::false_type {};

    // Specialization that matches the template we are interested in.
    template <template <typename...> class Base, typename... Args>
    struct is_specialisation_of<Base, Base<Args...>> : std::true_type {};

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
