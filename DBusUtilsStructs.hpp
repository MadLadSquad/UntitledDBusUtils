// This file contains the code for the Type and Struct classes as well as other
// metaprogramming utilities
#pragma once
#include <type_traits>
#include <functional>

namespace UDBus
{
    // Casual forward declarations
    struct Variant;
    class Iterator;

    template<typename T>
    struct ContainerVariantTemplate;

    template<typename T, typename... T2>
    class Struct;

    // These macros are needed because I couldn't find a clean way of creating a function that has
    // the same body but supports getting both a Struct and a Type
    #define DECLARE_TYPE_AND_STRUCT_ADDITIONAL(return_type, name, arg_name, additional_arg, body)   \
    template<typename TT, typename... TT2>                                                          \
    return_type name##Struct(UDBus::Struct<TT, TT2...>& arg_name, additional_arg) noexcept body     \
                                                                                                    \
    template<typename TT, typename... TT2>                                                          \
    return_type name(UDBus::Type<TT, TT2...>& arg_name, additional_arg) noexcept body

    #define DECLARE_TYPE_AND_STRUCT(return_type, name, arg_name, body)                              \
    template<typename TT, typename... TT2>                                                          \
    return_type name##Struct(UDBus::Struct<TT, TT2...>& arg_name) noexcept body                     \
                                                                                                    \
    template<typename TT, typename... TT2>                                                          \
    return_type name(UDBus::Type<TT, TT2...>& arg_name) noexcept body

    struct IgnoreType
    {
        char data = 0;
    };

    // Use this if you don't want or need the information in a certain field of the message
    IgnoreType& ignore() noexcept;

    struct BumpType
    {
        char data = 0;
    };

    // Hack to support single-field messages
    BumpType& bump() noexcept;

// Simplifies the code a bit
#define UDBUS_FREE_TYPE(x) decltype(x)::destroy(x)

    // The Type compile-time tree which is used to build the schema
    template<typename T, typename... T2>
    class Type
    {
    public:
        Type() noexcept = default;
        Type(T& t, T2&... args) noexcept
        {
            init(t, args...);
        }

        void init(T& t, T2&... args) noexcept
        {
            data = &t;
            n = Type<T2...>(args...);
        }

        Type<T2...>* next() noexcept
        {
            return &n;
        }

        template<typename TT, typename... TT2>
        static void destroy(Type<TT, TT2...>& t) noexcept
        {
            bool bWasInArrayOrMap = false;
            bool bDestroyEverything = false;
            destroy_i(t, bWasInArrayOrMap, bDestroyEverything);
        }

        template<typename TT>
        static void destroyComplex(TT& t) noexcept
        {
            bool bWasInArrayOrMap = false;
            bool bDestroyEverything = false;
            routeDestroy(t, bWasInArrayOrMap, bDestroyEverything);
        }

        T* data = nullptr;
        Type<T2...> n{};
    private:
        template<typename TT>
        static void routeDestroy(TT& t, bool& bWasInArrayOrMap, bool& bDestroyEverything) noexcept
        {
            const bool bParentIsArrayOrMap = bWasInArrayOrMap;
            const bool bParentShouldDestroyEverything = bDestroyEverything;
            if constexpr (is_specialisation_of<Struct, TT>{})
            {
                if (t.bShouldBeFreed || t.bIsOrigin)
                    bDestroyEverything = true;

                destroy_iStruct(t, bWasInArrayOrMap, bDestroyEverything);
                if (!bWasInArrayOrMap || (bDestroyEverything && !t.bIsOrigin))
                    delete &t;

                if (!bParentShouldDestroyEverything)
                    bDestroyEverything = false;
            }
            else if constexpr (std::is_same_v<Variant, TT>)
            {
                if (!bWasInArrayOrMap || bDestroyEverything)
                    delete &t;
            }
            else if constexpr (is_specialisation_of<ContainerVariantTemplate, TT>{})
            {
                delete t.v;
                if (!bWasInArrayOrMap || bDestroyEverything)
                    delete &t;
            }
            else if constexpr (is_array_type<TT>())
            {
                bWasInArrayOrMap = true;
                for (auto& a : t)
                    routeDestroy(a, bWasInArrayOrMap, bDestroyEverything);
                t.clear();
                if (!bParentIsArrayOrMap)
                    bWasInArrayOrMap = false;

                if (bDestroyEverything)
                    delete &t;
            }
            else if constexpr (is_map_type<TT>())
            {
                bWasInArrayOrMap = true;
                for (auto& [key, val] : t)
                    routeDestroy(val, bWasInArrayOrMap, bDestroyEverything);
                t.clear();
                if (!bParentIsArrayOrMap)
                    bWasInArrayOrMap = false;

                if (bDestroyEverything)
                    delete &t;
            }
            else if constexpr (!std::is_same_v<IgnoreType, TT> || !std::is_same_v<BumpType, TT>)
                if (bDestroyEverything)
                    delete &t;
        }

        template<typename TT, typename ...TT2>
        static void destroy_i(Type<TT, TT2...>& t, bool& bWasInArrayOrMap, bool& bDestroyEverything) noexcept
        {
            routeDestroy(*t.data, bWasInArrayOrMap, bDestroyEverything);
            if (t.next() != nullptr)
                destroy_i(*t.next(), bWasInArrayOrMap, bDestroyEverything);
        }

        template<typename TT, typename ...TT2>
        static void destroy_iStruct(Struct<TT, TT2...>& t, bool& bWasInArrayOrMap, bool& bDestroyEverything) noexcept
        {
            routeDestroy(*t.data, bWasInArrayOrMap, bDestroyEverything);
            if (t.next() != nullptr)
                destroy_i(*t.next(), bWasInArrayOrMap, bDestroyEverything);
        }
    };

    template<typename T>
    class Type<T>
    {
    public:
        Type() noexcept = default;
        Type(T& t) noexcept
        {
            init(t);
        }

        void init(T& t) noexcept
        {
            data = &t;
        }

        constexpr Type<T>* next() noexcept
        {
            return nullptr;
        }

        T* data = nullptr;
    };

    // The Struct compile-time tree that is used for nested structures
    template<typename T, typename... T2>
    class Struct : public Type<T, T2...>
    {
    public:
        Struct() noexcept : Type<T, T2...>() {};
        Struct(T& t, T2&... args) noexcept : Type<T, T2...>(t, args...) {};

        bool bShouldBeFreed = false;
        bool bIsOrigin = false;
    };

    template<typename T>
    class Struct<T> : public Type<T>
    {
    public:
        Struct() noexcept = default;
        Struct(T& t) noexcept : Type<T>(t) {};

        bool bShouldBeFreed = false;
        bool bIsOrigin = false;
    };

    // Allocates a new structure for building inline schemas
    template<typename T, typename... T2>
    Struct<T, T2...>& makeStruct(Struct<T, T2...>&& t) noexcept
    {
        auto* tmp = new Struct<T, T2...>{t};
        return *tmp;
    }

    class Message;

    struct Variant
    {
        std::function<bool(Message&, Iterator&, void**, void*)> parse = [](Message&, Iterator&, void**, void*) -> bool { return true; };
        void* data = nullptr;
    };

    Variant& makeVariant(Variant&& v) noexcept;

    // Since variants can be part of a container it's important that we allow for a dynamic way
    // to allocate these variants but still associate them with some user-defined function.
    // This type allows us to push a variant template that we can allocate for every element
    // of the container
    template<typename T>
    struct ContainerVariantTemplate
    {
        T* t = nullptr;
        Variant* v = nullptr;
    };

    template<typename T>
    ContainerVariantTemplate<T>& associateWithVariant(T& t, Variant& v) noexcept
    {
        auto* tmp = new ContainerVariantTemplate<T>{ &t, &v };
        return *tmp;
    }
}