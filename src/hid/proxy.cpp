#include "proxy.hpp"

#include "hid/report_descriptor/item.hpp"
#include "hid/descriptor_view.hpp"

#include <memory>
#include <cassert>
#include <iostream>

using namespace OB::HID;

namespace
{

class ReportIDCollector
{
  public:
    using report_selector_t = std::tuple<hid_report_id_t, hid_report_type_t>;

    std::set<report_selector_t> collect(std::span<const uint8_t> descriptor)
    {
        using namespace OB::HID::ReportDescriptor;
        reportIDs_.clear();
        DescriptorView view{descriptor};
        hid_report_id_t global_report_id = 0;
        for (auto item : view)
        {
            auto type = item.type();
            auto size = item.data_size();
            uint8_t bTag  = (descriptor[0]>>4)&0x0F;
            uint8_t bType = (descriptor[0]>>2)&0x03;
            uint8_t bSize = (descriptor[0]>>0)&0x03;
            if ((item.type() == ItemType::Global) &&
                (item.tagGlobal() == TagGlobal::ReportID) &&
                (item.data_size() == 1))
            {
                global_report_id = descriptor[1];
                std::cout << "ReportID: " << int{global_report_id} << std::endl;
            }
            else if ((item.type() == ItemType::Main) &&
                     ((item.tagMain() == TagMain::Input ) ||
                      (item.tagMain() == TagMain::Output) ||
                      (item.tagMain() == TagMain::Feature)))
            {
                if (item.tagMain() == TagMain::Input  ) reportIDs_.insert(std::tuple(global_report_id, hid_report_type_t::Input  ));
                if (item.tagMain() == TagMain::Output ) reportIDs_.insert(std::tuple(global_report_id, hid_report_type_t::Output ));
                if (item.tagMain() == TagMain::Feature) reportIDs_.insert(std::tuple(global_report_id, hid_report_type_t::Feature));
            }
        }
        return reportIDs_;
    }

  private:
    std::set<report_selector_t> reportIDs_;
};
} // namespace

void Proxy::addApplication(Application *application)
{
    auto reportDescriptor = application->hid_get_report_descriptor_fragment();

    ReportIDCollector parser;
    auto reportIDs = parser.collect(reportDescriptor);
    std::vector<report_mapping_t> mappings;

    // Keep the same remapped report ID for an old report ID within one application,
    // even if it appears across multiple report types.
    std::map<hid_report_id_t, hid_report_id_t> localOldToNew;

    for (const auto& k : reportIDs)
    {
        std::cout << "report id " << int{std::get<0>(k)} << std::endl;
        auto [reportID, reportType] = k;
        
        hid_report_id_t remappedID = 0;
        auto existing = localOldToNew.find(reportID);
        if (existing != localOldToNew.end())
        {
            remappedID = existing->second;
        }
        else
        {
            remappedID = nextUnusedReportID(reportType);
            localOldToNew.emplace(reportID, remappedID);
        }

        std::tuple<Application *, hid_report_id_t> remapped(application, reportID);
        m_reportMapping.emplace(std::make_tuple(remappedID, reportType), remapped);
        report_mapping_t mapping{reportID, reportType, remappedID};
        mappings.push_back(mapping);
        std::cout << "added mapping" << std::endl;

        if (reportType == hid_report_type_t::Input)
        {
            m_inputLocalToRemapped[reportID].insert(remappedID);
        }
    }

    m_endpoints.emplace(application, std::move(mappings));
    m_applications.push_back(application);

    m_reportDescriptor.clear();
    for (Application *endpoint : m_applications)
    {
        auto fragment = endpoint->hid_get_report_descriptor_fragment();
        auto mappingsIt = m_endpoints.find(endpoint);
        if (mappingsIt == m_endpoints.end())
        {
            continue;
        }

        std::map<hid_report_id_t, hid_report_id_t> oldToNew;
        for (const report_mapping_t &mapping : mappingsIt->second)
        {
            auto [oldId, _type, newId] = mapping;
            (void)_type;
            oldToNew.emplace(oldId, newId);
        }
        DescriptorView view{fragment};
        for (const auto item : view)
        {
            const auto itemData = item.data();
            const size_t itemLen = item.size();

            if (item.type() == OB::HID::ReportDescriptor::ItemType::Global &&
                item.tagGlobal() == OB::HID::ReportDescriptor::TagGlobal::ReportID &&
                item.data_size() == 1)
            {
                hid_report_id_t oldId = itemData[1];
                auto remap = oldToNew.find(oldId);
                if (remap != oldToNew.end())
                {
                    m_reportDescriptor.push_back(itemData[0]);
                    m_reportDescriptor.push_back(remap->second);
                }
                else
                {
                    m_reportDescriptor.insert(m_reportDescriptor.end(), itemData.begin(), itemData.end());
                }
            }
            else
            {
                m_reportDescriptor.insert(m_reportDescriptor.end(), itemData.begin(), itemData.end());
            }
        }
    }
}

std::span<const uint8_t> Proxy::hid_get_report_descriptor_fragment(void) const
{
    return m_reportDescriptor;
}

hid_report_size_t Proxy::hid_get_report_callback(hid_report_id_t report_id,
                                                 hid_report_type_t report_type,
                                                 uint8_t *buffer,
                                                 hid_report_size_t reqlen)
{
    auto [application, mappedReportID] = hid_report_id_mapping(report_id, report_type);
    if (application == nullptr)
    {
        return 0;
    }

    return application->hid_get_report_callback(mappedReportID, report_type, buffer, reqlen);
}

void Proxy::hid_set_report_callback(hid_report_id_t report_id,
                                    hid_report_type_t report_type,
                                    uint8_t const *buffer,
                                    hid_report_size_t reqlen)
{
    printf("3\n");
    auto [application, mappedReportID] = hid_report_id_mapping(report_id, report_type);
    if (application == nullptr)
    {
        return;
    }

    application->hid_set_report_callback(mappedReportID, report_type, buffer, reqlen);
}

void Proxy::hid_report_complete_callback(hid_report_id_t report_id)
{
    auto [application, mappedReportID] = hid_report_id_mapping(report_id, hid_report_type_t::Input);
    if (application == nullptr)
    {
        return;
    }
    application->hid_report_complete_callback(mappedReportID);
}

Proxy::report_id_mapping_t Proxy::hid_report_id_mapping(hid_report_id_t report_id, hid_report_type_t report_type) const
{
    std::cout << "mappings: " << std::endl;
    for (auto &mapping : m_reportMapping)
    {
        std::cout << "(" << int{std::get<0>(std::get<0>(mapping))} << "," <<  int{static_cast<uint8_t>(std::get<1>(std::get<0>(mapping)))} << "), " << int{std::get<1>(std::get<1>(mapping))} << std::endl;
    }
    std::cout << std::endl;
    auto it = m_reportMapping.find(std::make_tuple(report_id, report_type));
    if (it == m_reportMapping.end())
    {
        return {nullptr, 0};
    }
    return it->second;
}

uint8_t Proxy::hid_input_report_space_available(hid_report_id_t report_id)
{
    const hid_report_id_t remapped = remapInputReportID(report_id);
    if (remapped == 0)
    {
        return 0;
    }

    return driver().hid_input_report_space_available(remapped);
}

uint8_t *Proxy::hid_get_input_report_buf(hid_report_id_t report_id, hid_report_size_t &report_len)
{
    const hid_report_id_t remapped = remapInputReportID(report_id);
    if (remapped == 0)
    {
        report_len = 0;
        return nullptr;
    }

    return driver().hid_get_input_report_buf(remapped, report_len);
}

bool    Proxy::hid_send_input_report           (hid_report_id_t report_id, const uint8_t *report_buf, uint16_t report_len)
{
    const hid_report_id_t remapped = remapInputReportID(report_id);
    if (remapped == 0)
    {
        return false;
    }

    return driver().hid_send_input_report(remapped, report_buf, report_len);
}

const std::vector<Application *> &Proxy::applications() const
{
    return m_applications;
}

hid_report_id_t Proxy::nextUnusedReportID(hid_report_type_t reportType) const
{
    hid_report_id_t nextID = 1;
    for (auto [k, v] : m_reportMapping)
    {
        (void)v;
        auto [reportID, mappedType] = k;
        if (mappedType != reportType)
        {
            continue;
        }

        if (reportID >= nextID)
        {
            nextID = reportID + 1;
        }
    }
    return nextID;
}

hid_report_id_t Proxy::remapInputReportID(hid_report_id_t localReportID) const
{
    auto it = m_inputLocalToRemapped.find(localReportID);
    if (it == m_inputLocalToRemapped.end() || it->second.empty())
    {
        return 0;
    }

    if (it->second.size() > 1)
    {
        // Ambiguous local report IDs cannot be remapped reliably on this path.
        return 0;
    }

    return *it->second.begin();
}
