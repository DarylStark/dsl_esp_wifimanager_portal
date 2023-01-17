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
#include <Preferences.h>
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
                std::string password;
            };

            class WiFiManager : public AppBase
            {
            public:
                enum ManagerMode
                {
                    Starting,
                    ConnectPortal,
                    Connecting,
                    Connected
                };

            private:
                unsigned long __baudrate;
                std::list<WiFiNetwork> __networks;
                std::list<WiFiNetwork> __known_networks;
                WebServer __web_server;
                DNSServer __dns_server;
                std::string __ap_ssid;
                std::mutex __wifi_lock;
                Preferences __preferences;
                ManagerMode __mode;

                void __web_root();
                void __api_network_list();
                void __api_save_network();
                void __api_clear_saved_networks();

            public:
                WiFiManager(const std::string ap_ssid, unsigned long serial_baudrate = 9600);
                void setup();
                void loop();

                void configure_for_scanning_mode();
                void update_wifi_list();
            };
        };
    };
};

#endif /* __DSL_ESP_WIFIMANAGER_H__ */