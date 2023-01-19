#ifndef __DSL_ESP_WIFIMANAGER_PORTAL_H__
#define __DSL_ESP_WIFIMANAGER_PORTAL_H__

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
#include <dsl_esp_wifimanager.h>

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            struct ScannedNetwork : public dsl::esp::connections::WiFiNetwork
            {
                std::string bssid;
                int32_t channel;
                int32_t rssi;
                uint16_t encryption;
            };

            class WiFiManagerPortal : public AppBase
            {
            public:
                enum ManagerMode
                {
                    Starting,
                    Setup,
                    Connecting,
                    Connected,
                    Portal
                };

            private:
                unsigned long __baudrate;
                std::list<ScannedNetwork> __networks;
                WebServer __web_server;
                DNSServer __dns_server;
                std::string __ap_ssid;
                ManagerMode __mode;
                dsl::esp::connections::WiFiManager __wifi_manager;
                std::mutex __wifi_lock;

                void __web_root();
                void __api_network_list();
                void __api_saved_network_list();
                void __api_save_network();
                void __api_clear_saved_networks();
                void __api_delete_network();

            public:
                WiFiManagerPortal(const std::string ap_ssid, unsigned long serial_baudrate = 9600);
                void setup();
                void loop();

                void setup_connecting();
                void loop_connecting();

                void setup_portal();
                void loop_portal();
                void scan_thread();

                uint16_t get_status() const;
            };
        };
    };
};

#endif /* __DSL_ESP_WIFIMANAGER_PORTAL_H__ */