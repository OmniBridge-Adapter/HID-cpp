#pragma once

#include <cstdint>
#include <type_traits>

#include "hid/usage.hpp"

namespace OB::HID
{
    namespace Page
    {
        struct Ordinal
        {
            constexpr operator uint16_t() const
            {
                return value;
            }
            uint16_t value;
            constexpr Ordinal(uint16_t v) : value(v) {}

            // Constructor to allow implicit conversion from enum types to ordinal.
            template<class T> requires(std::is_enum_v<T>)
            constexpr Ordinal(T v) : value(static_cast<uint16_t>(v)) {}
        };
    }

    template<>
    constexpr UsagePageInfo get_info<Page::Ordinal>() 
    {
        return UsagePageInfo{0x0A};
    }

}