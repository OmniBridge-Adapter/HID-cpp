#include <gtest/gtest.h>

#include "hid/report.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

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
        K228 = 228,
        K229 = 229,
        K230 = 230,
        K231 = 231,
    };

    enum class TestUsagePage : std::uint16_t
    {
        TestUsage = 0x10,
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
    constexpr UsagePageInfo get_info<TestUsagePage>()
    {
        return UsagePageInfo(0x33);
    }
}

TEST(ReportDescriptor, ReportID_static)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<1>>
    >;
    auto myReport = MyReport{};
    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 2> check_descriptor = {0x85, 0x01};

    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]);
    }
}

TEST(ReportDescriptor, ReportID_variable)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Variable<uint8_t>>
    >;
    uint8_t reportID = 1;
    do{
        auto myReport = MyReport{report_id_t{reportID}};
        auto descriptor = myReport.descriptor();
        std::array<uint8_t, 2> check_descriptor = {0x85, reportID};
    
        EXPECT_EQ(descriptor.size(), check_descriptor.size());
        for (std::size_t i = 0; i < descriptor.size(); i++)
        {
            EXPECT_EQ(descriptor[i], check_descriptor[i]);
        }
    } while(++reportID < 255);
}

TEST(ReportDescriptor, GlobalItemsAreDeduplicated)
{
    using MyReport = OB::HID::Report<
        GlobalItem<TagGlobal::LogicalMinimum, 0>,
        GlobalItem<TagGlobal::LogicalMaximum, 1>,
        GlobalItem<TagGlobal::LogicalMinimum, 0>,
        GlobalItem<TagGlobal::LogicalMaximum, 1>,
        GlobalItem<TagGlobal::LogicalMaximum, 2>
    >;

    auto myReport = MyReport{};
    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 6> check_descriptor = {
        0x15, 0x00,
        0x25, 0x01,
        0x25, 0x02,
    };

    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]);
    }
}

TEST(ReportDescriptor, DynamicGlobalsAreRuntimeParameterized)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Variable<uint8_t>>,
        LogicalMinimum<Helpers::Variable<int>>,
        LogicalMaximum<Helpers::Variable<int>>,
        PhysicalMinimum<Helpers::Variable<int>>,
        PhysicalMaximum<Helpers::Variable<int>>,
        ReportCount<Helpers::Variable<int>>,
        ReportSize<Helpers::Variable<int>>,
        UnitExponent<Helpers::Variable<int>>,
        Unit<Helpers::Variable<int>>
    >;

    auto myReport = MyReport{
        report_id_t{3},
        logical_min_t{0},
        logical_max_t{1},
        physical_min_t{0},
        physical_max_t{1},
        report_count_t{4},
        report_size_t{1},
        unit_exponent_t{0},
        unit_t{0}
    };

    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 18> check_descriptor = {
        0x85, 0x03,
        0x15, 0x00,
        0x25, 0x01,
        0x35, 0x00,
        0x45, 0x01,
        0x95, 0x04,
        0x75, 0x01,
        0x55, 0x00,
        0x65, 0x00,
    };

    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]) << "i: " << i << std::endl;
    }
}

TEST(ReportDescriptor, InputWithDynamic)
{
    using MyReport = OB::HID::Report<
        ReportID<Helpers::Variable<uint8_t>>,
        UsagePage<TestUsagePage>,
        Input<
            DataFlags<
                DataOrConstant::Data,
                ArrayOrVariable::Array
            >,
            Usage<TestUsagePage::TestUsage>,
            LogicalMinimum<Helpers::Variable<int>>,
            LogicalMaximum<Helpers::Variable<int>>,
            ReportCount<Helpers::Constant<1>>,
            ReportSize<Helpers::Constant<8>>
        >
    >;
    auto myReport = MyReport{
        report_id_t{4},
        logical_min_t{4},
        logical_max_t{10}
    };

    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 16> check_descriptor = {
        0x85, 0x04, // ReportID(4)
        0x05, 0x33, // UsagePage(TestUsagePage)
        0x09, 0x10, // Usage(TestUsagePage::TestUsage)
        0x15, 0x04, // LogicalMinimum(4)
        0x25, 0x0A, // LogicalMaximum(10)
        0x95, 0x01, // ReportCount(1)
        0x75, 0x08, // ReportSize(8)
        0x81, 0x00, // Input(Data, Var, ...) 
    };

    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]) << "i: " << i << std::endl;
    }
}


TEST(ReportDescriptor, DescriptorWithCollection)
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

    MyReport myReport{};
    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 25> check_descriptor = {
        0x85, 0x01, // ReportID(1)
        0x05, 0x01, // UsagePage(0x01)
        0x09, 0x06, // Usage(TestCollection::Keyboard)
        0xA1, 0x02, // Collection(Logical)
        0x05, 0x07, // UsagePage(0x07)
        0x19, 0xE0, // UsageMinimum(224)  
        0x29, 0xE3, // UsageMaximum(227)
        0x95, 0x04, // ReportCount(4)
        0x75, 0x01, // ReportSize(1)
        0x15, 0x00, // LogicalMinimum(0)
        0x25, 0x01, // LogicalMaximum(1)
        0x81, 0x02, // Input(Data, Var, ...)
        0xC0
    };
    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]) << "i: " << i << std::endl;
    }
}


TEST(ReportDescriptor, DescriptorWithCollection_2)
{

    using MyReport = OB::HID::Report<
        ReportID<Helpers::Constant<2>>,
        UsagePage<TestCollection>,
        Collection<
            CollectionType::Physical,
            Usage<TestCollection::Keyboard>,
            Input<
                DataFlags<
                    DataOrConstant::Data,
                    ArrayOrVariable::Variable
                >,
                UsagePage<TestKeys>,
                UsageMinimum<TestKeys::K224>,
                UsageMaximum<TestKeys::K231>,
                ReportCount<Helpers::Constant<8>>,
                ReportSize<Helpers::Constant<1>>,
                LogicalMinimum<Helpers::Constant<0>>,
                LogicalMaximum<Helpers::Constant<1>>
            >
        >
    >;

    MyReport myReport{};
    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 25> check_descriptor = {
        0x85, 0x02, // ReportID(2)
        0x05, 0x01, // UsagePage(0x01)
        0x09, 0x06, // Usage(TestCollection::Keyboard)
        0xA1, 0x00, // Collection(Physical)
        0x05, 0x07, // UsagePage(0x07)
        0x19, 0xE0, // UsageMinimum(224)  
        0x29, 0xE7, // UsageMaximum(227)
        0x95, 0x08, // ReportCount(8)
        0x75, 0x01, // ReportSize(1)
        0x15, 0x00, // LogicalMinimum(0)
        0x25, 0x01, // LogicalMaximum(1)
        0x81, 0x02, // Input(Data, Var, ...)
        0xC0
    };
    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]) << "i: " << i << std::endl;
    }
}

TEST(ReportDescriptor, DynamicGlobalsInsideRepeatProduceIdenticalRepeatedStructure)
{
    using MyReport = OB::HID::Report<
        UsagePage<TestCollection>,
        Repeat<
            1,
            3,
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
                    ReportCount<Helpers::Variable<unsigned int>>,
                    ReportSize<Helpers::Variable<unsigned int>>,
                    LogicalMinimum<Helpers::Constant<0>>,
                    LogicalMaximum<Helpers::Constant<1>>
                >
            >
        >
    >;

    auto myReport = MyReport{
        repeat_count_t{2},
        report_count_t{4},
        report_size_t{1}
    };

    auto descriptor = myReport.descriptor();
    std::array<uint8_t, 44> check_descriptor = {
        0x05, 0x01,

        0x09, 0x06,
        0xA1, 0x02,
        0x05, 0x07,
        0x19, 0xE0,
        0x29, 0xE3,
        0x95, 0x04,
        0x75, 0x01,
        0x15, 0x00,
        0x25, 0x01,
        0x81, 0x02,
        0xC0,

        0x09, 0x06,
        0xA1, 0x02,
        0x05, 0x07,
        0x19, 0xE0,
        0x29, 0xE3,
        0x95, 0x04,
        0x75, 0x01,
        0x15, 0x00,
        0x25, 0x01,
        0x81, 0x02,
        0xC0,
    };

    EXPECT_EQ(descriptor.size(), check_descriptor.size());
    for (std::size_t i = 0; i < descriptor.size(); i++)
    {
        EXPECT_EQ(descriptor[i], check_descriptor[i]) << "i: " << i << std::endl;
    }
}