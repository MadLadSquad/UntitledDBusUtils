#include "DBusUtils.hpp"

UDBus::PendingCall::operator DBusPendingCall*() noexcept
{
    return pending;
}

void UDBus::PendingCall::ref(DBusPendingCall* p) noexcept
{
    pending = dbus_pending_call_ref(p);
}

void UDBus::PendingCall::ref(UDBus::PendingCall& p) noexcept
{
    pending = dbus_pending_call_ref(p);
}

void UDBus::PendingCall::unref() noexcept
{
    dbus_pending_call_unref(pending);
}

udbus_bool_t UDBus::PendingCall::set_notify(DBusPendingCallNotifyFunction function, void* user_data, DBusFreeFunction free_user_data) noexcept
{
    return dbus_pending_call_set_notify(pending, function, user_data, free_user_data);
}

void UDBus::PendingCall::cancel() noexcept
{
    dbus_pending_call_cancel(pending);
}

udbus_bool_t UDBus::PendingCall::get_completed() noexcept
{
    return dbus_pending_call_get_completed(pending);
}

void UDBus::PendingCall::block() noexcept
{
    return dbus_pending_call_block(pending);
}

udbus_bool_t UDBus::PendingCall::set_data(dbus_int32_t slot, void* data, DBusFreeFunction free_data_func) noexcept
{
    return dbus_pending_call_set_data(pending, slot, data, free_data_func);
}

void* UDBus::PendingCall::get_data(dbus_int32_t slot) noexcept
{
    return dbus_pending_call_get_data(pending, slot);
}

UDBus::PendingCall::~PendingCall() noexcept
{
    if (pending != nullptr)
        unref();
}

UDBus::PendingCall::operator DBusPendingCall**() noexcept
{
    return &pending;
}
