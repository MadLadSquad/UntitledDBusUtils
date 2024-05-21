#include "DBusUtils.hpp"

UDBus::Error::Error()
{
    dbus_error_init(&error);
}

UDBus::Error::operator DBusError*() noexcept
{
    return &error;
}

void UDBus::Error::set(const char* name, const char* message) noexcept
{
    dbus_set_error_const(&error, name, message);
}

void UDBus::Error::move(DBusError* src, DBusError* dest) noexcept
{
    dbus_move_error(src, dest);
}

void UDBus::Error::move(UDBus::Error& src, UDBus::Error& dest) noexcept
{
    dbus_move_error(src, dest);
}

bool UDBus::Error::has_name(const char* name) const noexcept
{
    return dbus_error_has_name(&error, name);;
}

bool UDBus::Error::is_set() noexcept
{
    return dbus_error_is_set(&error);
}

void UDBus::Error::free() noexcept
{
    if (is_set())
        dbus_error_free(&error);
}

UDBus::Error::~Error()
{
    free();
}

const char* UDBus::Error::name() const noexcept
{
    return error.name;
}

const char* UDBus::Error::message() const noexcept
{
    return error.message;
}

UDBus::Error::Error(const DBusError& err) noexcept
{
    error = err;
}
