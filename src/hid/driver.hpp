#pragma once

#include "hid/hid.hpp"

namespace OB::HID
{
    /*interface*/ class Driver
    {
    public:
        virtual uint8_t  hid_input_report_space_available(hid_report_id_t report_id) = 0;
        virtual uint8_t *hid_get_input_report_buf        (hid_report_id_t report_id, hid_report_size_t &report_len) = 0;
        virtual bool     hid_send_input_report           (hid_report_id_t report_id, const uint8_t *report_buf, uint16_t report_len) = 0;
    protected:
    private:
    };
}