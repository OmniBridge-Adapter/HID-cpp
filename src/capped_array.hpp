#pragma once

#include <array>
#include <type_traits>
#include <cstddef>
#include <cassert>
#include <algorithm>

template<typename T, std::size_t SIZE> requires(std::is_default_constructible_v<T>)
class CappedArray
{
public:
    constexpr CappedArray()
        : m_realSize{0}
    {}
#if 0
    template<size_t MAX_REPEAT = std::numeric_limits<std::size_t>::max()>
    constexpr CappedArray<T, SIZE*MAX_REPEAT> repeat(size_t repeats) const
    {
        assert(repeats < MAX_REPEATS);
        CappedArray<T, SIZE*MAX_REPEAT> out;
        size_t idx = 0;
        for (size_t i = 0; i < repeats; i++)
        {
            out.insert(out.end(), begin(), end());
        }
    }

    constexpr std::vector<T> repeat<std::numeric_limits<std::size_t>::max()>(std::size_t repeats) const
    {
        std::vector<T> out;
        out.reserve(size() * repeats);
        for (std::size_t i = 0; i < repeats; i++)
        {
            out.insert(out.cend(), begin(), end());
        }
    }
#else
    template<size_t MAX_REPEAT>
    constexpr CappedArray<T, SIZE*MAX_REPEAT> repeat(size_t repeats) const
    {
        assert(repeats <= MAX_REPEAT);
        CappedArray<T, SIZE*MAX_REPEAT> out;
        for (size_t i = 0; i < repeats; i++)
        {
            out.insert(out.end(), cbegin(), cend());
        }
        return out;
    }
#endif
    using array_type = std::array<T, SIZE>;
    using iterator = array_type::iterator;
    using const_iterator = array_type::const_iterator;

    template<class IT>
    constexpr iterator insert(iterator it, IT _begin, IT _end)
    {
        assert(m_realSize == (end()-begin()));
        size_t count = _end - _begin;
        assert((m_realSize + count) <= SIZE);
        std::shift_right(it, end(), count);
        std::copy(_begin, _end, it);
        m_realSize += count;
        return it;
    }

    constexpr void push_back(const T& t)
    {
        assert(m_realSize != SIZE);
        m_items[m_realSize] = t;
        m_realSize++;
    }

    constexpr iterator begin()
    {
        return m_items.begin();
    }

    constexpr const_iterator cbegin() const
    {
        return m_items.cbegin();
    }

    constexpr iterator end()
    {
        return m_items.begin() + m_realSize;
    }

    constexpr const_iterator cend() const
    {
        return m_items.cbegin() + m_realSize;
    }

    constexpr std::size_t size() const { return m_realSize; }

    constexpr T &operator[](std::size_t idx)
    {
        assert(idx < size());
        return m_items[idx];
    }
    constexpr const T &operator[](std::size_t idx) const
    {
        assert(idx < size());
        return m_items[idx];
    }
    constexpr static std::size_t max_size = SIZE;
protected:
private:
    // constexpr CappedArray
    std::array<T, SIZE> m_items;
    size_t m_realSize;
};
