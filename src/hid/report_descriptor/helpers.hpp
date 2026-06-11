#pragma once

#include <type_traits>

namespace OB::HID::ReportDescriptor::Helpers
{
    template<typename T>
    struct Variable{
        using type = T;
    };

    template<auto Value>
    struct Constant
    {
        using type = decltype(Value);
        constexpr const static type value = Value;
    };

    template<class T>
    struct is_constant : std::bool_constant<false> {};

    template<auto Value>
    struct is_constant<Constant<Value>> : std::bool_constant<true> {};

    template<class T>
    struct is_variable : std::bool_constant<false> {};

    template<typename T>
    struct is_variable<Variable<T>> : std::bool_constant<true> {};

    template<typename T>
    struct is_value : std::bool_constant<is_variable<T>::value || is_constant<T>::value> {};

    template<typename T, typename U>
    struct has_type : std::bool_constant<false> {};

    template<typename T, typename U>
    struct has_type<T, Variable<U>> : std::is_same<T, U> {};

    template<typename T, auto Value>
    struct has_type<T, Constant<Value>> : std::is_same<T, decltype(Value)> {};
}