#include <gtest/gtest.h>

#include "hid/proxy.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

namespace
{

constexpr uint16_t kGenericDesktopPage = 0x01;
constexpr uint8_t kUsageMouse = 0x02;
constexpr uint8_t kUsageJoystick = 0x04;

class FakeDriver final : public OB::HID::Driver
{
  public:
    uint8_t hid_input_report_space_available(hid_report_id_t report_id) override
    {
        last_space_query_id = report_id;
        return space_available;
    }

    uint8_t *hid_get_input_report_buf(hid_report_id_t report_id, hid_report_size_t &report_len) override
    {
        last_claim_id = report_id;
        report_len = claim_size;
        last_claim_size = report_len;
        return claim_success ? claim_buffer.data() : nullptr;
    }

    bool hid_send_input_report(hid_report_id_t report_id, const uint8_t* report_buf,
                               uint16_t report_len) override
    {
        last_send_id = report_id;
        last_send_len = report_len;
        if (report_buf != nullptr && report_len > 0)
        {
            last_send_payload.assign(report_buf, report_buf + report_len);
        }
        return send_success;
    }

    uint8_t space_available = 1;
    hid_report_size_t claim_size = 8;
    bool claim_success = true;
    bool send_success = true;

    hid_report_id_t last_space_query_id = 0;
    hid_report_id_t last_claim_id = 0;
    hid_report_size_t last_claim_size = 0;
    hid_report_id_t last_send_id = 0;
    uint16_t last_send_len = 0;
    std::vector<uint8_t> last_send_payload;
    std::array<uint8_t, 8> claim_buffer{};
};

class FakeApplication final : public OB::HID::Application
{
  public:
    FakeApplication(OB::HID::Driver& driver, std::vector<uint8_t> descriptor, hid_report_size_t get_size)
        : Application(driver), descriptor_(std::move(descriptor)), get_size_(get_size)
    {
    }

    std::span<const uint8_t> hid_get_report_descriptor_fragment(void) const override
    {
        return descriptor_;
    }

    hid_report_size_t hid_get_report_callback(hid_report_id_t report_id, hid_report_type_t report_type,
                                              uint8_t* buffer, hid_report_size_t reqlen) override
    {
        last_get_report_id = report_id;
        last_get_report_type = report_type;
        last_get_reqlen = reqlen;
        if (buffer != nullptr && reqlen > 0)
        {
            buffer[0] = 0xA5;
        }
        return get_size_;
    }

    void hid_set_report_callback(hid_report_id_t report_id, hid_report_type_t report_type,
                                 const uint8_t* buffer, hid_report_size_t reqlen) override
    {
        last_set_report_id = report_id;
        last_set_report_type = report_type;
        last_set_reqlen = reqlen;
        last_set_payload.clear();
        if (buffer != nullptr && reqlen > 0)
        {
            last_set_payload.assign(buffer, buffer + reqlen);
        }
    }

    void hid_report_complete_callback(hid_report_id_t report_id) override
    {
        last_complete_report_id = report_id;
        last_complete_report_type = hid_report_type_t::Input;
    }

    hid_report_id_t last_get_report_id = 0;
    hid_report_type_t last_get_report_type = hid_report_type_t::NONE;
    hid_report_size_t last_get_reqlen = 0;

    hid_report_id_t last_set_report_id = 0;
    hid_report_type_t last_set_report_type = hid_report_type_t::NONE;
    hid_report_size_t last_set_reqlen = 0;
    std::vector<uint8_t> last_set_payload;

    hid_report_id_t last_complete_report_id = 0;
    hid_report_type_t last_complete_report_type = hid_report_type_t::NONE;

  private:
    std::vector<uint8_t> descriptor_;
    hid_report_size_t get_size_;
};

// std::vector<uint8_t> descriptorWithInputAndOutputReportId1()
// {
//     // using namespace OB::HID
//     using MyReport = Report<
//         Collection<
//             CollectionType::Application,
//             // UsagePage<1>,
//             Usage<
//             ReportID
//         >,
//     >;
//     DescriptorBuilder builder;
//     builder.usage_page(kGenericDesktopPage)
//         .usage(kUsageMouse)
//         .collection_begin(hid::rdf::main::collection_type::APPLICATION)
//         .report_id(1)
//         .report_size(8)
//         .report_count(1)
//         .input(0x02)
//         .report_id(1)
//         .report_size(8)
//         .report_count(1)
//         .output(0x02)
//         .collection_end();

//     return builder.build();
// }

// std::vector<uint8_t> descriptorWithInputReportId1()
// {
//     DescriptorBuilder builder;
//     builder.usage_page(kGenericDesktopPage)
//         .usage(kUsageJoystick)
//         .collection_begin(hid::rdf::main::collection_type::APPLICATION)
//         .report_id(1)
//         .report_size(8)
//         .report_count(1)
//         .input(0x02)
//         .collection_end();

//     return builder.build();
// }

} // namespace

// TEST(ProxyBasic, RemapsConflictingInputReportIdsPerApplication)
// {
//     FakeDriver rootDriver;
//     OB::HID::Proxy proxy(rootDriver);

//     FakeApplication c1(rootDriver, descriptorWithInputAndOutputReportId1(), 1);
//     FakeApplication c2(rootDriver, descriptorWithInputReportId1(), 1);

//     proxy.addApplication(&c1);
//     proxy.addApplication(&c2);

//     auto [in1Application, in1LocalId] = proxy.hid_report_id_mapping(1, HID_REPORT_TYPE_INPUT);
//     EXPECT_EQ(in1Application, &c1);
//     EXPECT_EQ(in1LocalId, 1);

//     auto [out1Application, out1LocalId] = proxy.hid_report_id_mapping(1, HID_REPORT_TYPE_OUTPUT);
//     EXPECT_EQ(out1Application, &c1);
//     EXPECT_EQ(out1LocalId, 1);

//     auto [in2Application, in2LocalId] = proxy.hid_report_id_mapping(2, HID_REPORT_TYPE_INPUT);
//     EXPECT_EQ(in2Application, &c2);
//     EXPECT_EQ(in2LocalId, 1);
// }

// TEST(ProxyBasic, ForwardsGetAndSetToMappedApplicationReportId)
// {
//     FakeDriver rootDriver;
//     OB::HID::Proxy proxy(rootDriver);

//     FakeApplication c1(rootDriver, descriptorWithInputAndOutputReportId1(), 3);
//     FakeApplication c2(rootDriver, descriptorWithInputReportId1(), 7);

//     proxy.addApplication(&c1);
//     proxy.addApplication(&c2);

//     std::array<uint8_t, 4> getBuffer{};
//     const hid_report_size_t getSize =
//         proxy.hid_get_report_callback(2, HID_REPORT_TYPE_INPUT, getBuffer.data(), getBuffer.size());

//     EXPECT_EQ(getSize, 7);
//     EXPECT_EQ(c2.last_get_report_id, 1);
//     EXPECT_EQ(c2.last_get_report_type, HID_REPORT_TYPE_INPUT);
//     EXPECT_EQ(getBuffer[0], 0xA5);

//     const std::array<uint8_t, 3> payload = {0x10, 0x20, 0x30};
//     proxy.hid_set_report_callback(1, HID_REPORT_TYPE_OUTPUT, payload.data(), payload.size());

//     EXPECT_EQ(c1.last_set_report_id, 1);
//     EXPECT_EQ(c1.last_set_report_type, HID_REPORT_TYPE_OUTPUT);
//     ASSERT_EQ(c1.last_set_payload.size(), payload.size());
//     EXPECT_EQ(c1.last_set_payload[0], 0x10);
//     EXPECT_EQ(c1.last_set_payload[1], 0x20);
//     EXPECT_EQ(c1.last_set_payload[2], 0x30);
// }

// TEST(ProxyBasic, ForwardsInputBufferClaimsAndReleaseOnSend)
// {
//     FakeDriver rootDriver;
//     OB::HID::Proxy proxy(rootDriver);
//     OB::HID::Driver &proxyDriver{proxy};

//     FakeApplication c1(rootDriver, descriptorWithInputReportId1(), 1);
//     proxy.addApplication(&c1);

//     hid_report_size_t claimSize = 0;
//     uint8_t *claimedBuffer = proxyDriver.hid_get_input_report_buf(1, claimSize);

//     ASSERT_NE(claimedBuffer, nullptr);
//     EXPECT_EQ(claimedBuffer, rootDriver.claim_buffer.data());
//     EXPECT_EQ(claimSize, rootDriver.claim_size);
//     EXPECT_EQ(rootDriver.last_claim_id, 1);
//     EXPECT_EQ(rootDriver.last_claim_size, rootDriver.claim_size);

//     claimedBuffer[0] = 0x42;
//     EXPECT_TRUE(proxyDriver.hid_send_input_report(1, claimedBuffer, claimSize));
//     EXPECT_EQ(rootDriver.last_send_id, 1);
//     EXPECT_EQ(rootDriver.last_send_len, claimSize);
//     ASSERT_FALSE(rootDriver.last_send_payload.empty());
//     EXPECT_EQ(rootDriver.last_send_payload[0], 0x42);
// }
