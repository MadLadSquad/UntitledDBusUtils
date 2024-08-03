#include "DBusUtils.hpp"


void UDBus::Message::appendGenericBasic(char type, void* data) noexcept
{
    const auto f = [this, type, data]() -> void {
        if (iteratorStack.empty())
        {
            DBusMessageIter iter;
            dbus_message_iter_init_append(message, &iter);
            dbus_message_iter_append_basic(&iter, type, data);
            return;
        }
        iteratorStack.back().append_basic(type, data);
    };
    char tmp[2] = { type, '\0' }; // We just get a char here :/

    if (signatureAccumulationDepth > 0)
        handleContainerTypeWithInnerSignature(*this, tmp, f);
    else
        f();
}

void UDBus::Message::appendArrayBasic(char type, void* data, size_t size, size_t typeSize) noexcept
{
    const auto f = [type, data, this, size, typeSize]() -> void {
        if (iteratorStack.empty())
        {
            dbus_message_append_args(message, DBUS_TYPE_ARRAY, type, &data, size, DBUS_TYPE_INVALID);
            return;
        }

        auto& parent = iteratorStack.back();  // Standard iterators bullshit
        auto& child = iteratorStack.emplace_back(); // Push child
        char signature[] = { type, '\0' };

        // Set append mode
        parent.setAppend(*this, DBUS_TYPE_ARRAY, signature, child, false);
        for (size_t i = 0; i < size; i++)
        {
            void* tmp = (void*)((intptr_t)data + (i * typeSize));   // Evil pointer magic
            child.append_basic(type, tmp);// Append
        }
        handleClosingContainers(*this);
    };
    char signature[] = { DBUS_TYPE_ARRAY, type, '\0' };
    if (signatureAccumulationDepth > 0)
        handleContainerTypeWithInnerSignature(*this, signature, f);
    else
        f();
}

UDBus::Message& UDBus::Message::BeginStruct(UDBus::Message& message, char type, const char* innerType) noexcept
{
    const auto f = [&message, type]() -> void {
        pushToIteratorStack(message, type, nullptr);
    };
    if (message.signatureAccumulationDepth > 0)
        handleContainerTypeWithInnerSignature(message, innerType, f);
    else
        f();
    return message;
}

UDBus::Message& UDBus::Message::EndStruct(UDBus::Message& message, const char* type) noexcept
{
    const auto f = [&message]() -> void {
        handleClosingContainers(message);
    };
    if (message.signatureAccumulationDepth > 0)
        handleContainerTypeWithInnerSignature(message, type, f);
    else
        f();
    return message;
}

UDBus::Message& UDBus::Message::BeginVariant(UDBus::Message& message) noexcept
{
    message.signatureAccumulationDepth++;   // Increase the depth of the variant because variants can be nested
    message.eventList.emplace_back();       // Add new events list
    return message;
}

UDBus::Message& UDBus::Message::EndVariant(UDBus::Message& message, std::string containedSignature) noexcept
{
    // We only get an empty signature if we're appending to the message directly(not using ArrayBuilder)
    if (containedSignature.empty())
        containedSignature = message.eventList[message.signatureAccumulationDepth - 1].first;

    pushToIteratorStack(message, DBUS_TYPE_VARIANT, containedSignature.c_str());
    for (auto& a : message.eventList[message.signatureAccumulationDepth - 1].second)
        a();

    message.signatureAccumulationDepth--;
    handleClosingContainers(message);
    message.eventList.pop_back();
    return message;
}

void UDBus::Message::pushToIteratorStack(UDBus::Message& message, char type, const char* containedSignature) noexcept
{
    // The following 3 lines do the following:
    // We create a boolean variable that is passed to setAppend and controls whether the container is initialised or
    // simply opened. Types that require a contained_signature should always be opened, since their container is already
    // a root iterator in all cases, so we set it to false in that case directly.
    //
    // We make it true, only when the container doesn't require a contained signature and is the iterator is added to
    // the root of the dbus arguments tree. This mostly happens when appending structs to the root of the message call.
    bool bRootIterator = false;
    if (message.iteratorStack.empty())
        bRootIterator = true;

    auto& parent = bRootIterator ? message.iteratorStack.emplace_back() : message.iteratorStack.back();
    auto& child = message.iteratorStack.emplace_back();

    parent.setAppend(message, type, containedSignature, child, bRootIterator);
}

void UDBus::Message::handleContainerTypeWithInnerSignature(UDBus::Message& message, const char* type, const std::function<void(void)>& f) noexcept
{
    auto& ref = message.eventList[message.signatureAccumulationDepth - 1];
    ref.first += type;
    ref.second.emplace_back(f);
}

void UDBus::Message::handleClosingContainers(UDBus::Message& message) noexcept
{
    (message.iteratorStack.end() - 2)->close_container(); // Close parent first
    message.iteratorStack.pop_back(); // Pop child, this will close it too
    if (message.iteratorStack.size() == 1)
        message.iteratorStack.pop_back(); // Pop parent only if it's the last iterator
}

UDBus::Message& UDBus::operator<<(UDBus::Message& message, UDBus::MessageManipulators manipulators) noexcept
{
    switch (manipulators)
    {
        case BeginStruct:
            return Message::BeginStruct(message);
        case EndStruct:
            return Message::EndStruct(message);
        case BeginVariant:
            return Message::BeginVariant(message);
        case EndVariant:
            return Message::EndVariant(message);
        default:
            return message;
    }
}

template<>
void UDBus::Message::append<UDBus::ArrayBuilder>(const UDBus::ArrayBuilder& t) noexcept
{
    pushToIteratorStack(*this, DBUS_TYPE_ARRAY, t.signature.c_str());
    for (auto& a : t.eventList)
        a();
    handleClosingContainers(*this);
}