#pragma once

#include <stdint.h>

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

namespace OB::HID
{
    class RootDriver;
    /* Defined externally */
    uint8_t maxDrivers();
    RootDriver *getDriver(uint8_t idx);
    void InitDriver();
    /* End Defined externally */
    
    void Init();
}

#ifdef __cplusplus
extern "C" {
#endif

// void hid_init(void);


const uint8_t               *hid_report_descriptor_data(void);
hid_report_descriptor_size_t hid_report_descriptor_size(uint8_t instance);

#ifdef __cplusplus
}
#endif

