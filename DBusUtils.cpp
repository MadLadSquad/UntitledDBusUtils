#include "DBusUtils.hpp"

udbus_bool_t::udbus_bool_t(const dbus_bool_t dbus) noexcept
{
    b = dbus;
}

udbus_bool_t::operator bool() const noexcept
{
    return b;
}
