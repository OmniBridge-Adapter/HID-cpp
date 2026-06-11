#pragma once

#include <cstdint>
#include <span>

template<typename U = uint8_t>
class bitspan
{
public:
    constexpr bitspan(std::span<U> underlying)
    : m_offset{0}
    , m_bitcount{underlying.size_bytes()*8}
    , m_underlyingSpan(underlying)
    {}

    constexpr bitspan(const bitspan& bitspan, std::size_t offset, std::size_t bitcount)
    : m_offset(offset+bitspan.m_offset)
    , m_bitcount(bitcount)
    , m_underlyingSpan(bitspan.m_underlyingSpan)
    {
        assert((m_offset+m_bitcount) <= (bitspan.m_offset+bitspan.m_bitcount));
    }

    template<typename T>
    constexpr T get(std::size_t bitpos, std::size_t bitcount) const;

    template<typename T> requires(!std::is_const<U>::value)
    constexpr void set(T t, std::size_t bitpos, std::size_t bitcount) const;

    constexpr bitspan subspan(std::size_t offset, std::size_t bitcount) const
    {
        return bitspan(*this, offset, bitcount);
    }
protected:
private:
    std::size_t m_offset;
    std::size_t m_bitcount;
    std::span<U> m_underlyingSpan;
};


template<typename U>
template<typename T>
constexpr T bitspan<U>::get(std::size_t bitpos, std::size_t bitcount) const
{
    static_assert(std::is_integral_v<T>, "bitspan::get requires an integral type");
    assert(bitcount <= (sizeof(T) * 8));
    assert((bitpos + bitcount) <= m_bitcount);

    using unsigned_t = std::make_unsigned_t<T>;
    unsigned_t out = 0;
    const std::size_t absoluteBit = m_offset + bitpos;
    for (std::size_t i = 0; i < bitcount; ++i)
    {
        const std::size_t srcBit = absoluteBit + i;
        const std::size_t srcByte = srcBit / 8;
        const std::size_t srcBitInByte = srcBit % 8;
        const unsigned_t bit = static_cast<unsigned_t>((m_underlyingSpan[srcByte] >> srcBitInByte) & 0x1U);
        out |= static_cast<unsigned_t>(bit << i);
    }

    return static_cast<T>(out);
}

template<typename U>
template<typename T> requires(!std::is_const<U>::value)
constexpr void bitspan<U>::set(T t, std::size_t bitpos, std::size_t bitcount) const
{
    static_assert(std::is_integral_v<T>, "bitspan::set requires an integral type");
    assert(bitcount <= (sizeof(T) * 8));
    assert((bitpos + bitcount) <= m_bitcount);

    using unsigned_t = std::make_unsigned_t<T>;
    const unsigned_t value = static_cast<unsigned_t>(t);
    const std::size_t absoluteBit = m_offset + bitpos;

    for (std::size_t i = 0; i < bitcount; ++i)
    {
        const bool bitSet = ((value >> i) & static_cast<unsigned_t>(1U)) != 0;
        const std::size_t dstBit = absoluteBit + i;
        const std::size_t dstByte = dstBit / 8;
        const std::size_t dstBitInByte = dstBit % 8;
        const std::uint8_t mask = static_cast<std::uint8_t>(1U << dstBitInByte);

        if (bitSet)
        {
            m_underlyingSpan[dstByte] = static_cast<U>(m_underlyingSpan[dstByte] | mask);
        }
        else
        {
            m_underlyingSpan[dstByte] = static_cast<U>(m_underlyingSpan[dstByte] & static_cast<std::uint8_t>(~mask));
        }
    }
}