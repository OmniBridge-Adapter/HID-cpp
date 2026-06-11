#pragma once

#include <cstdint>

namespace OB::HID
{
    struct UsagePageInfo
    {
        std::uint16_t usagePage;
    };

    template<class T>
    constexpr UsagePageInfo get_info();

}