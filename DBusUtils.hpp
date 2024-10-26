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
#include "DBusUtilsMeta.hpp"

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
        EndVariant
    };

    enum ArrayBuilderManipulators
    {
        Next,
        BeginDictEntry,
        EndDictEntry
    };

    class ArrayBuilder;

    UDBus::ArrayBuilder& operator<<(UDBus::ArrayBuilder&, UDBus::ArrayBuilderManipulators) noexcept;

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

        // ostream style << operator. Simply calls append
        template<typename T>
        Message& operator<<(const T& t) noexcept
        {
            append(t);
            return *this;
        }

        [[nodiscard]] const char* get_error_name() const noexcept;
        udbus_bool_t set_error_name(const char* name) const noexcept;

        // Use this to pass to function arguments
        [[nodiscard]] DBusMessage* get() const noexcept;

        // Use this to assign to a function returning a raw dbus message pointer. It's preferred to use the
        // "UDBUS_GET_MESSAGE" macro, as it will make your code more concise and less syntax heavy
        DBusMessage** getMessagePointer() noexcept;

        ~Message() noexcept;

        // When the default arguments are overloaded it can be used to set up dict entries
        static Message& BeginStruct(Message& message, char type = DBUS_TYPE_STRUCT, const char* innerType = DBUS_STRUCT_BEGIN_CHAR_AS_STRING) noexcept;
        // When the default arguments are overloaded it can be used to set up dict entries
        static Message& EndStruct(Message& message, const char* innerType = DBUS_STRUCT_END_CHAR_AS_STRING) noexcept;

        static Message& BeginVariant(Message& message) noexcept;
        static Message& EndVariant(Message& message, std::string containedSignature = "") noexcept;

        template<typename T>
        void append(const T& t) noexcept
        {
            appendGenericBasic(Tag<T>::TypeString, (void*)&t);
        }

        template<typename T>
        void append(const std::vector<T>& t) noexcept
        {
            // We have to pass a triple char pointer to dbus if we want to pass arrays of strings. Therefore, we check
            // for the type and if we have a string we do cast magic to get the char***. You don't want to even know how
            // previous revisions of that handled this. Here for some fun:
            // https://github.com/MadLadSquad/UntitledDBusUtils/blob/90b4afc2e66bb28a72c211f165c58c8f2687bc88/DBusUtils.hpp#L269
            if constexpr (Tag<T>::TypeString == DBUS_TYPE_STRING)
            {
                auto f = static_cast<void**>(t.data());
                appendArrayBasic(Tag<T>::TypeString, static_cast<void*>(f), t.size(), sizeof(T));
            }
            else
                appendArrayBasic(Tag<T>::TypeString, static_cast<void*>(t.data()), t.size(), sizeof(T));
        }

        void setUserPointer(void* ptr) noexcept;

        template<typename T, typename... T2>
        MessageGetResult handleMessage(Type<T, T2...>& t) noexcept
        {
            iteratorStack.clear();
            bInitialGet = true;

            auto& latest = iteratorStack.emplace_back();
            auto& last = iteratorStack.emplace_back();

            latest.setGet(*this, &last, true);
            auto result = handleMethodCallInternal(t);

            iteratorStack.clear();
            variantStack.clear();
            bInitialGet = true;

            return result;
        }
    private:
        friend class ArrayBuilder;
        friend UDBus::ArrayBuilder& UDBus::operator<<(UDBus::ArrayBuilder&, UDBus::ArrayBuilderManipulators) noexcept;

        void appendGenericBasic(char type, void* data) noexcept;
        void appendArrayBasic(char type, void* data, size_t size, size_t typeSize) noexcept;

        static void pushToIteratorStack(Message& message, char type, const char* containedSignature) noexcept;
        static void handleContainerTypeWithInnerSignature(Message& message, const char* type, const std::function<void(void)>& f) noexcept;
        static void handleClosingContainers(Message& message) noexcept;

        DBusMessage* message = nullptr;

        std::deque<Iterator> iteratorStack{};
        size_t signatureAccumulationDepth = 0;
        std::deque<std::pair<std::string, std::vector<std::function<void(void)>>>> eventList{}; // Used by containers that require a type signature, like arrays and variants

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

#define HANDLE_CONTAINER_VARIANT_TEMPLATES(x, itt, typee, bWasInitiall, bAllocateStructs, bPopVariant)    if constexpr (UDBus::is_specialisation_of<ContainerVariantTemplate, TT>{})    \
{                                                                                                                                                                                       \
    auto& p = x;                                                                                                                                                                        \
    variantStack.emplace_back((x).v);                                                                                                                                \
    CHECK_SUCCESS(routeType(x->t, itt, typee, bWasInitiall, bAllocateStructs, bPopVariant));                                                                                            \
}                                                                                                                                                                                       \
else                                                                                                                                                                                    \
{                                                                                                                                                                                       \
    CHECK_SUCCESS(routeType(x, itt, typee, bWasInitiall, bAllocateStructs, bPopVariant));                                                                                               \
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
            else if (t.next() != nullptr)
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
                    CHECK_SUCCESS(handleBasicType<typename TT::key_type>(nit, nit.get_arg_type(), static_cast<void*>(&el.first->first)));
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

    // Handle array builders because they're special :/
    template<>
    void Message::append<UDBus::ArrayBuilder>(const UDBus::ArrayBuilder& t) noexcept;

    Message& operator<<(Message& message, MessageManipulators manipulators) noexcept;

    // Manipulators for Array builder
    ArrayBuilder& operator<<(ArrayBuilder& message, MessageManipulators manipulators) noexcept;
    ArrayBuilder& operator<<(ArrayBuilder& builder, ArrayBuilderManipulators manipulators) noexcept;

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
            if (bInitialising && !bInVariant)
                signature += Tag<T>::TypeString;
            else if (bInVariant)
                variantSignature += Tag<T>::TypeString;

            eventList.emplace_back([this, t]() -> void {
                message->append(t);
            });
        }

        template<typename T>
        void append(const std::vector<T>& t) noexcept
        {
            if (bInitialising && !bInVariant)
                signature += DBUS_TYPE_ARRAY_AS_STRING + Tag<T>::TypeString;
            else if (bInVariant)
                variantSignature += DBUS_TYPE_ARRAY_AS_STRING + Tag<T>::TypeString;

            eventList.emplace_back([this, t]() -> void {
                message->append(t);
            });
        }

        [[nodiscard]] Message& getMessage() const noexcept;

        ~ArrayBuilder() noexcept;
    private:
        friend class Message;
        friend UDBus::ArrayBuilder& UDBus::operator<<(UDBus::ArrayBuilder&, UDBus::MessageManipulators) noexcept;
        friend UDBus::ArrayBuilder& UDBus::operator<<(UDBus::ArrayBuilder&, UDBus::ArrayBuilderManipulators) noexcept;

        bool bInitialising = true;  // This boolean allows us to automatically determine the type of the array from the first push to it
        bool bInVariant = false;    // Whether we're in variant type: accumulates the type signature in variantSignature when true
        bool bInDictEntry = false;  // Whether we're currently creating a dict entry. Important for proper instantiation of nested containers in arrays

        Message* message = nullptr;

        size_t variantIndex = 0; // Check comment in the BeginVariant case of << overload that implements MessageManipulators support
        std::string variantSignature; // Signature of the variant for creating its iterator, since variants require a contained signature string
        std::string signature;
        std::vector<std::function<void(void)>> eventList; // Used by containers that require a type signature, like arrays and variants
    };

    // Enables pushing array builders to array builders
    template<>
    void UDBus::ArrayBuilder::append<UDBus::ArrayBuilder>(const UDBus::ArrayBuilder& t) noexcept;

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