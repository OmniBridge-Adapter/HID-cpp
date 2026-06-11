#include "hid.hpp"
#include "root_driver.hpp"

#include "interface.hpp"
#include "modules/gpio/gpio.hpp"

#include <array>
#include <cstdio>
#include <memory>

extern "C" bool tud_hid_n_report(uint8_t instance, uint8_t report_id, void const *report,
                                  uint16_t len);

namespace
{
std::array<ModuleDef, 1> g_moduleDefs = {
    gpioModuleDef,
};

constexpr hid_report_size_t kInputReportBufferSize = 64;

std::array<uint8_t, kInputReportBufferSize> g_inputReportBuffer{};
bool g_inputReportBufferClaimed = false;
hid_report_id_t g_inputReportBufferReportId = 0;

std::array<int, 32> g_interfacePins = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
};

std::unique_ptr<Interface> g_interface;
std::vector<std::unique_ptr<Interface>> g_interfaces;

}

// class RootDriver : public OB::HID::Driver
// {
// public:
//     virtual uint8_t hid_input_report_space_available(hid_report_id_t report_id)
//     {
//         (void)report_id;
//         return g_inputReportBufferClaimed ? 0 : static_cast<uint8_t>(g_inputReportBuffer.size());
//     }

//     virtual uint8_t *hid_get_input_report_buf(hid_report_id_t report_id, hid_report_size_t &report_len)
//     {
//         if (g_inputReportBufferClaimed)
//         {
//             report_len = 0;
//             return nullptr;
//         }

//         g_inputReportBufferClaimed = true;
//         g_inputReportBufferReportId = report_id;
//         report_len = static_cast<hid_report_size_t>(g_inputReportBuffer.size());
//         return g_inputReportBuffer.data();
//     }

//     virtual bool    hid_send_input_report           (hid_report_id_t report_id, const uint8_t *report_buf, uint16_t report_len)
//     {
//         const bool sent = tud_hid_n_report(0, report_id, report_buf, report_len);
//         if (report_buf == g_inputReportBuffer.data() &&
//             g_inputReportBufferClaimed &&
//             g_inputReportBufferReportId == report_id)
//         {
//             g_inputReportBufferClaimed = false;
//             g_inputReportBufferReportId = 0;
//         }

//         return sent;
//     }
// protected:
// private:
// } g_rootDriver;

void OB::HID::Init()
{
    OB::HID::RootDriver *driver = getDriver(0);
    if (driver == nullptr)
        return;
    g_interfaces.push_back(std::make_unique<Interface>(*driver, g_moduleDefs, g_interfacePins));
    driver->setCollection(&(g_interfaces.back()->collection()));
}

// extern "C" void hid_init(void)
// {
//     if (g_interface)
//     {
//         return;
//     }

//     g_interface = std::make_unique<Interface>(g_rootDriver, g_moduleDefs, g_interfacePins);
// }

// extern "C" const uint8_t *hid_report_descriptor_data(void)
// {
//     if (!g_interface)
//     {
//         hid_init();
//     }

//     auto fragment = g_interface->proxy().hid_get_report_descriptor_fragment();
//     return fragment.data();
// }

// extern "C" hid_report_descriptor_size_t hid_report_descriptor_size(void)
// {
//     if (!g_interface)
//     {
//         hid_init();
//     }

//     auto fragment = g_interface->proxy().hid_get_report_descriptor_fragment();
//     return static_cast<hid_report_descriptor_size_t>(fragment.size());
// }

// extern "C" hid_report_size_t hid_get_report(hid_report_id_t report_id,
//                                              hid_report_type_t report_type,
//                                              uint8_t *buffer,
//                                              hid_report_size_t reqlen)
// {
//     if (!g_interface)
//     {
//         hid_init();
//     }

//     printf("g_interface: %p\n", g_interface.get());

//     return g_interface->proxy().hid_get_report_callback(report_id, report_type, buffer, reqlen);
// }

// extern "C" void hid_set_report(hid_report_id_t report_id,
//                                 hid_report_type_t report_type,
//                                 uint8_t const *buffer,
//                                 hid_report_size_t reqlen)
// {
//     printf("sr\n");
//     if (!g_interface)
//     {
//         printf("init\n");
//         hid_init();
//     }
//     printf("%p\n", g_interface.get());

//     g_interface->proxy().hid_set_report_callback(report_id, report_type, buffer, reqlen);
// }


// uint8_t hid_input_report_space_available(hid_report_id_t report_id)
// {
//     (void)report_id;
//     return g_inputReportBufferClaimed ? 0 : static_cast<uint8_t>(g_inputReportBuffer.size());
// }

// uint8_t *hid_get_input_report_buf(hid_report_id_t report_id, hid_report_size_t *report_len)
// {
//     if (g_inputReportBufferClaimed)
//     {
//         *report_len = 0;
//         return nullptr;
//     }

//     g_inputReportBufferClaimed = true;
//     g_inputReportBufferReportId = report_id;
//     *report_len = static_cast<hid_report_size_t>(g_inputReportBuffer.size());
//     return g_inputReportBuffer.data();
// }
// bool    hid_send_input_report(hid_report_id_t report_id, const uint8_t *report_buf, uint16_t report_len)
// {
//     const bool sent = tud_hid_n_report(0, report_id, report_buf, report_len);
//     if (sent &&
//         report_buf == g_inputReportBuffer.data() &&
//         g_inputReportBufferClaimed &&
//         g_inputReportBufferReportId == report_id)
//     {
//         g_inputReportBufferClaimed = false;
//         g_inputReportBufferReportId = 0;
//     }

//     return sent;
// }