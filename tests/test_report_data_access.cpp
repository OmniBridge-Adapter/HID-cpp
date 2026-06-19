#include <gtest/gtest.h>

#include "hid/report.hpp"

#include <array>
#include <cstdint>
#include <span>

using namespace OB::HID::ReportDescriptor;

namespace
{
    enum class TestCollection : std::uint16_t
    {
        Keyboard = 0x06,
    };

    enum class TestKeys : std::uint16_t
    {
        K224 = 224,
        K225 = 225,
        K226 = 226,
        K227 = 227,
    };

    enum class TestNestedCollection : std::uint16_t
    {
        Outer = 0x30,
        Inner = 0x31,
    };

    enum class TestNestedField : std::uint16_t
    {
        Value = 0x40,
        Missing = 0x41,
        Repeated = 0x42,
    };

    enum class TestOtherField : std::uint16_t
    {
        Ghost = 0x60,
    };
}

namespace OB::HID
{
    template<>
    constexpr UsagePageInfo get_info<TestCollection>()
    {
        return UsagePageInfo{0x01};
    }

    template<>
    constexpr UsagePageInfo get_info<TestKeys>()
    {
        return UsagePageInfo{0x07};
    }

    template<>
    constexpr UsagePageInfo get_info<TestNestedCollection>()
    {
        return UsagePageInfo{0x0D};
    }

    template<>
    constexpr UsagePageInfo get_info<TestNestedField>()
    {
        return UsagePageInfo{0x20};
    }
}

TEST(ReportDataAccess, UsageSetGetWithNoCollectionNesting)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestKeys>,
        Input<
            DataFlags<
                DataOrConstant::Data,
                ArrayOrVariable::Variable
            >,
            UsageMinimum<TestKeys::K224>,
            UsageMaximum<TestKeys::K227>,
            ReportCount<Helpers::Constant<4>>,
            ReportSize<Helpers::Constant<1>>,
            LogicalMinimum<Helpers::Constant<0>>,
            LogicalMaximum<Helpers::Constant<1>>
        >
    >;

    MyReport report{};
    std::array<std::uint8_t, 2> buffer{1, 0};

    report.set<TestKeys::K225>(std::span<std::uint8_t>(buffer), 1U);
    report.set<TestKeys::K227>(std::span<std::uint8_t>(buffer), 1U);

    EXPECT_EQ(report.get<TestKeys::K224>(std::span<const std::uint8_t>(buffer)), 0U);
    EXPECT_EQ(report.get<TestKeys::K225>(std::span<const std::uint8_t>(buffer)), 1U);
    EXPECT_EQ(report.get<TestKeys::K227>(std::span<const std::uint8_t>(buffer)), 1U);
}

TEST(ReportDataAccess, UsageSetGetWithinCollection)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestCollection>,
        Collection<
            CollectionType::Logical,
            Usage<TestCollection::Keyboard>,
            Input<
                DataFlags<
                    DataOrConstant::Data,
                    ArrayOrVariable::Variable
                >,
                UsagePage<TestKeys>,
                UsageMinimum<TestKeys::K224>,
                UsageMaximum<TestKeys::K227>,
                ReportCount<Helpers::Constant<4>>,
                ReportSize<Helpers::Constant<1>>,
                LogicalMinimum<Helpers::Constant<0>>,
                LogicalMaximum<Helpers::Constant<1>>
            >
        >
    >;

    MyReport report{};
    std::array<std::uint8_t, 2> buffer{1, 0};

    report.set<TestCollection::Keyboard, TestKeys::K224>(std::span<std::uint8_t>(buffer), 1U);
    report.set<TestCollection::Keyboard, TestKeys::K227>(std::span<std::uint8_t>(buffer), 1U);

    EXPECT_EQ(buffer[0], 1U);
    EXPECT_EQ(buffer[1], 0x09U);

    const auto k224 = report.get<TestCollection::Keyboard, TestKeys::K224>((buffer));
    const auto k225 = report.get<TestCollection::Keyboard, TestKeys::K225>(std::span<const std::uint8_t>(buffer));
    const auto k227 = report.get<TestCollection::Keyboard, TestKeys::K227>(std::span<const std::uint8_t>(buffer));

    EXPECT_EQ(k224, 1U);
    EXPECT_EQ(k225, 0U);
    EXPECT_EQ(k227, 1U);
}

TEST(ReportDataAccess, UsageSetGetWithinNestedCollections)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestNestedCollection>,
        Collection<
            CollectionType::Logical,
            Usage<TestNestedCollection::Outer>,
            Collection<
                CollectionType::Logical,
                Usage<TestNestedCollection::Inner>,
                Input<
                    DataFlags<
                        DataOrConstant::Data,
                        ArrayOrVariable::Variable
                    >,
                    UsagePage<TestNestedField>,
                    Usage<TestNestedField::Value>,
                    ReportCount<Helpers::Constant<1>>,
                    ReportSize<Helpers::Constant<8>>,
                    LogicalMinimum<Helpers::Constant<0>>,
                    LogicalMaximum<Helpers::Constant<255>>
                >
            >
        >
    >;

    MyReport report{};
    std::array<std::uint8_t, 2> buffer{1, 0};

    report.set<TestNestedCollection::Outer, TestNestedCollection::Inner, TestNestedField::Value>(std::span<std::uint8_t>(buffer), 0x5AU);

    EXPECT_EQ(buffer[1], 0x5AU);
    
    const auto value = report.get<TestNestedCollection::Outer, TestNestedCollection::Inner, TestNestedField::Value>(std::span<const std::uint8_t>(buffer));

    EXPECT_EQ(value, 0x5AU);
}

TEST(ReportDataAccess, UsageSetGetWithinNestedCollectionsAllowsSkippingUnambiguousIntermediateUsage)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestNestedCollection>,
        Collection<
            CollectionType::Logical,
            Usage<TestNestedCollection::Outer>,
            Collection<
                CollectionType::Logical,
                Usage<TestNestedCollection::Inner>,
                Input<
                    DataFlags<
                        DataOrConstant::Data,
                        ArrayOrVariable::Variable
                    >,
                    UsagePage<TestNestedField>,
                    Usage<TestNestedField::Value>,
                    ReportCount<Helpers::Constant<1>>,
                    ReportSize<Helpers::Constant<8>>,
                    LogicalMinimum<Helpers::Constant<0>>,
                    LogicalMaximum<Helpers::Constant<255>>
                >
            >
        >
    >;

    MyReport report{};
    std::array<std::uint8_t, 2> buffer{1, 0};

    report.set<TestNestedCollection::Outer, TestNestedField::Value>(std::span<std::uint8_t>(buffer), 0x6CU);
    const auto value = report.get<TestNestedCollection::Outer, TestNestedField::Value>(std::span<const std::uint8_t>(buffer));

    EXPECT_EQ(value, 0x6CU);
}


TEST(ReportDataAccess, InvalidUsagePathIsRejectedAtCompileTime)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestNestedCollection>,
        Collection<
            CollectionType::Logical,
            Usage<TestNestedCollection::Outer>,
            Input<
                DataFlags<
                    DataOrConstant::Data,
                    ArrayOrVariable::Variable
                >,
                UsagePage<TestNestedField>,
                Usage<TestNestedField::Value>,
                ReportCount<Helpers::Constant<1>>,
                ReportSize<Helpers::Constant<8>>,
                LogicalMinimum<Helpers::Constant<0>>,
                LogicalMaximum<Helpers::Constant<255>>
            >
        >
    >;

    static_assert(!SupportsMissingUsageGet<MyReport>);
    SUCCEED();
}

TEST(ReportDataAccess, DynamicRepeatCollectionSelectsPathInstance)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestNestedCollection>,
        Repeat<
            1,
            4,
            Collection<
                CollectionType::Logical,
                Usage<TestNestedCollection::Outer>,
                Input<
                    DataFlags<
                        DataOrConstant::Data,
                        ArrayOrVariable::Variable
                    >,
                    UsagePage<TestNestedField>,
                    Usage<TestNestedField::Value>,
                    ReportCount<Helpers::Constant<1>>,
                    ReportSize<Helpers::Constant<8>>,
                    LogicalMinimum<Helpers::Constant<0>>,
                    LogicalMaximum<Helpers::Constant<255>>
                >
            >
        >
    >;

    MyReport report{repeat_count_t{2}};
    std::array<std::uint8_t, 3> buffer{1, 0, 0};

    report.set<TestNestedCollection::Outer, TestNestedField::Value>(std::span<std::uint8_t>(buffer), 0x11U, 0);
    report.set<TestNestedCollection::Outer, TestNestedField::Value>(std::span<std::uint8_t>(buffer), 0x22U, 1);

    EXPECT_EQ(buffer[1], 0x11);
    EXPECT_EQ(buffer[2], 0x22);

    const auto first = report.get<TestNestedCollection::Outer, TestNestedField::Value>(std::span<const std::uint8_t>(buffer), 0);
    const auto second = report.get<TestNestedCollection::Outer, TestNestedField::Value>(std::span<const std::uint8_t>(buffer), 1);

    EXPECT_EQ(first, 0x11U);
    EXPECT_EQ(second, 0x22U);
    EXPECT_EQ(report.reportSize(), 3U);
}

TEST(ReportDataAccess, DynamicRepeatCollectionRespectsRuntimeCount)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestNestedCollection>,
        Repeat<
            1,
            4,
            Collection<
                CollectionType::Logical,
                Usage<TestNestedCollection::Outer>,
                Input<
                    DataFlags<
                        DataOrConstant::Data,
                        ArrayOrVariable::Variable
                    >,
                    UsagePage<TestNestedField>,
                    Usage<TestNestedField::Value>,
                    ReportCount<Helpers::Constant<1>>,
                    ReportSize<Helpers::Constant<8>>,
                    LogicalMinimum<Helpers::Constant<0>>,
                    LogicalMaximum<Helpers::Constant<255>>
                >
            >
        >
    >;

    MyReport report{repeat_count_t{2}};
    EXPECT_EQ(report.reportSize(), 3U);
}

TEST(ReportDataAccess, ReportSizeIncludesReportIDAndDataBits)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestKeys>,
        Input<
            DataFlags<
                DataOrConstant::Data,
                ArrayOrVariable::Variable
            >,
            UsageMinimum<TestKeys::K224>,
            UsageMaximum<TestKeys::K227>,
            ReportCount<Helpers::Constant<4>>,
            ReportSize<Helpers::Constant<1>>,
            LogicalMinimum<Helpers::Constant<0>>,
            LogicalMaximum<Helpers::Constant<1>>
        >
    >;

    MyReport report{};

    EXPECT_EQ(report.reportSize(), 2U);
}

TEST(ReportDataAccess, RuntimeIndicesSelectCollectionAndArrayElement)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>,
        UsagePage<TestNestedCollection>,
        Repeat<
            1,
            4,
            Collection<
                CollectionType::Logical,
                Usage<TestNestedCollection::Outer>,
                Input<
                    DataFlags<
                        DataOrConstant::Data,
                        ArrayOrVariable::Variable
                    >,
                    UsagePage<TestNestedField>,
                    Usage<TestNestedField::Value>,
                    ReportCount<Helpers::Constant<2>>,
                    ReportSize<Helpers::Constant<8>>,
                    LogicalMinimum<Helpers::Constant<0>>,
                    LogicalMaximum<Helpers::Constant<255>>
                >
            >
        >
    >;

    MyReport report{repeat_count_t{2}};
    std::array<std::uint8_t, 5> buffer{1, 0, 0, 0, 0};

    const std::array<std::size_t, 2> firstInSecondCollection{1, 0};
    const std::array<std::size_t, 2> secondInSecondCollection{1, 1};

    report.set<TestNestedCollection::Outer, TestNestedField::Value>(std::span<std::uint8_t>(buffer), 0x33U, std::span<const std::size_t>(firstInSecondCollection));
    report.set<TestNestedCollection::Outer, TestNestedField::Value>(std::span<std::uint8_t>(buffer), 0x77U, std::span<const std::size_t>(secondInSecondCollection));

    const auto first = report.get<TestNestedCollection::Outer, TestNestedField::Value>(std::span<const std::uint8_t>(buffer), std::span<const std::size_t>(firstInSecondCollection));
    const auto second = report.get<TestNestedCollection::Outer, TestNestedField::Value>(std::span<const std::uint8_t>(buffer), std::span<const std::size_t>(secondInSecondCollection));

    EXPECT_EQ(first, 0x33U);
    EXPECT_EQ(second, 0x77U);
}
