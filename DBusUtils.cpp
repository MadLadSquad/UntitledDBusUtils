#include "DBusUtils.hpp"

udbus_bool_t::udbus_bool_t(dbus_bool_t dbus) noexcept
{
    b = dbus;
}

udbus_bool_t::operator bool() noexcept
{
    return b;
}
