#include <ranges>
#include "DBusUtils.hpp"

void UDBus::Message::setUserPointer(void* ptr) noexcept
{
    userPointer = ptr;
}

UDBus::Message::~Message() noexcept
{
    unref();
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
    message = dbus_message_new_method_return(UDBUS_GET_MESSAGE(method_call));
}

void UDBus::Message::new_method_return(DBusMessage* method_call) noexcept
{
    message = dbus_message_new_method_return(method_call);
}

UDBus::Message UDBus::Message::new_method_return_s(UDBus::Message& method_call) noexcept
{
    return UDBus::Message(dbus_message_new_method_return(UDBUS_GET_MESSAGE(method_call)));
}

UDBus::Message UDBus::Message::new_method_return_s(DBusMessage* method_call) noexcept
{
    return UDBus::Message(dbus_message_new_method_return(method_call));
}

void UDBus::Message::new_signal(const char* path, const char* interface, const char* name) noexcept
{
    message = dbus_message_new_signal(path, interface, name);
}

void UDBus::Message::new_error(UDBus::Message& reply_to, const char* error_name, const char* error_message) noexcept
{
    message = dbus_message_new_error(UDBUS_GET_MESSAGE(reply_to), error_name, error_message);
}

void UDBus::Message::new_error_raw(DBusMessage* reply_to, const char* error_name, const char* error_message) noexcept
{
    message = dbus_message_new_error(reply_to, error_name, error_message);
}

void UDBus::Message::copy(UDBus::Message& reply_to) noexcept
{
    message = dbus_message_copy(UDBUS_GET_MESSAGE(reply_to));
}

void UDBus::Message::copy(DBusMessage* reply_to) noexcept
{
    message = dbus_message_copy(reply_to);
}

void UDBus::Message::ref(UDBus::Message& reply_to) noexcept
{
    message = dbus_message_ref(UDBUS_GET_MESSAGE(reply_to));
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

int UDBus::Message::get_type() noexcept
{
    return dbus_message_get_type(message);
}

const char *UDBus::Message::get_error_name() noexcept
{
    return dbus_message_get_error_name(message);
}

udbus_bool_t UDBus::Message::set_error_name(const char* name) noexcept
{
    return dbus_message_set_error_name(message, name);
}

void UDBus::Message::unref() noexcept
{
    if (message != nullptr)
        dbus_message_unref(message);
    message = nullptr;
}

udbus_bool_t UDBus::Message::is_method_call(const char* iface, const char* method) noexcept
{
    return dbus_message_is_method_call(message, iface, method);
}

udbus_bool_t UDBus::Message::is_valid() noexcept
{
    return message != nullptr;
}

udbus_bool_t UDBus::Message::is_signal(const char* iface, const char* method) noexcept
{
    return dbus_message_is_signal(message, iface, method);
}
