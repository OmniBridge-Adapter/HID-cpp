#include <gtest/gtest.h>

#include "hid/report.hpp"

#include <array>
#include <cstdint>
#include <span>

TEST(CappedArray, RepeatReplicatesValues)
{
    CappedArray<std::uint8_t, 3> source;
    source.push_back(0x11);
    source.push_back(0x22);
    source.push_back(0x33);

    const auto repeated = source.repeat<4>(2);
    ASSERT_EQ(repeated.size(), 6U);
    EXPECT_EQ(repeated[0], 0x11);
    EXPECT_EQ(repeated[1], 0x22);
    EXPECT_EQ(repeated[2], 0x33);
    EXPECT_EQ(repeated[3], 0x11);
    EXPECT_EQ(repeated[4], 0x22);
    EXPECT_EQ(repeated[5], 0x33);
}

TEST(Bitspan, SetAndGetAcrossBytes)
{
    std::array<std::uint8_t, 2> backing{0x00, 0x00};
    bitspan span{std::span<std::uint8_t>(backing)};

    span.set<std::uint16_t>(0b101010101, 3, 9);
    const auto value = span.get<std::uint16_t>(3, 9);

    EXPECT_EQ(value, 0b101010101);
}

TEST(Bitspan, SubspanUsesRelativeOffsets)
{
    std::array<std::uint8_t, 3> backing{0x00, 0x00, 0x00};
    bitspan root{std::span<std::uint8_t>(backing)};
    auto sub = root.subspan(5, 10);

    sub.set<std::uint16_t>(0x155, 1, 9);
    const auto from_sub = sub.get<std::uint16_t>(1, 9);
    const auto from_root = root.get<std::uint16_t>(6, 9);

    EXPECT_EQ(from_sub, 0x155);
    EXPECT_EQ(from_root, 0x155);
}
