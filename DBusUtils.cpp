#include "DBusUtils.hpp"

UDBus::Types& UDBus::Types::get() noexcept
{
    static Types manager;
    return manager;
}

udbus_bool_t::udbus_bool_t(dbus_bool_t dbus) noexcept
{
    b = dbus;
}
