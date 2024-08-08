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

void UDBus::Message::endContainer(bool bWasInitial) noexcept
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

void UDBus::Message::handleVariants(UDBus::Iterator& current, int type, UDBus::Variant& data)
{
    if (current.get_arg_type() != DBUS_TYPE_VARIANT)
        throw std::runtime_error("Schema expected a variant, got: " + std::to_string(type));

    setupContainer(current);
    data.f(iteratorStack.back(), userPointer);
    endContainer(bInitialGet);
}