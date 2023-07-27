#include "DBusUtils.hpp"

UDBus::Message::~Message() noexcept
{
    if (message != nullptr)
        dbus_message_unref(message);
}

DBusMessage* UDBus::Message::get() noexcept
{
    return message;
}

DBusMessage** UDBus::Message::getMessagePointer() noexcept
{
    return &message;
}

void UDBus::Message::new_method_call(const char* bus_name, const char* path, const char* interface, const char* func) noexcept
{
    message = dbus_message_new_method_call(bus_name, path, interface, func);
}

void UDBus::Message::new_1(int messageType) noexcept
{
    message = dbus_message_new(messageType);
}

void UDBus::Message::new_method_return(UDBus::Message& method_call) noexcept
{
    message = dbus_message_new_method_return(GET_MESSAGE(method_call));
}

void UDBus::Message::new_method_return(DBusMessage* method_call) noexcept
{
    message = dbus_message_new_method_return(method_call);
}

void UDBus::Message::new_signal(const char* path, const char* interface, const char* name) noexcept
{
    message = dbus_message_new_signal(path, interface, name);
}

void UDBus::Message::new_error(UDBus::Message& reply_to, const char* error_name, const char* error_message) noexcept
{
    message = dbus_message_new_error(GET_MESSAGE(reply_to), error_name, error_message);
}

void UDBus::Message::new_error_raw(DBusMessage* reply_to, const char* error_name, const char* error_message) noexcept
{
    message = dbus_message_new_error(reply_to, error_name, error_message);
}

void UDBus::Message::copy(UDBus::Message& reply_to) noexcept
{
    message = dbus_message_copy(GET_MESSAGE(reply_to));
}

void UDBus::Message::copy(DBusMessage* reply_to) noexcept
{
    message = dbus_message_copy(reply_to);
}

void UDBus::Message::ref(UDBus::Message& reply_to) noexcept
{
    message = dbus_message_ref(GET_MESSAGE(reply_to));
}

void UDBus::Message::ref(DBusMessage* reply_to) noexcept
{
    message = dbus_message_ref(reply_to);
}

void UDBus::Message::demarshal(const char* str, int len, DBusError* error) noexcept
{
    message = dbus_message_demarshal(str, len, error);
}

UDBus::Message::operator DBusMessage*() noexcept
{
    return message;
}

void UDBus::Message::pending_call_steal_reply(DBusPendingCall* pending) noexcept
{
    message = dbus_pending_call_steal_reply(pending);
}

UDBus::Message::Message(DBusMessage* msg) noexcept
{
    message = msg;
}
