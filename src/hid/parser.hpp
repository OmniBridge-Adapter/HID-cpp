#pragma once

#include "hid.hpp"
#include "hid/report_descriptor/tag.hpp"
#include "hid/report_descriptor/item.hpp"
// #include "hid/report_descriptor.hpp"

#include <stdint.h>
#include <vector>
#include <span>
#include <set>

#include "hid/report_descriptor.hpp"

namespace OB::HID::ReportDescriptor
{    
    
    class Parser
    {
    public:
        Parser();
        void parse(std::span<const uint8_t> report_descriptor);
        const std::set<std::tuple<hid_report_id_t, hid_report_type_t>> &reportIDs() const;
    protected:
    private:
        void SetGlobal(TagGlobal tag, uint32_t value);
        uint32_t GetGlobal(TagGlobal) const;

        struct GlobalState
        {
            uint16_t valid_mask;
            uint32_t values[16];
        };
        std::vector<GlobalState> m_globalStack;
        GlobalState m_globalState;

        std::set<std::tuple<hid_report_id_t, hid_report_type_t>> m_reportIDs;

        std::vector<CollectionType> m_collectionStack;
    };
}