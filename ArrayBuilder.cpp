#include "DBusUtils.hpp"


UDBus::ArrayBuilder::ArrayBuilder(UDBus::Message& msg) noexcept
{
    init(msg);
}

void UDBus::ArrayBuilder::init(UDBus::Message& msg) noexcept
{
    message = &msg;
}

UDBus::ArrayBuilder::~ArrayBuilder() noexcept
{
    message = nullptr;
}

UDBus::Message& UDBus::ArrayBuilder::getMessage() noexcept
{
    return *message;
}

#define WRAP_AS_PROCEDURE(x) [&]() -> void { x; }

UDBus::ArrayBuilder& UDBus::operator<<(UDBus::ArrayBuilder& builder, UDBus::MessageManipulators manipulators) noexcept
{
    std::function<void(void)> f = WRAP_AS_PROCEDURE();

    ArrayBuilder* tmpBuilderPointer = &builder; // Temporary pointer to the builder argument because lambda captures complicate the program flow
    std::string tmpVariantSignature = builder.variantSignature; // Temporary string that will be copied in the lambda
    switch (manipulators)
    {
        case BeginStruct:
            if (builder.bInitialising && !builder.bInVariant)
                builder.signature += DBUS_STRUCT_BEGIN_CHAR_AS_STRING;
            else if (builder.bInVariant)
                builder.variantSignature += DBUS_STRUCT_BEGIN_CHAR_AS_STRING;
            f = WRAP_AS_PROCEDURE(Message::BeginStruct(builder.getMessage()));
            break;
        case EndStruct:
            if (builder.bInitialising && !builder.bInVariant)
                builder.signature += DBUS_STRUCT_END_CHAR_AS_STRING;
            else if (builder.bInVariant)
                builder.variantSignature += DBUS_STRUCT_END_CHAR_AS_STRING;
            f = WRAP_AS_PROCEDURE(Message::EndStruct(builder.getMessage()));
            break;
        case BeginVariant:
            if (builder.bInitialising)
                builder.signature += DBUS_TYPE_VARIANT_AS_STRING;
            builder.bInVariant = true;
            // Pushing f() will make the last item the current size and insert will insert at the position before that index, so we have to add one
            // This is done because the first child callback is placed before the actual variant iterator creation callback. This results in a mix-up like this:
            // a (uivu) -> v a(uivu) where in reality it should be the opposite. The variant type should be pushed before the array iterator.
            builder.variantIndex = builder.eventList.size() + 1;
            f = WRAP_AS_PROCEDURE(Message::BeginVariant(builder.getMessage()));
            break;
        case EndVariant:
            builder.bInVariant = false;
            f = [tmpBuilderPointer, tmpVariantSignature]() -> void {
                Message::EndVariant(tmpBuilderPointer->getMessage(), tmpVariantSignature);
            };
            // Insert variant callback before the first child, fixing a mix-up, detailed in the above case's comments
            builder.eventList.insert(builder.eventList.begin() + static_cast<decltype(builder.eventList)::difference_type>(builder.variantIndex), f);
            builder.variantSignature.clear(); // Clear signature because we might have multiple variants
            return builder; // We've already inserted so we can now return
        default:
            break;
    }
    builder.eventList.push_back(f);
    return builder;
}

UDBus::ArrayBuilder& UDBus::operator<<(UDBus::ArrayBuilder& builder, ArrayBuilderManipulators manipulators) noexcept
{
    switch (manipulators)
    {
        case Next:
            builder.bInitialising = false;
            break;
        case BeginDictEntry:
            if (builder.bInitialising && !builder.bInVariant)
                builder.signature += DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
            builder.bInDictEntry = true;
            builder.eventList.emplace_back(WRAP_AS_PROCEDURE(Message::BeginStruct(*builder.message, DBUS_TYPE_DICT_ENTRY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING)));
            break;
        case EndDictEntry:
            if (builder.bInitialising && !builder.bInVariant)
                builder.signature += DBUS_DICT_ENTRY_END_CHAR_AS_STRING;
            builder.eventList.emplace_back(WRAP_AS_PROCEDURE(Message::EndStruct(*builder.message, DBUS_DICT_ENTRY_END_CHAR_AS_STRING)));
            break;
    }
    return builder;
}

// Pushing an array builder simply merges both arrays
template<>
void UDBus::ArrayBuilder::append<UDBus::ArrayBuilder>(const UDBus::ArrayBuilder& t) noexcept
{
    if (bInitialising && !bInVariant)
        signature += DBUS_TYPE_ARRAY_AS_STRING + t.signature;
    else if (bInDictEntry && bInVariant)
        variantSignature += DBUS_TYPE_ARRAY_AS_STRING + t.signature; // If we're in variant we need this signature to properly create the variant iterator

    eventList.emplace_back([this, t]() -> void {
        Message::pushToIteratorStack(*message, DBUS_TYPE_ARRAY, t.signature.c_str());
        for (auto& f : t.eventList)
            f();
        Message::handleClosingContainers(*message);
    });
}