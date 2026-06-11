#pragma once

#include "hid/application.hpp"
#include "hid/driver.hpp"

#include <map>
#include <vector>
#include <tuple>
#include <set>

namespace OB::HID
{
    class Proxy : public Application, public Driver
    {
    public:
        Proxy(Driver &driver) : Application{driver}{}
        using report_id_mapping_t = std::tuple<Application *, hid_report_id_t>;
        void addApplication(Application *collection);
        const std::vector<Application *> &applications() const;

        virtual std::span<const uint8_t> hid_get_report_descriptor_fragment(void) const override;
        virtual hid_report_size_t        hid_get_report_callback(hid_report_id_t report_id, hid_report_type_t report_type, uint8_t*       buffer, hid_report_size_t reqlen) override;
        virtual void                     hid_set_report_callback(hid_report_id_t report_id, hid_report_type_t report_type, uint8_t const* buffer, hid_report_size_t reqlen) override;
        virtual void                     hid_report_complete_callback(hid_report_id_t report_id) override;
        report_id_mapping_t              hid_report_id_mapping(hid_report_id_t report_id, hid_report_type_t report_type) const;
    protected:
        virtual uint8_t hid_input_report_space_available(hid_report_id_t report_id);
        virtual uint8_t *hid_get_input_report_buf        (hid_report_id_t report_id, hid_report_size_t &report_len);
        virtual bool    hid_send_input_report           (hid_report_id_t report_id, const uint8_t *report_buf, uint16_t report_len);

    private:
        hid_report_id_t nextUnusedReportID(hid_report_type_t reportType) const;
        hid_report_id_t remapInputReportID(hid_report_id_t localReportID) const;

        using report_mapping_t = std::tuple<hid_report_id_t, hid_report_type_t, hid_report_id_t>;
        std::map<
            Application*, 
            std::vector<report_mapping_t>
        > m_endpoints;
        // A reportID of reportType maps to an Application's reportID of the same reportType.
        std::map<std::tuple<hid_report_id_t, hid_report_type_t>, report_id_mapping_t> m_reportMapping;
        std::map<hid_report_id_t, std::set<hid_report_id_t>> m_inputLocalToRemapped;
        std::vector<Application *> m_applications;
        std::vector<uint8_t> m_reportDescriptor;
    };
}