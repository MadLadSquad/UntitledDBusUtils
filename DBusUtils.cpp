#include "DBusUtils.hpp"

UDBus::Types& UDBus::Types::get() noexcept
{
    static Types manager;
    return manager;
}
