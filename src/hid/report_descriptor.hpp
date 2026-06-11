#pragma once

#include "report_descriptor/tag.hpp"

namespace OB::HID::ReportDescriptor
{    
    enum class CollectionType
    {
        Physical = 0x00,
        Application = 0x01,
        Logical = 0x02,
        Report = 0x03,
        NamedArray = 0x04,
        UsageSwitch = 0x05,
        UsageModifier = 0x06,
    };

}