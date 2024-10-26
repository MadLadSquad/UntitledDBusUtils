#include "DBusUtils.hpp"

UDBus::Connection::~Connection() noexcept
{
    unref();
}

UDBus::Connection::operator DBusConnection*() const noexcept
{
    return connection;
}

void UDBus::Connection::unref() const noexcept
{
    dbus_connection_unref(connection);
}

void UDBus::Connection::close() const noexcept
{
    dbus_connection_close(connection);
}

void UDBus::Connection::bus_get(const DBusBusType type, UDBus::Error& error) noexcept
{
    connection = dbus_bus_get(type, error);
}

void UDBus::Connection::bus_get_private(const DBusBusType type, UDBus::Error& error) noexcept
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

void UDBus::Connection::ref(const UDBus::Connection& conn) noexcept
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

void UDBus::Connection::flush() const noexcept
{
    dbus_connection_flush(connection);
}

udbus_bool_t UDBus::Connection::send(UDBus::Message& message, dbus_uint32_t* client_serial) const noexcept
{
    return dbus_connection_send(connection, message, client_serial);
}

udbus_bool_t UDBus::Connection::send_with_reply(UDBus::Message& message, UDBus::PendingCall& pending_return, const int timeout_milliseconds) const noexcept
{
    return dbus_connection_send_with_reply(connection, message, pending_return, timeout_milliseconds);
}

UDBus::Message UDBus::Connection::send_with_reply_and_block(UDBus::Message& message, const int timeout_milliseconds, UDBus::Error& error) const noexcept
{
    return Message(dbus_connection_send_with_reply_and_block(connection, message, timeout_milliseconds, error));
}

int UDBus::Connection::request_name(const char* name, const unsigned int flags, UDBus::Error& error) const noexcept
{
    return dbus_bus_request_name(connection, name, flags, error);
}

udbus_bool_t UDBus::Connection::read_write(const int timeout_milliseconds) const noexcept
{
    return dbus_connection_read_write(connection, timeout_milliseconds);
}

udbus_bool_t UDBus::Connection::read_write_dispatch(const int timeout_milliseconds) const noexcept
{
    return dbus_connection_read_write_dispatch(connection, timeout_milliseconds);
}

UDBus::Message UDBus::Connection::pop_message() const noexcept
{
    return Message(dbus_connection_pop_message(connection));
}

