#pragma once

#include "hid.hpp"

#include <span>

namespace OB::HID
{
    class Driver;
    class Application
    {
    public:
        Application(Driver &driver) : m_driver{driver}{}
        virtual std::span<const uint8_t> hid_get_report_descriptor_fragment(void) const = 0;
        virtual hid_report_size_t        hid_get_report_callback     (hid_report_id_t report_id, hid_report_type_t report_type, uint8_t*       buffer, hid_report_size_t reqlen) = 0;
        virtual void                     hid_set_report_callback     (hid_report_id_t report_id, hid_report_type_t report_type, uint8_t const* buffer, hid_report_size_t reqlen) = 0;
        virtual void                     hid_report_complete_callback(hid_report_id_t report_id) = 0;
    protected:
        Driver &driver() { return m_driver; }
    private:
        Driver &m_driver;
    };
}
