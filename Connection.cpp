#include "DBusUtils.hpp"

UDBus::Connection::~Connection() noexcept
{
    unref();
}

UDBus::Connection::operator DBusConnection*() noexcept
{
    return connection;
}

void UDBus::Connection::unref() noexcept
{
    dbus_connection_unref(connection);
}

void UDBus::Connection::close() noexcept
{
    dbus_connection_close(connection);
}

void UDBus::Connection::bus_get(DBusBusType type, UDBus::Error& error) noexcept
{
    connection = dbus_bus_get(type, error);
}

void UDBus::Connection::bus_get_private(DBusBusType type, UDBus::Error& error) noexcept
{
    connection = dbus_bus_get_private(type, error);
}

void UDBus::Connection::open(const char* address, UDBus::Error& error) noexcept
{
    connection = dbus_connection_open(address, error);
}

void UDBus::Connection::open_private(const char* address, UDBus::Error& error) noexcept
{
    connection = dbus_connection_open_private(address, error);
}

void UDBus::Connection::ref(UDBus::Connection& conn) noexcept
{
    connection = dbus_connection_ref(conn);
}

void UDBus::Connection::ref(DBusConnection* conn) noexcept
{
    connection = dbus_connection_ref(conn);
}

UDBus::Connection::Connection(DBusConnection* conn) noexcept
{
    connection = conn;
}

void UDBus::Connection::flush() noexcept
{
    dbus_connection_flush(connection);
}

udbus_bool_t UDBus::Connection::send(UDBus::Message& message, dbus_uint32_t* client_serial) noexcept
{
    return dbus_connection_send(connection, message, client_serial);
}

udbus_bool_t UDBus::Connection::send_with_reply(UDBus::Message& message, UDBus::PendingCall& pending_return, int timeout_milliseconds) noexcept
{
    return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);
}

UDBus::Message UDBus::Connection::send_with_reply_and_block(UDBus::Message& message, int timeout_mislliseconds, UDBus::Error& error) noexcept
{
    return Message(dbus_connection_send_with_reply_and_block(connection, message, timeout_mislliseconds, error));
}
