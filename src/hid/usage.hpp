#pragma once

#include <cstdint>

namespace OB::HID
{

    // concept UsagePage = 
    struct UsagePageInfo
    {
        std::uint16_t usagePage;
    };

    template<class Page>
    constexpr UsagePageInfo get_info();

    template<class Page>
    constexpr uint32_t get_usage_id(Page usage)
    {
        uint32_t usageID = (get_info<Page>().usagePage << 16) | static_cast<uint16_t>(usage);
        return usageID;
    }

}