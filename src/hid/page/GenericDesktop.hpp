#pragma once

#include <cstdint>
#include "hid/usage.hpp"

namespace OB::HID
{
    namespace Page
    {
        enum class GenericDesktop
        {
            Undefined              = 0x00,
            Pointer                = 0x01,
            Mouse                  = 0x02,
            Joystick               = 0x04,
            Gamepad                = 0x05,
            Keyboard               = 0x06,
            Keypad                 = 0x07,
            MultiAxisController    = 0x08,
            TabletPCSystemControls = 0x09,
            WaterCoolingDevice     = 0x0A,
            ComputerChassisDevice  = 0x0B,
            WirelessRadioControls  = 0x0C,
        };
    }

    template<>
    constexpr UsagePageInfo get_info<Page::GenericDesktop>() 
    {
        return UsagePageInfo{0x01};
    }

}