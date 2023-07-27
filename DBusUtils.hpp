#pragma once
#include <dbus/dbus.h>
#include <vector>
#include <cstdint>
#include <typeinfo>
#include <iostream>
#include <cstdarg>
#include <array>

#define GET_MESSAGE(x) *x.getMessagePointer()
#define GET_TYPE_ID(x) sizeof(x), typeid(x).hash_code()
#define SIMPLE_TYPE_UPPER_BOUNDARY 11

#define SIGNATURE(x) UDBus::Types::getTypeInfo<x>().signature.c_str()

namespace UDBus
{
    struct Type
    {
        size_t size;
        uint64_t id = 0;
        bool bSimple = true;
        std::string signature;

    };

    class Types
    {
    public:
        // Given a type(T1) and a variadic templated list of types, representing member types, generates and pushes a
        // Type struct to the global type registry to represent T1. This is designed for structs only
        template<typename T1, typename ...T2>
        static void generateStructSignature() noexcept
        {
            generateSignatureGeneric<T1, T2...>(DBUS_STRUCT_BEGIN_CHAR_AS_STRING, DBUS_STRUCT_END_CHAR_AS_STRING);
        }

        // Given a type(T1) and a variadic templated list of types, representing member types, generates and pushes a
        // Type struct to the global type registry to represent T1. This is designed for pairs and dictionary entries
        // only
        template<typename T1, typename ...T2>
        static void generateDictEntrySignature() noexcept
        {
            generateSignatureGeneric<T1, T2...>(DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING, DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
        }

        // Given a type, get the type information about it from the global type registry
        template<typename T>
        static const Type& getTypeInfo() noexcept
        {
            for (const auto& a : get().typesRegistry)
                if (a.id == typeid(T).hash_code())
                    return a;
            // TODO: Throw error here
        }

    private:
        friend class Message;

        template<typename T1, typename ...T2>
        static void generateSignatureGeneric(const char* begin, const char* end) noexcept
        {
            std::vector<uint64_t> ids = { typeid(T2).hash_code()... };
            std::string signature = begin;

            for (auto& a : ids)
            {
                for (auto& f : get().typesRegistry)
                {
                    if (f.id == a)
                    {
                        signature += f.signature;
                        goto exit_inner;
                    }
                }
        exit_inner:;
            }
            get().typesRegistry.emplace_back(GET_TYPE_ID(T1), false, signature + end);
        }

        std::vector<Type> typesRegistry =
        {
            Type{ GET_TYPE_ID(uint8_t),             true, DBUS_TYPE_BYTE_AS_STRING },
            Type{ GET_TYPE_ID(int8_t),             true, DBUS_TYPE_BYTE_AS_STRING },
            Type{ GET_TYPE_ID(dbus_uint32_t),       true, DBUS_TYPE_UINT32_AS_STRING },
            // uint32_t before bool because bool is an uint32, retarded af can confirm
            Type{ GET_TYPE_ID(dbus_bool_t),         true, DBUS_TYPE_BOOLEAN_AS_STRING },
            Type{ GET_TYPE_ID(dbus_int16_t),        true, DBUS_TYPE_INT16_AS_STRING },
            Type{ GET_TYPE_ID(dbus_uint16_t),       true, DBUS_TYPE_UINT16_AS_STRING },
            Type{ GET_TYPE_ID(dbus_int32_t),        true, DBUS_TYPE_INT32_AS_STRING },
            Type{ GET_TYPE_ID(dbus_int64_t),        true, DBUS_TYPE_INT64_AS_STRING },
            Type{ GET_TYPE_ID(dbus_uint64_t),       true, DBUS_TYPE_UINT64_AS_STRING },
            Type{ GET_TYPE_ID(float),              true, DBUS_TYPE_DOUBLE_AS_STRING },
            Type{ GET_TYPE_ID(double),              true, DBUS_TYPE_DOUBLE_AS_STRING },
            Type{ GET_TYPE_ID(const char*),         true, DBUS_TYPE_STRING_AS_STRING },
            Type{ GET_TYPE_ID(char*),               true, DBUS_TYPE_STRING_AS_STRING },

            // Array types
            Type{ GET_TYPE_ID(std::vector<uint8_t>),             true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<int8_t>),             true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<dbus_uint32_t>),       true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT32_AS_STRING },
            // uint32_t before bool because bool is an uint32, retarded af can confirm
            Type{ GET_TYPE_ID(std::vector<dbus_bool_t>),         true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BOOLEAN_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<dbus_int16_t>),        true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_INT16_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<dbus_uint16_t>),       true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT16_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<dbus_int32_t>),        true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_INT32_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<dbus_int64_t>),        true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_INT64_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<dbus_uint64_t>),       true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT64_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<float>),              true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_DOUBLE_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<double>),              true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_DOUBLE_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<const char*>),         true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING },
            Type{ GET_TYPE_ID(std::vector<char*>),               true, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING },
        };

        static Types& get() noexcept;
    };

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

        void demarshal(const char* str, int len, DBusError* error) noexcept;

        void pending_call_steal_reply(DBusPendingCall* pending) noexcept;

        /**
         * @brief A function that allows you to safely append simple types to dbus messages. The following types are
         * considered simple:
         * - int8_t
         * - uint8_t
         * - dbus_uint32_t
         * - dbus_bool_t - Do not use regular booleans as the protocol wants 4 bytes
         * - dbus_int16_t
         * - dbus_uint16_t
         * - dbus_int32_t
         * - dbus_int64_t
         * - dbus_uint64_t
         * - float
         * - double
         * - const char*
         * - char*
         *
         * As well as the following array types:
         * - std::vector<int8_t>
         * - std::vector<uint8_t>
         * - std::vector<dbus_uint32_t>
         * - std::vector<dbus_bool_t> - Do not use regular booleans as the protocol wants 4 bytes
         * - std::vector<dbus_int16_t>
         * - std::vector<dbus_uint16_t>
         * - std::vector<dbus_int32_t>
         * - std::vector<dbus_int64_t>
         * - std::vector<dbus_uint64_t>
         * - std::vector<float>
         * - std::vector<double>
         * - std::vector<const char*>
         * - std::vector<char*>
         *
         * @tparam T - A template varargs list, don't fill this manually, as the compiler will deduce it by you filling
         * args.
         * @param args - A vararg list of T*. Makes it so every argument passed is a pointer. Please make sure that
         * strings are also passed as pointers.
         */
        template<typename ...T>
        void append_args_simple(T*... args) noexcept
        {
            const auto standardTypeIDs = Types::get().typesRegistry;

            std::vector<uint64_t> typeIDs = { typeid(*args).hash_code()... };
            std::vector<void*> values = { (args)... };

            for (size_t i = 0; i < values.size(); i++)
            {
                const std::string* type;

                // Find our signature
                for (const auto& f : standardTypeIDs)
                {
                    if (typeIDs[i] == f.id)
                    {
                        type = &f.signature;
                        goto cont;
                    }
                }
            cont:
                // TODO: Handle error here
                // This will happen if a type was passed that does not exist
                if (type->empty())
                    continue;
                if (type->front() == DBUS_TYPE_ARRAY)
                {
                    if (type->length() > 2)
                    {
                        // TODO: Handle error here
                        return;
                    }

                    // String handling nightmares
                    std::vector<char>* normalVector;
                    std::vector<char*>* stringVector;

                    // A little story: basically a vector is whatever your standard library specifies, however it is
                    // certain that any vector has the same size as any other vector on your given system. This is
                    // because it stores a heap pointer for the data.
                    //
                    // Anyway, if we have a string we have to use an "std::vector<char*>*", since string arrays given
                    // to dbus have to be of type char***. If it's not a string then it's whatever, so we're using
                    // std::vector<char>* just to have something there.
                    if ((*type)[1] == DBUS_TYPE_STRING)
                    {
                        stringVector = (std::vector<char*>*)values[i];
                        char** dt = stringVector->data();

                        // We already assume a string so why not hardcode it
                        dbus_message_append_args(message, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &dt, stringVector->size(), DBUS_TYPE_INVALID);
                        return;
                    }
                    normalVector = (std::vector<char>*)values[i];
                    char* dt = normalVector->data();

                    dbus_message_append_args(message, DBUS_TYPE_ARRAY, (*type)[1], &dt, normalVector->size(), DBUS_TYPE_INVALID);
                }
                else
                {
                    DBusMessageIter iter;
                    dbus_message_iter_init_append(message, &iter);
                    dbus_message_iter_append_basic(&iter, type->front(), const_cast<const void*>(values[i]));
                }
            }
        }

        // Use this to pass to function arguments
        DBusMessage* get() noexcept;

        // Use this to assign to a function returning a raw dbus message pointer. It's preferred to use the
        // "GET_MESSAGE" macro, as it will make your code more concise and less syntax heavy
        DBusMessage** getMessagePointer() noexcept;

        ~Message() noexcept;
    private:

        DBusMessage* message = nullptr;
    };

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

        dbus_bool_t send(Message& message, dbus_uint32_t* client_serial) noexcept;
        dbus_bool_t send_with_reply(Message& message, PendingCall& pending_return, int timeout_milliseconds) noexcept;
        Message send_with_reply_and_block(Message& message, int timeout_mislliseconds, Error& error) noexcept;

        ~Connection() noexcept;
    private:
        DBusConnection* connection = nullptr;
    };

    // An abstraction on top of the regular low level iterator constructs. May be easier for some users to use, if they
    // want to be more low level and have more control over what they're pushing
    class Iterator
    {
    public:
        Iterator() = default;
        Iterator(Message& message, int type, const char* contained_signature, Iterator& it, bool bInit) noexcept;

        /**
         * @brief Initialises the iterator. It may be pre-initialised as a sub-iterator. This should only be used when
         * setting a sub-iterator. Appending arguments to the iterator, before calling this is perfectly normal.
         * @param message - A reference to the message to use the iterator on
         * @param type - The type of the iterator
         * @param contained_signature - The type signature of the sub-iterator. May be nullptr
         * @param it - A reference to a sub-iterator. The sub-iterator can be default-initialised beforehand
         * @param bInit - Whether to initialise the
         *
         */
        void set(Message& message, int type, const char* contained_signature, Iterator& it, bool bInit) noexcept;

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
        DBusMessageIter iterator{};
        Iterator* inner = nullptr;
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

        dbus_bool_t set_notify(DBusPendingCallNotifyFunction function, void* user_data, DBusFreeFunction free_user_data) noexcept;

        void cancel() noexcept;
        dbus_bool_t get_completed() noexcept;

        void block() noexcept;

        dbus_bool_t set_data(dbus_int32_t slot, void* data, DBusFreeFunction free_data_func) noexcept;
        void* get_data(dbus_int32_t slot) noexcept;

        ~PendingCall() noexcept;
    private:
        DBusPendingCall* pending = nullptr;

    };
}