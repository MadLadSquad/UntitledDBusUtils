#include "DBusUtils.hpp"

UDBus::MessageBuilder::MessageBuilder(Message& msg) noexcept
{
    setMessage(msg);
}

void UDBus::MessageBuilder::setMessage(Message& msg) noexcept
{
    this->message = &msg;
    if (nodeStack.empty())
        nodeStack.push(&node);
}

void UDBus::MessageBuilder::appendGenericBasic(char type, void* data) const noexcept
{
    const auto f = [type, data, this](const AppendNode&) -> void
    {
        // If the iterator stack is empty it means that we can freely append to a generic iterator
        if (message->iteratorStack.empty())
        {
            DBusMessageIter iter;
            dbus_message_iter_init_append(message->get(), &iter);
            dbus_message_iter_append_basic(&iter, type, data);
            return;
        }
        // Else append to the deepest iterator always
        message->iteratorStack.back().append_basic(type, data);
    };
    const char signature[2] = { type, '\0' };
    if (layerDepth > 0)
    {
        nodeStack.top()->children.emplace_back(AppendNode{
            .children = {},
            .event = f,
            .signature = signature
        });
    }
    else
        f(*nodeStack.top());
}

void UDBus::MessageBuilder::appendArrayBasic(char type, void* data, size_t n, size_t size) const noexcept
{
    const auto f = [type, data, n, size, this](const AppendNode&) -> void
    {
        // If the iterator stack is empty we can append the array directly
        if (message->iteratorStack.empty())
        {
            dbus_message_append_args(message->get(), DBUS_TYPE_ARRAY, type, &data, n, DBUS_TYPE_INVALID);
            return;
        }

        // Otherwise, get the parent iterator and create a child iterator
        auto& parent = message->iteratorStack.back();
        auto& child = message->iteratorStack.emplace_back();
        const char signature[] = { type, '\0' };

        // Assign the child to the parent
        parent.setAppend(*message, DBUS_TYPE_ARRAY, signature, child, false);
        for (size_t i = 0; i < n; i++)
        {
            // Evil pointer magic to iterate a typeless array without template arguments
            const auto tmp = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + (i * size));
            child.append_basic(type, tmp);
        }
        closeContainers();
    };

    const char signature[] = { DBUS_TYPE_ARRAY, type, '\0' };
    if (layerDepth > 0)
    {
        nodeStack.top()->children.emplace_back(AppendNode{
            .children = {},
            .event = f,
            .signature = signature
        });
    }
    else
        f(*nodeStack.top());
}

void UDBus::MessageBuilder::appendStructureEvent(const char type, const char* containedSignature) const noexcept
{
    bool bRootIterator = false;
    auto& stack = message->iteratorStack;
    if (stack.empty())
        bRootIterator = true;

    auto& parent = bRootIterator ? stack.emplace_back() : stack.back();
    auto& child = stack.emplace_back();

    parent.setAppend(*message, type, containedSignature, child, bRootIterator);
}

void UDBus::MessageBuilder::sendMessage(AppendNode& node) noexcept
{
    node.event(node);
    for (auto& a : node.children)
        sendMessage(a);
}

void UDBus::MessageBuilder::getSignature(AppendNode& node, std::string& signature) noexcept
{
    if (node.bIgnore && node.signature.empty())
        return;

    if (node.signature[0] == DBUS_TYPE_VARIANT)
        signature += DBUS_TYPE_VARIANT_AS_STRING;
    else if (node.signature == DBUS_TYPE_STRUCT_AS_STRING)
        signature += DBUS_STRUCT_BEGIN_CHAR_AS_STRING;
    else if (node.signature == DBUS_TYPE_DICT_ENTRY_AS_STRING)
        signature += DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
    else
        signature += node.signature;

    for (auto& a : node.children)
        getSignature(a, node.signature[0] == DBUS_TYPE_VARIANT ? node.innerSignature : signature);


    if (node.signature == DBUS_TYPE_STRUCT_AS_STRING)
        signature += DBUS_STRUCT_END_CHAR_AS_STRING;
    else if (node.signature == DBUS_TYPE_DICT_ENTRY_AS_STRING)
        signature += DBUS_DICT_ENTRY_END_CHAR_AS_STRING;
    else if (node.signature == DBUS_TYPE_ARRAY_AS_STRING)
        signature += node.innerSignature;
}

#define BEGIN_GENERIC_STRUCTURE(type, typeString, containedSignature)               \
    {                                                                               \
        auto& a = nodeStack.top()->children.emplace_back(AppendNode{                \
            .children = {},                                                         \
            .event = [this](const AppendNode& n) -> void                            \
            {                                                                       \
                appendStructureEvent(type, containedSignature);                     \
            },                                                                      \
            .signature = typeString,                                                \
        });                                                                         \
        nodeStack.push(&a);                                                         \
        layerDepth++;                                                               \
    }

template <>
UDBus::MessageBuilder& UDBus::MessageBuilder::append<UDBus::MessageManipulators>(const MessageManipulators& op) noexcept
{
    switch (op)
    {
    case BeginStruct:
        BEGIN_GENERIC_STRUCTURE(DBUS_TYPE_STRUCT, DBUS_TYPE_STRUCT_AS_STRING, nullptr);
        break;
    case EndStruct:
        endStructure();
        break;

    case BeginVariant:
        BEGIN_GENERIC_STRUCTURE(DBUS_TYPE_VARIANT, DBUS_TYPE_VARIANT_AS_STRING, n.innerSignature.c_str());
        break;
    case EndVariant:
        for (auto& a : nodeStack.top()->children)
            getSignature(a, nodeStack.top()->innerSignature);
        endStructure();
        break;

    case BeginArray:
        BEGIN_GENERIC_STRUCTURE(DBUS_TYPE_ARRAY, DBUS_TYPE_ARRAY_AS_STRING, n.innerSignature.c_str());
        break;
    case Next:
        nodeStack.top()->innerSignature.clear();
        break;
    case EndArray:
        for (auto& a : nodeStack.top()->children)
            getSignature(a, nodeStack.top()->innerSignature);
        endStructure();
        break;


    case BeginDictEntry:
        BEGIN_GENERIC_STRUCTURE(DBUS_TYPE_DICT_ENTRY, DBUS_TYPE_DICT_ENTRY_AS_STRING, nullptr);
        break;

    case EndDictEntry:
        endStructure();
        break;

    case EndMessage:
        sendMessage(node);
        break;
    default:
        break;
    }
    return *this;
}

void UDBus::MessageBuilder::closeContainers() const noexcept
{
    (message->iteratorStack.end() - 2)->close_container(); // Close parent first
    message->iteratorStack.pop_back(); // Pop child, this will close it too
    if (message->iteratorStack.size() == 1)
        message->iteratorStack.pop_back(); // Pop parent only if it's the last iterator
}

void UDBus::MessageBuilder::endStructure() noexcept
{
    nodeStack.top()->children.emplace_back(AppendNode
    {
        .children = {},
        .event = [this](const AppendNode&) -> void
        {
            closeContainers();
        },
        .signature = {},
        .bIgnore = true
    });
    nodeStack.pop();
    layerDepth--;
}