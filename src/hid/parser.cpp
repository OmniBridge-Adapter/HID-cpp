
#include "parser.hpp"
#include "hid.hpp"
#include "hid/report_descriptor/item.hpp"

#include <cassert>

const uint8_t item_sizes[4] = {0, 1, 2, 4};

using namespace OB::HID::ReportDescriptor;

Parser::Parser()
{
    m_globalStack.push_back({});
}

void Parser::parse(std::span<const uint8_t> reportDescriptor)
{

    std::span<const uint8_t> remainingDescriptor = reportDescriptor;
    hid_report_id_t reportID;

    do
    {
        std::span<const uint8_t> thisItem;
        uint8_t b = remainingDescriptor[0];

        ItemType btype = static_cast<ItemType>((b>>2) & 0x03);
        assert((b & 0x0C) != 0x0C && "HID Report Item uses unexpected RESERVED ItemType");

        uint8_t btag = static_cast<TagBase>((b>>4) & 0x0f);

        uint8_t data_size = item_sizes[(b&0x03)];
        assert(data_size <= 4 && "data_size cannot be greater than 4 bytes (32-bits)");

        thisItem = remainingDescriptor.subspan(0, data_size+1);
        remainingDescriptor = remainingDescriptor.subspan(data_size+1);

        uint32_t data = 0;
        for (int i = 0; i < data_size; i++)
        {
            data |= (thisItem[i+1] << (i*8));
        }

        switch (btype)
        {
        case ItemType::Main:
        {
            auto main_btag = static_cast<TagMain>(btag);
            switch (main_btag)
            {
            case TagMain::Input:
            {
                hid_report_id_t reportID = GetGlobal(TagGlobal::ReportID);
                m_reportIDs.emplace(reportID, hid_report_type_t::Input);
                break;
            }
            case TagMain::Output:
            {
                hid_report_id_t reportID = GetGlobal(TagGlobal::ReportID);
                m_reportIDs.emplace(reportID, hid_report_type_t::Output);
                break;
            }
            case TagMain::Feature:
            {
                hid_report_id_t reportID = GetGlobal(TagGlobal::ReportID);
                m_reportIDs.emplace(reportID, hid_report_type_t::Feature);
                break;
            }
            case TagMain::Collection:
            {
                auto collectionType = static_cast<CollectionType>(data);
                m_collectionStack.push_back(collectionType);
                break;
            }
            case TagMain::EndCollection:
            {
                assert(m_collectionStack.size() > 0);
                m_collectionStack.pop_back();
                break;
            }
            }
            break;
        }
        case ItemType::Global:
        {
            auto global_btag = static_cast<TagGlobal>(btag);
            this->SetGlobal(global_btag, data);
            break;
        }
        case ItemType::Local: break;
        case ItemType::Reserved: break;
        default:
            assert(false && "ItemType has invalid value");
        }


    } while (remainingDescriptor.size_bytes() > 0);  
}

const std::set<std::tuple<hid_report_id_t, hid_report_type_t>> &Parser::reportIDs() const
{
    return m_reportIDs;
}

void Parser::SetGlobal(TagGlobal tag, uint32_t value)
{
    if (tag == TagGlobal::Push)
    {
        m_globalStack.push_back(m_globalState);
    }
    else if (tag == TagGlobal::Pop)
    {
        // This should be a runtime check rather than an assertion
        assert(m_globalStack.size() > 0);
        m_globalState = m_globalStack.back(); 
        m_globalStack.pop_back();
    }
    else
    {
        uint8_t idx = static_cast<uint8_t>(tag);
        m_globalState.values[idx] = value;
        m_globalState.valid_mask |= (1<<idx);
    }
}

uint32_t Parser::GetGlobal(TagGlobal tag) const
{
    uint8_t idx = static_cast<uint8_t>(tag);
    assert(m_globalState.valid_mask & (1<<idx));
    return m_globalState.values[idx];
}
