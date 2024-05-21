/**
 * Hello everyone, I'm Stanislav Vasilev(Madman10K) and I welcome you to the worst library I have ever created.
 * Previously, I had some experience working with templates and designing wrappers around other libraries, however,
 * nothing really compares to what I've written here. I sure hope this doesn't land in history as my magnum opus, but
 * who knows, I might even land on reddit :skull_emoji:
 *
 * I'm really sorry that you, for one reason or another, have to look at the internals of this god forsaken library.
 * The fact is that without extreme use of templates and systems that work around the user's input to the library and
 * the underlying dbus-1 C API, easy interaction with dbus using official channels without using Gnome or QT libraries
 * is simply impossible in C++. I think that the reward is worth it, even if you, the reader of this file may not agree
 * with me. I completely understand and empathise with you and wish you well on your endeavours. May God be with you!
 */

#pragma once
#include <dbus/dbus.h>
#include <vector>
#include <cstdint>
#include <typeinfo>
#include <iostream>
#include <cstdarg>
#include <array>
#include <deque>
#include <functional>

#define UDBUS_GET_MESSAGE(x) *(x).getMessagePointer()

// Use this instead of dbus_bool_t
struct udbus_bool_t
{
    udbus_bool_t(dbus_bool_t dbus) noexcept;
    dbus_bool_t b;
};

namespace UDBus
{
    class Error
    {
    public:
        Error();
        explicit Error(const DBusError& err) noexcept;

        operator DBusError*() noexcept;

        void set(const char* name, const char* message) noexcept;

        static void move(DBusError* src, DBusError* dest) noexcept;
        static void move(Error& src, Error& dest) noexcept;

        bool has_name(const char* name) const noexcept;

        bool is_set() noexcept;

        [[nodiscard]] const char* name() const noexcept;
        [[nodiscard]] const char* message() const noexcept;

        void free() noexcept;
        ~Error();
    private:
        DBusError error = {};
    };

    // Go around the C++ type system using Tag dispatching. This struct will have specialisations for all types we
    // want to support. Check the macro below
    template<typename T>
    struct Tag;

    // Specialises the Tag struct with a type definition and a constexpr string that represents the type
    #define MAKE_FUNC_BASIC(x, y) template<> struct Tag<x> { using Type = x; static constexpr char TypeString = y; }

    MAKE_FUNC_BASIC(const char*,                    DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(char*,                          DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(int8_t,                         DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(uint8_t,                        DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(dbus_int16_t,                   DBUS_TYPE_INT16     );
    MAKE_FUNC_BASIC(dbus_uint16_t,                  DBUS_TYPE_UINT16    );
    MAKE_FUNC_BASIC(dbus_int32_t,                   DBUS_TYPE_INT32     );
    MAKE_FUNC_BASIC(dbus_uint32_t,                  DBUS_TYPE_UINT32    );
    // uint32_t before bool because bool is an uint32. Retarded af fr
    MAKE_FUNC_BASIC(udbus_bool_t,                   DBUS_TYPE_BOOLEAN   );
    MAKE_FUNC_BASIC(dbus_int64_t,                   DBUS_TYPE_INT64     );
    MAKE_FUNC_BASIC(dbus_uint64_t,                  DBUS_TYPE_UINT64    );

    MAKE_FUNC_BASIC(std::vector<const char*>,       DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(std::vector<char*>,             DBUS_TYPE_STRING    );
    MAKE_FUNC_BASIC(std::vector<int8_t>,            DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(std::vector<uint8_t>,           DBUS_TYPE_BYTE      );
    MAKE_FUNC_BASIC(std::vector<dbus_int16_t>,      DBUS_TYPE_INT16     );
    MAKE_FUNC_BASIC(std::vector<dbus_uint16_t>,     DBUS_TYPE_UINT16    );
    MAKE_FUNC_BASIC(std::vector<dbus_int32_t>,      DBUS_TYPE_INT32     );
    MAKE_FUNC_BASIC(std::vector<dbus_uint32_t>,     DBUS_TYPE_UINT32    );
    // uint32_t before bool again. I hate this fucking library
    MAKE_FUNC_BASIC(std::vector<udbus_bool_t>,      DBUS_TYPE_BOOLEAN   );
    MAKE_FUNC_BASIC(std::vector<dbus_int64_t>,      DBUS_TYPE_INT64     );
    MAKE_FUNC_BASIC(std::vector<dbus_uint64_t>,     DBUS_TYPE_UINT64    );

    class Message;

    // An abstraction on top of the regular low level iterator constructs. May be easier for some users to use, if they
    // want to be more low level and have more control over what they're pushing
    class Iterator
    {
    public:
        Iterator() = default;
        Iterator(Message& message, int type, const char* contained_signature, Iterator& it, bool bInit) noexcept;

        /**
         * @brief Initialises the iterator for appending data. It may be pre-initialised as a sub-iterator. This should
         * only be used when setting a sub-iterator. Appending arguments to the iterator, before calling this is
         * perfectly normal.
         * @param message - A reference to the message to use the iterator on
         * @param type - The type of the iterator
         * @param contained_signature - The type signature of the sub-iterator. May be nullptr
         * @param it - A reference to a sub-iterator. The sub-iterator can be default-initialised beforehand
         * @param bInit - Whether to initialise the
         *
         */
        void setAppend(Message& message, int type, const char* contained_signature, Iterator& it, bool bInit) noexcept;

        /**
         * @brief Initialises the iterator
         * @param message - A reference to the message to be used on the iterator
         * @param it - A reference to the sub-iterator
         * @param bInit - Whether to initialise the iterator, this should only be done on the iterator layer
         */
        void setGet(Message& message, Iterator& it, bool bInit) noexcept;

        operator DBusMessageIter*() noexcept;

        bool append_basic(int type, const void* value) noexcept;
        bool append_fixed_array(int element_type, const void* value, int n_elements) noexcept;

        void close_container() noexcept;
        void abandon_container() noexcept;
        void abandon_container_if_open() noexcept;

        bool has_next() noexcept;
        bool next() noexcept;

        char* get_signature() noexcept;
        int get_arg_type() noexcept;
        int get_element_type() noexcept;

        void recurse() noexcept;

        void get_basic(void* value) noexcept;
        int get_element_count() noexcept;
        void get_fixed_array(void* value, int* n_elements) noexcept;

        ~Iterator();
    private:
        enum IteratorType
        {
            APPEND_ITERATOR,
            GET_ITERATOR,
            EMPTY,
        };

        IteratorType iteratorType = EMPTY;

        DBusMessageIter iterator{};
        Iterator* inner = nullptr;
    };

    enum MessageManipulators
    {
        BeginStruct,
        EndStruct,
        BeginArray,
        EndArray,
        BeginVariant,
        EndVariant
    };

    enum ArrayBuilderManipulators
    {
        Next,
        Key,
        Value
    };

    class ArrayBuilder;

    // An abstraction on top of DBusMessage* to support RAII and make calls more concise. All functions that return a
    // DBusMessage* are also replicated here as member functions without the "dbus_message" prefix.
    //
    // Any functions with a postfix of "_raw" or "_1" are named so due to C++ rules on function overloading.
    // So-called "raw functions" simply take a raw libdbus-1 type, instead of our custom type.
    class Message
    {
    public:
        Message() = default;
        explicit Message(DBusMessage* msg) noexcept;

        operator DBusMessage*() noexcept;

        void new_1(int messageType) noexcept;

        void new_method_call(const char* bus_name, const char* path, const char* interface, const char* func) noexcept;

        void new_method_return(Message& method_call) noexcept;
        void new_method_return(DBusMessage* method_call) noexcept;

        void new_signal(const char* path, const char* interface, const char* name) noexcept;

        void new_error(Message& reply_to, const char* error_name, const char* error_message) noexcept;
        void new_error_raw(DBusMessage* reply_to, const char* error_name, const char* error_message) noexcept;

        void copy(Message& reply_to) noexcept;
        void copy(DBusMessage* reply_to) noexcept;

        void ref(Message& reply_to) noexcept;
        void ref(DBusMessage* reply_to) noexcept;

        void unref() noexcept;

        void demarshal(const char* str, int len, DBusError* error) noexcept;

        void pending_call_steal_reply(DBusPendingCall* pending) noexcept;

        int get_type() noexcept;

        // ostream style << operator. Simply calls append
        template<typename T>
        Message& operator<<(const T& t) noexcept
        {
            append(t);
            return *this;
        }

        const char* get_error_name() noexcept;
        udbus_bool_t set_error_name(const char* name) noexcept;

        // Use this to pass to function arguments
        DBusMessage* get() noexcept;

        // Use this to assign to a function returning a raw dbus message pointer. It's preferred to use the
        // "UDBUS_GET_MESSAGE" macro, as it will make your code more concise and less syntax heavy
        DBusMessage** getMessagePointer() noexcept;

        ~Message() noexcept;

        static Message& BeginStruct(Message& message) noexcept;
        static Message& EndStruct(Message& message) noexcept;

        static Message& BeginVariant(Message& message) noexcept;
        static Message& EndVariant(Message& message) noexcept;

        static Message& BeginArray(Message& message) noexcept;
        static Message& EndArray(Message& message) noexcept;

        template<typename T>
        void append(const T& t) noexcept
        {
            appendGenericBasic(Tag<T>::TypeString, (void*)&t);
        }

        template<typename T>
        void append(const std::vector<T>& t) noexcept
        {
            // We have to pass a triple char pointer to dbus if we want to pass arrays of strings. Therefore, we check
            // for the type and if we have a string we do cast magic to get the char***. You don't even know how
            // previous revisions of that handled this. Here for some fun:
            // https://github.com/MadLadSquad/UntitledDBusUtils/blob/90b4afc2e66bb28a72c211f165c58c8f2687bc88/DBusUtils.hpp#L269
            if constexpr (Tag<T>::TypeString == DBUS_TYPE_STRING)
            {
                void** f = (void**)t.data();
                appendArrayBasic(Tag<T>::TypeString, (void*)f, t.size(), sizeof(T));
            }
            else
                appendArrayBasic(Tag<T>::TypeString, (void*)t.data(), t.size(), sizeof(T));
        }
    private:
        friend class ArrayBuilder;

        void appendGenericBasic(char type, void* data) noexcept;
        void appendArrayBasic(char type, void* data, size_t size, size_t typeSize) noexcept;

        static void pushToIteratorStack(Message& message, char type, const char* containedSignature) noexcept;
        static void handleContainerTypeWithInnerSignature(Message& message, const char* type, const std::function<void(void)>& f) noexcept;
        static void handleClosingContainers(Message& message) noexcept;

        DBusMessage* message = nullptr;

        std::deque<Iterator> iteratorStack;
        size_t signatureAccumulationDepth = 0;
        std::deque<std::pair<std::string, std::vector<std::function<void(void)>>>> eventList; // Used by containers that require a type signature, like arrays and variants
    };

    // Handle array builders because they're special :/
    template<>
    void Message::append<UDBus::ArrayBuilder>(const UDBus::ArrayBuilder& t) noexcept;

    Message& operator<<(Message& message, MessageManipulators manipulators) noexcept;

    // A class that is used to build a complex array such as an array of structs or a dictionary
    class ArrayBuilder
    {
    public:
        ArrayBuilder() = default;
        explicit ArrayBuilder(Message& msg) noexcept;

        void init(Message& msg) noexcept;

        template<typename T>
        ArrayBuilder& operator<<(const T& t) noexcept
        {
            append(t);
            return *this;
        }

        template<typename T>
        void append(const T& t) noexcept
        {

        }

        Message& getMessage() noexcept;

        ~ArrayBuilder() noexcept;
    private:
        friend class Message;

        Message* message = nullptr;

        std::deque<Iterator> iteratorStack;
        size_t signatureAccumulationDepth = 0;
        std::deque<std::pair<std::string, std::vector<std::function<void(void)>>>> eventList; // Used by containers that require a type signature, like arrays and variants
    };

    ArrayBuilder& operator<<(ArrayBuilder& message, MessageManipulators manipulators) noexcept;

    class PendingCall;

    class Connection
    {
    public:
        Connection() = default;
        explicit Connection(DBusConnection* conn) noexcept;

        operator DBusConnection*() noexcept;

        void bus_get(DBusBusType type, Error& error) noexcept;
        void bus_get_private(DBusBusType type, Error& error) noexcept;

        void open(const char* address, Error& error) noexcept;
        void open_private(const char* address, Error& error) noexcept;

        void ref(Connection& conn) noexcept;
        void ref(DBusConnection* conn) noexcept;

        void unref() noexcept;
        void close() noexcept;

        void flush() noexcept;

        udbus_bool_t send(Message& message, dbus_uint32_t* client_serial) noexcept;
        udbus_bool_t send_with_reply(Message& message, PendingCall& pending_return, int timeout_milliseconds) noexcept;
        Message send_with_reply_and_block(Message& message, int timeout_mislliseconds, Error& error) noexcept;

        ~Connection() noexcept;
    private:
        DBusConnection* connection = nullptr;
    };

    class PendingCall
    {
    public:
        PendingCall() = default;

        operator DBusPendingCall*() noexcept;
        operator DBusPendingCall**() noexcept;

        void ref(DBusPendingCall* p) noexcept;
        void ref(PendingCall& p) noexcept;

        void unref() noexcept;

        udbus_bool_t set_notify(DBusPendingCallNotifyFunction function, void* user_data, DBusFreeFunction free_user_data) noexcept;

        void cancel() noexcept;
        udbus_bool_t get_completed() noexcept;

        void block() noexcept;

        udbus_bool_t set_data(dbus_int32_t slot, void* data, DBusFreeFunction free_data_func) noexcept;
        void* get_data(dbus_int32_t slot) noexcept;

        ~PendingCall() noexcept;
    private:
        DBusPendingCall* pending = nullptr;

    };
}