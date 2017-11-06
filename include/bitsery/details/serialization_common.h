//MIT License
//
//Copyright (c) 2017 Mindaugas Vinkelis
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#ifndef BITSERY_DETAILS_SERIALIZATION_COMMON_H
#define BITSERY_DETAILS_SERIALIZATION_COMMON_H

#include <type_traits>
#include <utility>
#include <tuple>
#include "adapter_utils.h"
#include "../traits/core/traits.h"


namespace bitsery {

    //this allows to call private serialize method for the class
    //just make friend it to that class
    struct Access {
        template<typename S, typename T>
        static auto serialize(S &s, T &obj) -> decltype(obj.serialize(s)) {
            obj.serialize(s);
        }
    };

    //when call to serialize function is ambiguous (member and non-member serialize function exists for a type)
    //specialize this class, by inheriting from either UseNonMemberFnc or UseMemberFnc
    //e.g.
    //template <> struct SelectSerializeFnc<MyDerivedClass>:UseMemberFnc {};
    template<typename T>
    struct SelectSerializeFnc : std::integral_constant<int, 0> {
    };

    //types you need to inherit from when specializing SelectSerializeFnc class
    struct UseNonMemberFnc : std::integral_constant<int, 1> {
    };
    struct UseMemberFnc : std::integral_constant<int, 2> {
    };


    //serializer/deserializer, does not public interface to get underlying writer/reader
    //to prevent users from using writer/reader directly, because they have different interface
    //and they cannot be used describing serialization flows.: use extensions for this reason.
    //this class allows to get underlying adapter writer/reader, and only should be used outside serialization functions.
    struct AdapterAccess {
        template<typename Serializer>
        static typename Serializer::TWriter &getWriter(Serializer &s) {
            return s._writer;
        }

        template<typename Deserializer>
        static typename Deserializer::TReader &getReader(Deserializer &s) {
            return s._reader;
        }
    };

    namespace details {

        //helper types for error handling
        template<typename T>
        struct IsContainerTraitsDefined : public IsDefined<typename traits::ContainerTraits<T>::TValue> {
        };

        template<typename T>
        struct IsTextTraitsDefined : public IsDefined<typename traits::TextTraits<T>::TValue> {
        };

        template<typename Ext, typename T>
        struct IsExtensionTraitsDefined : public IsDefined<typename traits::ExtensionTraits<Ext, T>::TValue> {
        };

#ifdef _MSC_VER
        //helper types for HasSerializeFunction
        template <typename S, typename T>
        using TrySerializeFunction = decltype(serialize(std::declval<S &>(), std::declval<T &>()));

        template <typename S, typename T>
        struct HasSerializeFunctionHelper {
            template <typename Q, typename R, typename = TrySerializeFunction<Q, R>>
            static std::true_type tester(Q&&, R&&);
            static std::false_type tester(...);
            using type = decltype(tester(std::declval<S>(), std::declval<T>()));
        };
        template <typename S, typename T>
        struct HasSerializeFunction :HasSerializeFunctionHelper<S, T>::type {};

        //helper types for HasSerializeMethod
        template <typename S, typename T>
        using TrySerializeMethod = decltype(Access::serialize(std::declval<S &>(), std::declval<T &>()));

        template <typename S, typename T>
        struct HasSerializeMethodHelper {
            template <typename Q, typename R, typename = TrySerializeMethod<Q, R>>
            static std::true_type tester(Q&&, R&&);
            static std::false_type tester(...);
            using type = decltype(tester(std::declval<S>(), std::declval<T>()));
        };
        template <typename S, typename T>
        struct HasSerializeMethod :HasSerializeMethodHelper<S, T>::type {};

        //helper types for IsFlexibleIncluded
        template <typename S, typename T>
        using TryArchiveProcess = decltype(archiveProcess(std::declval<S &>(), std::declval<T &&>()));

        template <typename S, typename T>
        struct IsFlexibleIncludedHelper {
            template <typename Q, typename R, typename = TryArchiveProcess<Q, R>>
            static std::true_type tester(Q&&, R&&);
            static std::false_type tester(...);
            using type = decltype(tester(std::declval<S>(), std::declval<T>()));
        };

        template <typename S, typename T>
        struct IsFlexibleIncluded :IsFlexibleIncludedHelper<S, T>::type {};
#else
        //helper metafunction, that is added to c++17
        template<typename... Ts>
        struct make_void {
            typedef void type;
        };
        template<typename... Ts>
        using void_t = typename make_void<Ts...>::type;

        template<typename, typename, typename = void>
        struct HasSerializeFunction : std::false_type {
        };

        template<typename S, typename T>
        struct HasSerializeFunction<S, T,
                void_t<decltype(serialize(std::declval<S &>(), std::declval<T &>()))>
        > : std::true_type {
        };


        template<typename, typename, typename = void>
        struct HasSerializeMethod : std::false_type {
        };

        template<typename S, typename T>
        struct HasSerializeMethod<S, T,
                void_t<decltype(Access::serialize(std::declval<S &>(), std::declval<T &>()))>
        > : std::true_type {
        };

        //this solution doesn't work with visual studio, but is more elegant
        template<typename, typename, typename = void>
        struct IsFlexibleIncluded : std::false_type {
        };

        template<typename S, typename T>
        struct IsFlexibleIncluded<S, T,
                void_t<decltype(archiveProcess(std::declval<S &>(), std::declval<T &&>()))>
        > : std::true_type {
        };
#endif


        //used for extensions, when extension TValue = void
        struct DummyType {
        };

/*
 * this includes all integral types floats and enums(except bool)
 */
        template<typename T>
        struct IsFundamentalType : std::integral_constant<bool,
                std::is_enum<T>::value
                || std::is_floating_point<T>::value
                || std::is_integral<T>::value> {
        };

        template<typename T, typename Integral = void>
        struct IntegralFromFundamental {
            using TValue = T;
        };

        template<typename T>
        struct IntegralFromFundamental<T, typename std::enable_if<std::is_enum<T>::value>::type> {
            using TValue = typename std::underlying_type<T>::type;
        };

        template<typename T>
        struct IntegralFromFundamental<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
            using TValue = typename std::conditional<std::is_same<T, float>::value, uint32_t, uint64_t>::type;
        };

        template<typename T>
        struct UnsignedFromFundamental {
            using type = typename std::make_unsigned<typename IntegralFromFundamental<T>::TValue>::type;
        };

        template<typename T>
        using SameSizeUnsigned = typename UnsignedFromFundamental<T>::type;


/*
 * functions for object serialization
 */

        template<typename S, typename T>
        struct SerializeFunction {

            static void invoke(S &s, T &v) {
                static_assert(HasSerializeFunction<S, T>::value || HasSerializeMethod<S, T>::value,
                              "\nPlease define 'serialize' function for your type (inside or outside of class):\n"
                                      "  template<typename S>\n"
                                      "  void serialize(S& s)\n"
                                      "  {\n"
                                      "    ...\n"
                                      "  }\n");
                using TDecayed = typename std::decay<T>::type;
                selectSerializeFnc(s, v, SelectSerializeFnc<TDecayed>{});
            }

        private:
            static void selectSerializeFnc(S &s, T &v, std::integral_constant<int, 0>) {
                static_assert(!(HasSerializeFunction<S, T>::value && HasSerializeMethod<S, T>::value),
                              "\nPlease define only one 'serialize' function (member OR free).\n"
                                      "If serialization function is inherited from base class, then explicitly select correct function for your type e.g.:\n"
                                      "  template <>\n"
                                      "  struct SelectSerializeFnc<DerivedClass>:UseMemberFnc {};\n");
                selectSerializeFnc(s, v, std::integral_constant<int,
                        HasSerializeFunction<S, T>::value ? 1 : 2>{});
            }

            static void selectSerializeFnc(S &s, T &v, std::integral_constant<int, 1>) {
                serialize(s, v);
            }

            static void selectSerializeFnc(S &s, T &v, std::integral_constant<int, 2>) {
                Access::serialize(s, v);
            }
        };

/*
 * functions for object serialization
 */

        template<typename S, typename T, typename Enabled = void>
        struct ArchiveFunction {

            static void invoke(S &s, T &&obj) {
                static_assert(IsFlexibleIncluded<S, T>::value,
                              "\nPlease include '<bitsery/flexible.h>' to use 'archive' function:\n");

                archiveProcess(s, std::forward<T>(obj));
            }
        };

        /*
         * helper function for getting context from serializer/deserializer
         */

        template<typename T, template<typename...> class Template>
        struct IsSpecializationOf : std::false_type {
        };

        template<template<typename...> class Template, typename... Args>
        struct IsSpecializationOf<Template<Args...>, Template> : std::true_type {
        };

        //helper types for better error messages
        template<typename Find, typename ... TList>
        struct GetTypeIndex : std::integral_constant<size_t, 0> {
        };

        //found it
        template<typename Find, typename ... Tail>
        struct GetTypeIndex<Find, Find, Tail...> : std::integral_constant<size_t, 0> {
        };

        //iteratates over types
        template<typename Find, typename Head, typename ... Tail>
        struct GetTypeIndex<Find, Head, Tail...> : std::integral_constant<size_t,
                1 + GetTypeIndex<Find, Tail...>::value> {
        };

        template<typename Find, typename ... TList>
        struct HasType : std::integral_constant<bool, (GetTypeIndex<Find, TList...>::value<(sizeof ... (TList)))> {
        };

        template<typename TCast, typename Tuple>
        struct HasContext : std::is_same<TCast, Tuple> {
        };

        template<typename TCast, typename ... Args>
        struct HasContext<TCast, std::tuple<Args...>> : HasType<TCast, Args...> {
        };

        /*
         * get context, and static assert if type doesn't exists
         */

        template<typename TCast, typename ... Args>
        TCast *getContextImpl(std::tuple<Args...> *ctx, std::true_type) {
            using TCastIndex = GetTypeIndex<TCast, Args...>;
            static_assert(HasType<TCast, Args...>::value, "Invalid context cast. Type doesn't exists.");
            return std::addressof(std::get<TCastIndex::value>(*ctx));
        }

        template<typename TCast, typename TContext>
        TCast *getContextImpl(TContext *ctx, std::false_type) {
            static_assert(std::is_convertible<TContext *, TCast *>::value, "Invalid context cast. Type doesn't exists.");
            return static_cast<TCast *>(ctx);
        }

        //get local ctx
        template<typename TCast, typename TContext, typename TInternalContext>
        TCast *chooseInternalOrExternalContext(TContext *, TInternalContext &internalCtx, std::true_type) {
            return getContextImpl<TCast>(&internalCtx, std::true_type{});
        }

        //get external ctx
        template<typename TCast, typename TContext, typename TInternalContext>
        TCast *chooseInternalOrExternalContext(TContext *ctx, TInternalContext &, std::false_type) {
            return ctx
                   ? getContextImpl<TCast>(ctx, IsSpecializationOf<TContext, std::tuple>{})
                   : nullptr;
        }

        template<typename TCast, typename TContext, typename TInternalContext>
        TCast *getContext(TContext *ctx, TInternalContext &internalCtx) {
            return chooseInternalOrExternalContext<TCast>(ctx, internalCtx, HasContext<TCast, TInternalContext>{});
        }

        /*
         * get context, if type doesn't exists then do not static_assert but return null instead
         */

        template<typename TCast, typename TContext>
        TCast *getContextFromTypeIfExists(TContext *ctx, std::true_type) {
            return static_cast<TCast *>(ctx);
        }

        template<typename TCast, typename TContext>
        TCast *getContextFromTypeIfExists(TContext *, std::false_type) {
            return nullptr;
        }

        template<typename TCast, typename TContext>
        TCast *getContextImplIfExists(TContext *ctx, std::false_type) {
            return getContextFromTypeIfExists<TCast>(ctx, std::is_convertible<TContext *, TCast *>{});
        }

        template<typename TCast, typename TContext>
        TCast *getContextFromTupleIfExists(TContext *ctx, std::true_type tmp) {
            return getContextImpl<TCast>(ctx, tmp);
        }

        template<typename TCast, typename TContext>
        TCast *getContextFromTupleIfExists(TContext *, std::false_type) {
            return nullptr;
        }

        template<typename TCast, typename TContext>
        TCast *getContextImplIfExists(TContext *ctx, std::true_type) {
            return getContextFromTupleIfExists<TCast>(ctx, HasContext<TCast, TContext>{});
        }

        //get local ctx
        template<typename TCast, typename TContext, typename TInternalContext>
        TCast *chooseInternalOrExternalContextIfExists(TContext *, TInternalContext &internalCtx, std::true_type) {
            return getContextImplIfExists<TCast>(&internalCtx, std::true_type{});
        }

        //get external ctx
        template<typename TCast, typename TContext, typename TInternalContext>
        TCast *chooseInternalOrExternalContextIfExists(TContext *ctx, TInternalContext &, std::false_type) {
            return ctx
                   ? getContextImplIfExists<TCast>(ctx, IsSpecializationOf<TContext, std::tuple>{})
                   : nullptr;
        }

        template<typename TCast, typename TContext, typename TInternalContext>
        TCast *getContextIfTypeExists(TContext *ctx, TInternalContext &internalCtx) {
            return chooseInternalOrExternalContextIfExists<TCast>(ctx, internalCtx, HasContext<TCast, TInternalContext>{});
        }

    }
}

#endif //BITSERY_DETAILS_SERIALIZATION_COMMON_H

