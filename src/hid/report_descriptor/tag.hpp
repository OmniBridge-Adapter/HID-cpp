#pragma once

#include <cstdint>

namespace OB::HID::ReportDescriptor
{

    using TagBase = std::uint8_t;

    enum class TagMain : TagBase
    {
        Input         = 0b1000,
        Output        = 0b1001,
        Feature       = 0b1011,
        Collection    = 0b1010,
        EndCollection = 0b1100,
    };

    enum class TagGlobal : TagBase
    {
        UsagePage       = 0b0000,
        LogicalMinimum  = 0b0001,
        LogicalMaximum  = 0b0010,
        PhysicalMinimum = 0b0011,
        PhysicalMaximum = 0b0100,
        UnitExponent    = 0b0101,
        Unit            = 0b0110,
        ReportSize      = 0b0111,
        ReportID        = 0b1000,
        ReportCount     = 0b1001,
        Push            = 0b1010,
        Pop             = 0b1011,
    };

    enum class TagLocal : TagBase
    {
        Usage        = 0b0000,
        UsageMinimum = 0b0001,
        UsageMaximum = 0b0010,
    };

    enum class ItemType
    {
        Main = 0,
        Global = 1,
        Local = 2,
        Reserved = 3,
    };
}
