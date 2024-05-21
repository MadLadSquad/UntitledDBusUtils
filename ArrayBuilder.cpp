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
    switch (manipulators)
    {
        case BeginStruct:
            f = WRAP_AS_PROCEDURE(Message::BeginStruct(builder.getMessage()));
            break;
        case EndStruct:
            f = WRAP_AS_PROCEDURE(Message::EndStruct(builder.getMessage()));
            break;
        case BeginArray:
            f = WRAP_AS_PROCEDURE(Message::BeginArray(builder.getMessage()));
            break;
        case EndArray:
            f = WRAP_AS_PROCEDURE(Message::EndArray(builder.getMessage()));
            break;
        case BeginVariant:
            f = WRAP_AS_PROCEDURE(Message::BeginVariant(builder.getMessage()));
            break;
        case EndVariant:
            f = WRAP_AS_PROCEDURE(Message::EndVariant(builder.getMessage()));
            break;
        default:
            break;
    }
    return builder;
}