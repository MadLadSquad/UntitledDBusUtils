#include "DBusUtils.hpp"

void UDBus::Message::setupContainer(UDBus::Iterator& it) noexcept
{
    if (bInitialGet)
        it.recurse();
    else
    {
        auto& n = iteratorStack.emplace_back();
        it.setGet(*this, &n, false);
    }

    bInitialGet = false;
}

void UDBus::Message::endContainer(const bool bWasInitial) noexcept
{
    if (!bInitialGet)
        iteratorStack.pop_back();
    else if (bWasInitial)
        bInitialGet = true;
}

UDBus::Variant& UDBus::makeVariant(UDBus::Variant&& v) noexcept
{
    auto* tmp = new Variant{v};
    return *tmp;
}

UDBus::IgnoreType& UDBus::ignore() noexcept
{
    static IgnoreType i{};
    return i;
}

UDBus::BumpType& UDBus::bump() noexcept
{
    static BumpType i{};
    return i;
}

UDBus::MessageGetResult UDBus::Message::handleVariants(UDBus::Iterator& current, const UDBus::Variant& data) noexcept
{
    if (current.get_arg_type() != DBUS_TYPE_VARIANT)
        return RESULT_INVALID_VARIANT_TYPE;

    setupContainer(current);
    if (!data.parse(*this, iteratorStack.back(), const_cast<void**>(&data.data), userPointer))
        return RESULT_INVALID_VARIANT_PARSING;
    endContainer(bInitialGet);
    return RESULT_SUCCESS;
}