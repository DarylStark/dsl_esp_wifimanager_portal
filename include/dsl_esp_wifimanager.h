#ifndef __DSL_ESP_WIFIMANAGER_H__
#define __DSL_ESP_WIFIMANAGER_H__

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <string>
#include <list>
#include <sstream>
#include <thread>
#include <mutex>
#include <dsl_esp_appbase.h>

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            struct WiFiNetwork
            {
                std::string ssid;
                std::string bssid;
                int32_t channel;
                int32_t rssi;
                uint16_t encryption;
            };

            class WiFiManager : public AppBase
            {
            private:
                unsigned long __baudrate;
                std::list<WiFiNetwork> __networks;
                WebServer __web_server;
                DNSServer __dns_server;
                std::string __ap_ssid;
                std::mutex __wifi_lock;
                std::mutex __ssid_list_lock;

                void __web_root();
                void __api_network_list();
                void __api_save_network();

            public:
                WiFiManager(const std::string ap_ssid, unsigned long serial_baudrate = 9600);
                void setup();
                void loop();

                void configure_for_scanning();
                void update_wifi_list();
            };
        };
    };
};

#endif /* __DSL_ESP_WIFIMANAGER_H__ */