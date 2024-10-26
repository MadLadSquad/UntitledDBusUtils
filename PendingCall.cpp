#include "DBusUtils.hpp"

UDBus::PendingCall::operator DBusPendingCall*() const noexcept
{
    return pending;
}

void UDBus::PendingCall::ref(DBusPendingCall* p) noexcept
{
    pending = dbus_pending_call_ref(p);
}

void UDBus::PendingCall::ref(const UDBus::PendingCall& p) noexcept
{
    pending = dbus_pending_call_ref(p);
}

void UDBus::PendingCall::unref() noexcept
{
    if (pending != nullptr)
        dbus_pending_call_unref(pending);
    pending = nullptr;
}

udbus_bool_t UDBus::PendingCall::set_notify(const DBusPendingCallNotifyFunction function, void* user_data, const DBusFreeFunction free_user_data) const noexcept
{
    return dbus_pending_call_set_notify(pending, function, user_data, free_user_data);
}

void UDBus::PendingCall::cancel() const noexcept
{
    dbus_pending_call_cancel(pending);
}

udbus_bool_t UDBus::PendingCall::get_completed() const noexcept
{
    return dbus_pending_call_get_completed(pending);
}

void UDBus::PendingCall::block() const noexcept
{
    return dbus_pending_call_block(pending);
}

udbus_bool_t UDBus::PendingCall::set_data(const dbus_int32_t slot, void* data, const DBusFreeFunction free_data_func) const noexcept
{
    return dbus_pending_call_set_data(pending, slot, data, free_data_func);
}

void* UDBus::PendingCall::get_data(const dbus_int32_t slot) const noexcept
{
    return dbus_pending_call_get_data(pending, slot);
}

UDBus::PendingCall::~PendingCall() noexcept
{
    unref();
}

UDBus::PendingCall::operator DBusPendingCall**() noexcept
{
    return &pending;
}
