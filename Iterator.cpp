#include "DBusUtils.hpp"

UDBus::Iterator::~Iterator()
{
    std::cout << "Destroyed iterator" << std::endl;
    close_container();
}

UDBus::Iterator::operator DBusMessageIter*() noexcept
{
    return &iterator;
}

UDBus::Iterator::Iterator(UDBus::Message& message, int type, const char* contained_signature, UDBus::Iterator& it, bool bInit) noexcept
{
    setAppend(message, type, contained_signature, it, bInit);
}

bool UDBus::Iterator::append_basic(int type, const void* value) noexcept
{
    return iteratorType == APPEND_ITERATOR && dbus_message_iter_append_basic(&iterator, type, value);
}

bool UDBus::Iterator::append_fixed_array(int element_type, const void* value, int n_elements) noexcept
{
    return iteratorType == APPEND_ITERATOR && dbus_message_iter_append_fixed_array(&iterator, element_type, value, n_elements);
}

void UDBus::Iterator::close_container() noexcept
{
    if (inner != nullptr && iteratorType == APPEND_ITERATOR)
        dbus_message_iter_close_container(&iterator, &inner->iterator);
    inner = nullptr;
    iteratorType = EMPTY;
}

void UDBus::Iterator::abandon_container() noexcept
{
    dbus_message_iter_abandon_container(&iterator, &inner->iterator);
}

void UDBus::Iterator::abandon_container_if_open() noexcept
{
    dbus_message_iter_abandon_container_if_open(&iterator, &inner->iterator);
}

bool UDBus::Iterator::has_next() noexcept
{
    return dbus_message_iter_has_next(&iterator);
}

bool UDBus::Iterator::next() noexcept
{
    return dbus_message_iter_next(&iterator);
}

char* UDBus::Iterator::get_signature() noexcept
{
    return dbus_message_iter_get_signature(&iterator);
}

void UDBus::Iterator::setAppend(UDBus::Message& message, int type, const char* contained_signature, UDBus::Iterator& it, bool bInit) noexcept
{
    iteratorType = APPEND_ITERATOR;
    inner = &it;
    if (bInit)
        dbus_message_iter_init_append(message, &iterator);
    if (inner != nullptr)
        dbus_message_iter_open_container(&iterator, type, contained_signature, &inner->iterator);
}

int UDBus::Iterator::get_arg_type() noexcept
{
    return dbus_message_iter_get_arg_type(&iterator);
}

int UDBus::Iterator::get_element_type() noexcept
{
    return dbus_message_iter_get_element_type(&iterator);
}

void UDBus::Iterator::recurse() noexcept
{
    dbus_message_iter_recurse(&iterator, &inner->iterator);
}

void UDBus::Iterator::get_basic(void *value) noexcept
{
    dbus_message_iter_get_basic(&iterator, value);
}

int UDBus::Iterator::get_element_count() noexcept
{
    return dbus_message_iter_get_element_count(&iterator);
}

void UDBus::Iterator::get_fixed_array(void* value, int* n_elements) noexcept
{
    dbus_message_iter_get_fixed_array(&iterator, value, n_elements);
}

void UDBus::Iterator::setGet(UDBus::Message& message, Iterator* it, bool bInit) noexcept
{
    iteratorType = GET_ITERATOR;
    inner = it;
    if (bInit)
        dbus_message_iter_init(message, &iterator);
    else if (it != nullptr)
        recurse();
}
