#include "proxy.hpp"
// #include "hid-rp/hid/report.hpp"
// #include "hid-rp/hid/rdf/descriptor_view.hpp"
// #include "hid-rp/hid/rdf/parser.hpp"

#include "hid/report_descriptor/item.hpp"

#include <memory>
#include <cassert>

using namespace OB::HID;

namespace
{

class ReportIDCollector
{
  public:
    using report_selector_t = std::tuple<hid_report_id_t, hid_report_type_t>;

    std::set<report_selector_t> collect(std::span<const uint8_t> descriptor)
    {
        reportIDs_.clear();
        hid_report_id_t global_report_id = 0;
        while (descriptor.size() > 1)
        {
            uint8_t bTag  = (descriptor[0]>>4)&0x0F;
            uint8_t bType = (descriptor[0]>>2)&0x03;
            uint8_t bSize = (descriptor[0]>>0)&0x03;
            if ((bType == 1) &&
                (bTag  == 8) &&
                (bSize == 1))
            {
                global_report_id = descriptor[1];
            }
            else if ((bType = 0) &&
                     ((bTag == 8 ) ||
                      (bTag == 9 ) ||
                      (bTag == 10)))
            {
                if (bTag == 8 ) reportIDs_.insert(std::tuple(global_report_id, hid_report_type_t::Input  ));
                if (bTag == 9 ) reportIDs_.insert(std::tuple(global_report_id, hid_report_type_t::Output ));
                if (bTag == 10) reportIDs_.insert(std::tuple(global_report_id, hid_report_type_t::Feature));
            }
            if (bSize == 3) bSize++;
            bSize++;
            descriptor = descriptor.subspan(bSize, descriptor.size()-bSize);
        }
        return reportIDs_;
    }

  private:
    std::set<report_selector_t> reportIDs_;
};
} // namespace

template<typename T>
struct DescriptorView
{
    DescriptorView(std::span<T> span) : m_span{span}{}

    struct DescriptorViewIterator{

        struct ItemRef{

            constexpr ReportDescriptor::ItemType type() const
            {
                uint8_t bType = (m_span[0] >> 2 ) & 0x03;
                return static_cast<ReportDescriptor::ItemType>(bType);
            }

            template<typename U> 
            constexpr U tag() const
            {
                assert(ReportDescriptor::TagType<U>() == type());
                uint8_t bTag = (m_span[0] >> 4) & 0x0F;
                return static_cast<U>(bTag);
            }

            constexpr ReportDescriptor::TagMain   tagMain()   const
            { 
                assert(type() == ReportDescriptor::ItemType::Main);   
                return tag<ReportDescriptor::TagMain>(); 
            }
            constexpr ReportDescriptor::TagLocal  tagLocal()  const
            { 
                assert(type() == ReportDescriptor::ItemType::Local);  
                return tag<ReportDescriptor::TagLocal>(); 
            }
            constexpr ReportDescriptor::TagGlobal tagGlobal() const
            { 
                assert(type() == ReportDescriptor::ItemType::Global); 
                return tag<ReportDescriptor::TagGlobal>(); 
            }


            constexpr std::size_t data_size() const
            {
                uint8_t bSize = m_span[0];
                if (bSize == 3) bSize++;
                return bSize;
            }

            constexpr std::span<T> data() 
            {
                return m_span;
            }

            constexpr std::span<const T> data() const
            {
                return m_span;
            }

            constexpr std::size_t size() const
            {
                return data_size() + 1;
            }

            std::span<T> m_span;
        };

        DescriptorViewIterator(std::span<T> span): m_span{span}{}

        ItemRef operator*()
        {
            ItemRef i{m_span};
            auto size = i.size();
            return ItemRef{m_span.subspan(0, size)};
        }

        DescriptorViewIterator operator++()
        {
            DescriptorViewIterator self = *this;
            ItemRef item = self.operator*();
            uint8_t itemSize = item.size();
            m_span = m_span.subspan(itemSize, m_span.size_bytes() - itemSize);
            return self;
        }

        bool operator==(const DescriptorViewIterator &other) const
        {
            if (other.m_span.data() == nullptr && m_span.size_bytes() == 0) return true; // at end
            return (other.m_span.data() == m_span.data()) && (other.m_span.size_bytes() == m_span.size_bytes());
        }

        std::span<T> m_span;
    };

    DescriptorViewIterator begin()
    {
        return {m_span};
    }

    DescriptorViewIterator end()
    {
        return std::span<T>{static_cast<T*>(nullptr), 0};
    }

    std::span<T> m_span;
};

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
            const auto* itemData = reinterpret_cast<const uint8_t*>(std::addressof(item));
            const size_t itemLen = item.size();

            if (item.type() == OB::HID::ReportDescriptor::ItemType::Global &&
                item.tagGlobal() == OB::HID::ReportDescriptor::TagGlobal::ReportID &&
                item.data_size() == 1)
            {
                hid_report_id_t oldId = item.data()[0];
                auto remap = oldToNew.find(oldId);
                if (remap != oldToNew.end())
                {
                    m_reportDescriptor.push_back(itemData[0]);
                    m_reportDescriptor.push_back(remap->second);
                }
                else
                {
                    m_reportDescriptor.insert(m_reportDescriptor.end(), itemData, itemData + itemLen);
                }
            }
            else
            {
                m_reportDescriptor.insert(m_reportDescriptor.end(), itemData, itemData + itemLen);
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
