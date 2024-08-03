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

void UDBus::Message::endContainer(bool& bWasInitial) noexcept
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