#ifndef __DSL_ESP_WIFIMANAGER_H__
#define __DSL_ESP_WIFIMANAGER_H__

#include <Arduino.h>
#include <dsl_esp_appbase.h>

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            class WiFiManager : public AppBase
            {
            private:
                unsigned long __baudrate;

            public:
                WiFiManager(unsigned long serial_baudrate = 9600);
                void setup();
                void loop();
            };
        };
    };
};

#endif /* __DSL_ESP_WIFIMANAGER_H__ */