#pragma once

#include <cstdint>
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
        };
    }

    template<>
    constexpr UsagePageInfo get_info<Page::Ordinal>() 
    {
        return UsagePageInfo{0x0A};
    }

}