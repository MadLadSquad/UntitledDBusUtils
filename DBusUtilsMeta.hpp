#pragma once
#include "DBusUtilsTags.hpp"
#include "DBusUtilsStructs.hpp"

namespace UDBus
{
    template <typename T, typename = void>
    struct is_complete : std::false_type {};

    // Used to check if a type exists
    template <typename T>
    struct is_complete<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};
}