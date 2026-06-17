

#include "hid/proxy.hpp"
#include "hid/report.hpp"
#include "hid/page/GenericDesktop.hpp"
#include "hid/page/Keyboard.hpp"
#include "hid/descriptor_view.hpp"

using namespace OB::HID;

using DummyInput = OB::HID::ReportDescriptor::Input<
    OB::HID::ReportDescriptor::DataFlags<
        OB::HID::ReportDescriptor::DataOrConstant::Data
    >,
    OB::HID::ReportDescriptor::UsagePage<Page::Keyboard>,
    OB::HID::ReportDescriptor::Usage<Page::Keyboard::Keyboard_aA>,
    OB::HID::ReportDescriptor::ReportCount<OB::HID::ReportDescriptor::Helpers::Constant<1>>,
    OB::HID::ReportDescriptor::ReportSize<OB::HID::ReportDescriptor::Helpers::Constant<1>>
>;

using DummyOutput = OB::HID::ReportDescriptor::Output<
    OB::HID::ReportDescriptor::DataFlags<
        OB::HID::ReportDescriptor::DataOrConstant::Data
    >,
    OB::HID::ReportDescriptor::Usage<Page::GenericDesktop::Joystick>,
    OB::HID::ReportDescriptor::ReportCount<OB::HID::ReportDescriptor::Helpers::Constant<1>>,
    OB::HID::ReportDescriptor::ReportSize<OB::HID::ReportDescriptor::Helpers::Constant<1>>
>;

using MyInputReportId1 = OB::HID::Report<
    OB::HID::ReportDescriptor::ReportID<OB::HID::ReportDescriptor::Helpers::Constant<1>>,
    OB::HID::ReportDescriptor::Collection<
        OB::HID::ReportDescriptor::CollectionType::Application,
        OB::HID::ReportDescriptor::UsagePage<Page::GenericDesktop>,
        OB::HID::ReportDescriptor::Usage<Page::GenericDesktop::Keyboard>,
        DummyInput
    >
>;
using MyInputOutputReportId1 = OB::HID::Report<
    OB::HID::ReportDescriptor::ReportID<OB::HID::ReportDescriptor::Helpers::Constant<1>>,
    OB::HID::ReportDescriptor::Collection<
        OB::HID::ReportDescriptor::CollectionType::Application,
        OB::HID::ReportDescriptor::UsagePage<Page::GenericDesktop>,
        OB::HID::ReportDescriptor::Usage<Page::GenericDesktop::Keyboard>,
        DummyInput,
        DummyOutput
    >
>;

#include <cstdio>

int main(int argc, char** argv)
{
    auto myReport = MyInputOutputReportId1{};
    auto descriptor = myReport.descriptor();
    std::span<uint8_t> descriptor_span{descriptor.begin(), descriptor.end()};

    auto descriptorView = DescriptorView<uint8_t>{descriptor_span};

    for (auto item : descriptorView)
    {
        printf("\n");
        for (uint8_t byte : item.data())
        {
            printf("%02X ", byte);
        }
    }
    return 0;
}
