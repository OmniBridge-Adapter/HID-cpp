#pragma once

#include "hid/driver.hpp"
#include "hid/application.hpp"

namespace OB::HID
{
    /* abstract */ class RootDriver : public Driver
    {
    public:
        RootDriver() : m_application{nullptr}{}
        Application *getCollection() { return m_application; }
        const Application *getCollection() const { return m_application; }
        void setCollection(Application *collection) { m_application = collection; }
    protected:
    private:
        Application *m_application;
    };
}