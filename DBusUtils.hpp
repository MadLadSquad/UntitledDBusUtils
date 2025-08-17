/**
 * Hello everyone, I'm Stanislav Vasilev(Madman10K) and I welcome you to the worst library I have ever created.
 * Previously, I had some experience working with templates and designing wrappers around other libraries, however,
 * nothing really compares to what I've written here. I sure hope this doesn't land in history as my magnum opus, but
 * who knows :skull_emoji:
 *
 * I'm really sorry that you, for one reason or another, have to look at the internals of this godforsaken library.
 * The fact is that without extreme use of templates and systems that work around the user's input to the library and
 * the underlying dbus-1 C API, easy interaction with dbus using official channels without using Gnome or QT libraries
 * is simply impossible in C++. Of course, I can write my own library using some nice low level networking library,
 * but I decided to just go bananas here for absolutely no reason. I think that the reward is worth it, even if you,
 * the reader of this file may not agree with me. I completely understand and empathise with you and wish you well.
 *
 * May God be with you!
 */

#pragma once
#include "DBusUtilsMeta.hpp"
#include <stack>

#define UDBUS_GET_MESSAGE(x) *(x).getMessagePointer()

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

        [[nodiscard]] bool is_set() const noexcept;

        [[nodiscard]] const char* name() const noexcept;
        [[nodiscard]] const char* message() const noexcept;

        void free() noexcept;
        ~Error();
    private:
        DBusError error = {};
    };

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
         * @param it - A reference to the sub-iterator. Can be null if only trying to get basic data
         * @param bInit - Whether to initialise the iterator, this should only be done on the iterator layer
         */
        void setGet(Message& message, Iterator* it, bool bInit) noexcept;

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

        BeginVariant,
        EndVariant,

        BeginArray,
        Next,
        EndArray,

        BeginDictEntry,
        EndDictEntry,

        EndMessage
    };

    class MessageBuilder
    {
    public:
        MessageBuilder() noexcept = default;
        explicit MessageBuilder(Message& msg) noexcept;
        void setMessage(Message& msg) noexcept;

        template<typename T>
        MessageBuilder& operator<<(const T& t) noexcept
        {
            return append(t);
        }

        template<typename T>
        MessageBuilder& append(const T& t) noexcept
        {
            if (nodeStack.empty())
                nodeStack.push(&node);

            // To make string handling more robust we store all strings in our own custom temporary string array.
            // This allows for the pushing of temporary strings, reduces bugs related to lifetimes, and it makes the
            // underlying string handling logic significantly easier compared to other approaches.
            //
            // Due to pointer invalidation during the construction of the message we pass the index of the temporary
            // string, instead of a pointer. This is then cast back into the index in the callback function of the
            // appendGenericBasic function.
            if constexpr (Tag<T>::TypeString == DBUS_TYPE_STRING)
            {
                tempStrings.push_back(t);
                appendGenericBasic(DBUS_TYPE_STRING, (void*)(tempStrings.size() - 1));
            }
            else
                appendGenericBasic(Tag<T>::TypeString, (void*)&t);
            return *this;
        }

        template<typename T>
        MessageBuilder& append(const std::vector<T>& t) noexcept
        {
            if (nodeStack.empty())
                nodeStack.push(&node);

            // We have to pass a triple char pointer to dbus if we want to pass arrays of strings. Therefore, we check
            // for the type and if we have a string we do cast magic to get the char***. You don't want to even know how
            // previous revisions of that handled this. Here for some fun:
            // https://github.com/MadLadSquad/UntitledDBusUtils/blob/90b4afc2e66bb28a72c211f165c58c8f2687bc88/DBusUtils.hpp#L269
            if constexpr (Tag<T>::TypeString == DBUS_TYPE_STRING)
            {
                const auto f = (void**)t.data();
                appendArrayBasic(Tag<T>::TypeString, (void*)f, t.size(), sizeof(T));
            }
            else
                appendArrayBasic(Tag<T>::TypeString, (void*)t.data(), t.size(), sizeof(T));
            return *this;
        }

    private:
        Message* message = nullptr;

        void appendGenericBasic(char type, void* data) const noexcept;
        void appendArrayBasic(char type, void* data, size_t n, size_t size) const noexcept;

        void appendStructureEvent(char type, const char* containedSignature) const noexcept;

        void closeContainers() const noexcept;
        void endStructure() noexcept;

        std::vector<std::string> tempStrings{};

        struct AppendNode
        {
            std::vector<AppendNode> children{};
            std::function<void(AppendNode&)> event = [](AppendNode&) -> void {};
            std::string signature{};
            std::string innerSignature{};
            bool bIgnore = false;
        };

        static void sendMessage(AppendNode& node) noexcept;
        static void getSignature(AppendNode& node, std::string& signature) noexcept;

        AppendNode node{};
        std::stack<AppendNode*> nodeStack;
        size_t layerDepth = 0;
    };
    template<> MessageBuilder& MessageBuilder::append<MessageManipulators>(const MessageManipulators& op) noexcept;

    enum MessageGetResult
    {
        RESULT_SUCCESS,

        RESULT_NOT_CALLED,

        RESULT_MORE_FIELDS_THAN_REQUIRED,
        RESULT_LESS_FIELDS_THAN_REQUIRED,

        RESULT_INVALID_BASIC_TYPE,
        RESULT_INVALID_STRUCT_TYPE,
        RESULT_INVALID_ARRAY_TYPE,
        RESULT_INVALID_DICTIONARY_TYPE,
        RESULT_INVALID_DICTIONARY_KEY,
        RESULT_INVALID_VARIANT_TYPE,
        RESULT_INVALID_VARIANT_PARSING,
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

        operator DBusMessage*() const noexcept;

        void new_1(int messageType) noexcept;

        void new_method_call(const char* bus_name, const char* path, const char* interface, const char* func) noexcept;

        void new_method_return(Message& method_call) noexcept;
        void new_method_return(DBusMessage* method_call) noexcept;
        static Message new_method_return_s(Message& method_call) noexcept;
        static Message new_method_return_s(DBusMessage* method_call) noexcept;

        template<typename T, typename... T2>
        MessageGetResult handleMethodCall(const char* interface, const char* method, Type<T, T2...>& t) noexcept
        {
            if (is_method_call(interface, method))
                return handleMessage(t);
            return RESULT_NOT_CALLED;
        }

        template<typename T, typename... T2>
        MessageGetResult handleSignal(const char* interface, const char* method, Type<T, T2...>& t) noexcept
        {
            if (is_signal(interface, method))
                return handleMessage(t);
            return RESULT_NOT_CALLED;
        }

        void new_signal(const char* path, const char* interface, const char* name) noexcept;

        void new_error(Message& reply_to, const char* error_name, const char* error_message) noexcept;
        void new_error_raw(DBusMessage* reply_to, const char* error_name, const char* error_message) noexcept;

        void copy(Message& reply_to) noexcept;
        void copy(const DBusMessage* reply_to) noexcept;

        void ref(Message& reply_to) noexcept;
        void ref(DBusMessage* reply_to) noexcept;

        void unref() noexcept;

        void demarshal(const char* str, int len, DBusError* error) noexcept;

        void pending_call_steal_reply(DBusPendingCall* pending) noexcept;

        [[nodiscard]] udbus_bool_t is_valid() const noexcept;

        udbus_bool_t is_method_call(const char* iface, const char* method) const noexcept;
        udbus_bool_t is_signal(const char* iface, const char* method) const noexcept;

        [[nodiscard]] int get_type() const noexcept;

        [[nodiscard]] const char* get_error_name() const noexcept;
        udbus_bool_t set_error_name(const char* name) const noexcept;

        // Use this to pass to function arguments
        [[nodiscard]] DBusMessage* get() const noexcept;

        // Use this to assign to a function returning a raw dbus message pointer. It's preferred to use the
        // "UDBUS_GET_MESSAGE" macro, as it will make your code more concise and less syntax heavy
        DBusMessage** getMessagePointer() noexcept;

        void setUserPointer(void* ptr) noexcept;

        template<typename T, typename... T2>
        MessageGetResult handleMessage(Type<T, T2...>& t, UDBus::Iterator* iterator = nullptr) noexcept
        {
            if (iterator == nullptr)
            {
                iteratorStack.clear();
                bInitialGet = true;

                auto& latest = iteratorStack.emplace_back();
                auto& last = iteratorStack.emplace_back();
                latest.setGet(*this, &last, true);
            }

            auto result = handleMethodCallInternal(t);

            if (iterator == nullptr)
            {
                iteratorStack.clear();
                variantStack.clear();
                bInitialGet = true;
            }

            return result;
        }

        ~Message() noexcept;
    private:
        friend class MessageBuilder;

        DBusMessage* message = nullptr;

        std::deque<Iterator> iteratorStack{};
        // Contains variant structs that will be called for an array of dictionary of variants.
        std::deque<Variant*> variantStack{};

        void setupContainer(Iterator& it) noexcept;
        void endContainer(bool bWasInitial) noexcept;

        void* userPointer = nullptr;

        DECLARE_TYPE_AND_STRUCT(void, allocateArrayElements, t, {
            if constexpr (is_specialisation_of<UDBus::Struct, TT>{})
            {
                t.data = new TT{};
                allocateArrayElementsStruct(*t.data);
                t.data->bShouldBeFreed = true;
            }
            else
                t.data = new TT{};

            if (t.next() != nullptr)
                allocateArrayElements(*t.next());
        })

#define CHECK_SUCCESS(x) auto result = x; if (result != RESULT_SUCCESS) return result

        template<typename TT>
        MessageGetResult routeType(TT& t, UDBus::Iterator& it, int type, bool& bWasInitial, bool bAllocateStructs, const bool bPopVariant) noexcept
        {
            if constexpr (UDBus::is_specialisation_of<UDBus::Struct, TT>{})
            {
                CHECK_SUCCESS(handleStructType(it, type, t, bWasInitial, bAllocateStructs));
            }
            else if constexpr (std::is_same_v<Variant, TT>)
            {
                if (bPopVariant)
                {
                    CHECK_SUCCESS(handleVariants(it, *variantStack.back()));
                    // Don't forget that variant templates just store a template so we have to make a copy every time
                    // if we want our data to persist
                    t = *variantStack.back();
                }
                else
                {
                    CHECK_SUCCESS(handleVariants(it, t));
                }
            }
            else if constexpr (is_array_type<TT>())
            {
                CHECK_SUCCESS(handleArray(it, type, t, bWasInitial));
            }
            else if constexpr (is_map_type<TT>())
            {
                CHECK_SUCCESS(handleDictionaries(it, type, t, bWasInitial));
            }
            else if constexpr (!std::is_same_v<IgnoreType, TT> && !std::is_same_v<BumpType, TT>)
            {
                CHECK_SUCCESS(handleBasicType<TT>(it, type, &t));
            }

            if constexpr (UDBus::is_specialisation_of<ContainerVariantTemplate, TT>{})
                if (bPopVariant)
                    variantStack.pop_back();

            return RESULT_SUCCESS;
        }

#define HANDLE_CONTAINER_VARIANT_TEMPLATES(x, itt, typee, bWasInitiall, bAllocateStructs, bPopVariant)  \
if constexpr (UDBus::is_specialisation_of<ContainerVariantTemplate, TT>{})                              \
{                                                                                                       \
    auto& p = x;                                                                                        \
    variantStack.emplace_back((x).v);                                                                   \
    CHECK_SUCCESS(routeType(x->t, itt, typee, bWasInitiall, bAllocateStructs, bPopVariant));            \
}                                                                                                       \
else                                                                                                    \
{                                                                                                       \
    CHECK_SUCCESS(routeType(x, itt, typee, bWasInitiall, bAllocateStructs, bPopVariant));               \
}

        DECLARE_TYPE_AND_STRUCT(MessageGetResult, handleMethodCallInternal, t, {
            Iterator* it = &(*iteratorStack.rbegin());
            bool bWasInitial = false;
            if (bInitialGet)
            {
                bWasInitial = true;
                // Element before the last element
                it = &(*(iteratorStack.rbegin() + 1));
            }

            auto type = it->get_arg_type();
            HANDLE_CONTAINER_VARIANT_TEMPLATES(*t.data, *it, type, bWasInitial, false, false)

            if (it->next())
            {
                if (t.next() != nullptr)
                {
                    CHECK_SUCCESS(handleMethodCallInternal(*t.next()));
                }
                else if (!std::is_same<BumpType, TT>::value)
                    return RESULT_MORE_FIELDS_THAN_REQUIRED;
            }
            // Return this error if all of these evaluate to true:
            // 1. There is no data in the data stream but there is data in the structure
            // 2. The item is not a bump type
            // 3. The next item is not a bump type with checks for both Type<BumpType> and Struct<BumpType>
            //
            // Fun fact: it might seem more intuitive to dereference next as in `decltype(t.next())` and then checking
            // against Type<BumpType>/Struct<BumpType>, however, next is market as a constexpr function, which means
            // that since it returns nullptr the resulting type will not be any of the 2 expected types, but rather
            // std::nullptr_t. This took me 1.5 hours to debug... If only I were paid to develop this project...
            else if (t.next() != nullptr && !std::is_same<BumpType, TT>::value)
                if constexpr (!std::is_same<decltype(t.next()), Type<BumpType>*>::value && !std::is_same<decltype(t.next()), Struct<BumpType>*>::value)
                    return RESULT_LESS_FIELDS_THAN_REQUIRED;

            if constexpr (UDBus::is_specialisation_of<ContainerVariantTemplate, TT>{})
                variantStack.pop_back();
            return RESULT_SUCCESS;
        })

        template<typename T>
        MessageGetResult handleBasicType(Iterator& it, int type, void* data) noexcept
        {
            // Make an exception for object paths as strings
            if (type == Tag<T>::TypeString || (type == DBUS_TYPE_OBJECT_PATH && Tag<T>::TypeString == DBUS_TYPE_STRING))
                it.get_basic((void*)data);
            else
                return RESULT_INVALID_BASIC_TYPE;
            return RESULT_SUCCESS;
        }

        template<typename T, typename ...T2>
        MessageGetResult handleStructType(Iterator& it, const int type, Struct<T, T2...>& s, const bool bWasInitial, const bool bAllocateArrayElements) noexcept
        {
            if (type == DBUS_TYPE_STRUCT)
            {
                setupContainer(it);
                if (bAllocateArrayElements)
                {
                    allocateArrayElementsStruct(s);
                    s.bIsOrigin = true;
                }

                CHECK_SUCCESS(handleMethodCallInternalStruct(s));
                endContainer(bWasInitial);
            }
            else
                return RESULT_INVALID_STRUCT_TYPE;
            return RESULT_SUCCESS;
        }

        template<typename TT>
        MessageGetResult handleArray(Iterator& it, const int type, TT& t, const bool bWasInitial) noexcept
        {
            if (type != DBUS_TYPE_ARRAY)
                return RESULT_INVALID_ARRAY_TYPE;
            setupContainer(it);
            while (iteratorStack.back().get_arg_type() != DBUS_TYPE_INVALID)
            {
                auto& current = iteratorStack.back();
                auto& el = t.emplace_back();
                bool tmp = false;

                HANDLE_CONTAINER_VARIANT_TEMPLATES(el, current, current.get_arg_type(), tmp, true, true);

                current.next();
            }
            endContainer(bWasInitial);
            return RESULT_SUCCESS;
        }

        template<typename TT>
        MessageGetResult handleDictionaries(Iterator& it, const int type, TT& t, const bool bWasInitial) noexcept
        {
            if (type != DBUS_TYPE_ARRAY)
                return RESULT_INVALID_DICTIONARY_TYPE;
            setupContainer(it);
            while (iteratorStack.back().get_arg_type() != DBUS_TYPE_INVALID)
            {
                auto& current = iteratorStack.back();
                setupContainer(current);
                auto& nit = iteratorStack.back();
                bool tmp = false;

                const auto& el = t.emplace();
                if constexpr (is_complete<Tag<typename TT::key_type>>{} && !is_array_type<typename TT::key_type>())
                {
                    if (!std::is_same_v<IgnoreType, TT> && !std::is_same_v<BumpType, TT>)
                    {
                        // Keep the C-style cast because the C++ solution requires 2 lines worth of casts to compile without issues
                        CHECK_SUCCESS(handleBasicType<typename TT::key_type>(nit, nit.get_arg_type(), (void*)&el.first->first));
                    }
                }
                else
                    return RESULT_INVALID_DICTIONARY_KEY;

                nit.next(); // Move to the value

                HANDLE_CONTAINER_VARIANT_TEMPLATES(el.first->second, nit, nit.get_arg_type(), tmp, true, true);

                endContainer(false);
                current.next();
            }
            endContainer(bWasInitial);
            return RESULT_SUCCESS;
        }

        MessageGetResult handleVariants(Iterator& current, const Variant& data) noexcept;

        bool bInitialGet = true;
    };

    class PendingCall;

    class Connection
    {
    public:
        Connection() = default;
        explicit Connection(DBusConnection* conn) noexcept;

        operator DBusConnection*() const noexcept;

        void bus_get(DBusBusType type, Error& error) noexcept;
        void bus_get_private(DBusBusType type, Error& error) noexcept;

        [[nodiscard]] int request_name(const char* name, unsigned int flags, Error& error) const noexcept;

        [[nodiscard]] udbus_bool_t read_write(int timeout_milliseconds) const noexcept;
        [[nodiscard]] udbus_bool_t read_write_dispatch(int timeout_milliseconds) const noexcept;

        [[nodiscard]] Message pop_message() const noexcept;

        void open(const char* address, Error& error) noexcept;
        void open_private(const char* address, Error& error) noexcept;

        void ref(const Connection& conn) noexcept;
        void ref(DBusConnection* conn) noexcept;

        void unref() const noexcept;
        void close() const noexcept;

        void flush() const noexcept;

        udbus_bool_t send(Message& message, dbus_uint32_t* client_serial) const noexcept;
        udbus_bool_t send_with_reply(Message& message, PendingCall& pending_return, int timeout_milliseconds) const noexcept;
        Message send_with_reply_and_block(Message& message, int timeout_milliseconds, Error& error) const noexcept;

        ~Connection() noexcept;
    private:
        DBusConnection* connection = nullptr;
    };

    class PendingCall
    {
    public:
        PendingCall() = default;

        operator DBusPendingCall*() const noexcept;
        operator DBusPendingCall**() noexcept;

        void ref(DBusPendingCall* p) noexcept;
        void ref(const PendingCall& p) noexcept;

        void unref() noexcept;

        udbus_bool_t set_notify(DBusPendingCallNotifyFunction function, void* user_data, DBusFreeFunction free_user_data) const noexcept;

        void cancel() const noexcept;
        [[nodiscard]] udbus_bool_t get_completed() const noexcept;

        void block() const noexcept;

        udbus_bool_t set_data(dbus_int32_t slot, void* data, DBusFreeFunction free_data_func) const noexcept;
        [[nodiscard]] void* get_data(dbus_int32_t slot) const noexcept;

        ~PendingCall() noexcept;
    private:
        DBusPendingCall* pending = nullptr;

    };
}
