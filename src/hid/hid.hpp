#pragma once

#include <stdint.h>


namespace OB::HID
{
    enum class hid_report_type_t
    {
        NONE = 0,
        Input = 1,
        Output = 2,
        Feature = 3,
    };
    
    typedef uint8_t hid_report_id_t;
    // typedef hid_report_type_t hid_report_type_t;
    
    typedef uint16_t hid_report_size_t;
    typedef uint16_t hid_report_descriptor_size_t;
}
