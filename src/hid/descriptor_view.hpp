#pragma once

#include "hid/report_descriptor/tag.hpp"

namespace OB::HID
{
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
                    return tag<ReportDescriptor::TagMain>(); 
                }
                constexpr ReportDescriptor::TagLocal  tagLocal()  const
                { 
                    return tag<ReportDescriptor::TagLocal>(); 
                }
                constexpr ReportDescriptor::TagGlobal tagGlobal() const
                { 
                    return tag<ReportDescriptor::TagGlobal>(); 
                }


                constexpr std::size_t data_size() const
                {
                    uint8_t bSize = m_span[0] & 0x03;
                    if (bSize == 3) bSize++;
                    return bSize;
                }

                constexpr std::span<T> data() 
                {
                    return m_span.subspan(0, size());
                }

                constexpr std::span<const T> data() const
                {
                    return m_span.subspan(0, size());
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

}