#pragma once

#include <array>
#include "hid/report_descriptor/tag.hpp"

namespace OB::HID::ReportDescriptor
{

    class DescriptorGlobalState
    {
    public:
        constexpr DescriptorGlobalState()
        : m_has{}
        , m_values{}
        {}

        constexpr bool has(TagGlobal tag) const
        {
            return m_has[index(tag)];
        }

        constexpr int value(TagGlobal tag) const
        {
            return m_values[index(tag)];
        }

        constexpr void set(TagGlobal tag, int value)
        {
            const std::size_t idx = index(tag);
            m_has[idx] = true;
            m_values[idx] = value;
        }

    private:
        static constexpr std::size_t index(TagGlobal tag)
        {
            return static_cast<std::size_t>(tag);
        }

        std::array<bool, 16> m_has;
        std::array<int, 16> m_values;
    };


}