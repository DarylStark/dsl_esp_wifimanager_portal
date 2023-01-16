#include "dsl_esp_wifimanager.h"

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            WiFiManager::WiFiManager(const std::string ap_ssid, unsigned long serial_baudrate /* = 9600 */)
                : __baudrate(serial_baudrate), __web_server(80), __ap_ssid(ap_ssid), __last_scan(0)
            {
            }

            void WiFiManager::setup()
            {
                // Start the Serial
                Serial.begin(__baudrate);

                // Start the setup for
                _setup();

                // TODO: get WiFi credentials from flash and try to connect
                // to the WiFi

                // Configure for scanning
                configure_for_scanning();
            }

            void WiFiManager::loop()
            {
                _loop();

                __web_server.handleClient();
                delay(20);
            }

            void WiFiManager::configure_for_scanning()
            {
                // Start the WiFi scanner
                std::thread scan_thread(std::bind(&WiFiManager::update_wifi_list, this));
                scan_thread.detach();

                // Set the WiFi mode to AP
                WiFi.mode(WIFI_AP_STA);

                // Configure the Access Point
                WiFi.softAP(__ap_ssid.c_str(), NULL);

                // Configure the webserver
                __web_server.on(
                    "/api/network_list",
                    HTTP_GET,
                    std::bind(&WiFiManager::__api_network_list, this));

                // Start the webserver
                __web_server.begin();
            }

            void WiFiManager::__api_network_list()
            {
                std::stringstream json_output;
                json_output << "{";
                json_output << "\"networks\": [";
                uint16_t cnt = 0;

                for (const auto &network : __networks)
                {
                    json_output << "{";
                    json_output << "\"ssid\": \"" << network.ssid << "\",";
                    json_output << "\"bssid\": \"" << network.bssid << "\",";
                    json_output << "\"channel\": " << network.channel << ",";
                    json_output << "\"rssi\": " << network.rssi << ",";
                    json_output << "\"encryption\": " << network.encryption << "";
                    json_output << "}";
                    if (++cnt < __networks.size())
                        json_output << ",";
                }

                json_output << "]";
                json_output << "}";
                __web_server.send(200, "text/json", json_output.str().c_str());
            }

            void WiFiManager::update_wifi_list()
            {
                while (true)
                {
                    delay(3000);

                    // TODO: LOCK

                    // Clean up
                    WiFi.scanDelete();

                    // Get the WiFi networks
                    uint16_t ssid_count = WiFi.scanNetworks(false, true);
                    WiFi.mode(WIFI_AP_STA);
                    __networks.clear();

                    for (uint16_t ssid_id = 0; ssid_id < ssid_count; ssid_id++)
                    {
                        __networks.push_back(
                            {WiFi.SSID(ssid_id).c_str(),
                             WiFi.BSSIDstr(ssid_id).c_str(),
                             WiFi.channel(ssid_id),
                             WiFi.RSSI(ssid_id),
                             WiFi.encryptionType(ssid_id)});
                    }
                }
            }
        };
    };
};