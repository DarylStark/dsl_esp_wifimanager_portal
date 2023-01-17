#include "dsl_esp_wifimanager.h"

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            WiFiManager::WiFiManager(const std::string ap_ssid, unsigned long serial_baudrate /* = 9600 */)
                : __baudrate(serial_baudrate), __web_server(80), __ap_ssid(ap_ssid), __mode(Starting)
            {
            }

            void WiFiManager::setup()
            {
                // Start the Serial
                Serial.begin(__baudrate);

                // Start the Preferences library
                __preferences.begin("wifimanager");

                // Start the setup for
                _setup();

                // Get saved WiFi networks from the flash
                uint16_t wifi_count = __preferences.getUInt("count", 0);
                for (uint16_t i = 0; i < wifi_count; ++i)
                {
                    std::stringstream field_ssid;
                    field_ssid << "ssid_" << i;
                    std::stringstream field_password;
                    field_password << "password_" << i;
                    std::string ssid = __preferences.getString(field_ssid.str().c_str()).c_str();
                    std::string password = __preferences.getString(field_password.str().c_str()).c_str();

                    if (ssid.length() > 0)
                    {
                        WiFiNetwork network;
                        network.ssid = ssid;
                        network.password = password;
                        __known_networks.emplace_back(network);
                    }
                }

                for (const auto &network : __known_networks)
                {
                    Serial.print("Connecting to network ");
                    Serial.println(network.ssid.c_str());

                    __mode = Connecting;

                    // Connect to the network
                    WiFi.begin(network.ssid.c_str(), network.password.c_str());
                    // TODO: Make this timeout configurable
                    WiFi.waitForConnectResult(10000);

                    if (WiFi.isConnected())
                    {
                        __mode = Connected;
                        Serial.println("CONNECTED!");
                        return;
                    }
                }

                // Configure for scanning
                configure_for_scanning_mode();
            }

            void WiFiManager::loop()
            {
                if (__mode == ConnectPortal)
                {
                    _loop();
                    __web_server.handleClient();
                    __dns_server.processNextRequest();
                    delay(20);
                    return;
                }
                delay(2000);
            }

            void WiFiManager::configure_for_scanning_mode()
            {
                // Set the mode
                __mode = ConnectPortal;

                // Start the WiFi scanner
                std::thread scan_thread(std::bind(&WiFiManager::update_wifi_list, this));
                scan_thread.detach();

                // Set the WiFi mode to AP
                WiFi.mode(WIFI_AP_STA);

                // Configure the Access Point
                WiFi.setHostname("DSL_ESP_WifiManager");

                // Start the AP
                WiFi.softAP(__ap_ssid.c_str(), NULL);

                // Start the DNS server
                __dns_server.start(53, "*", WiFi.softAPIP());

                // Configure the webserver
                __web_server.on(
                    "/generate_204",
                    HTTP_GET,
                    std::bind(&WiFiManager::__web_root, this));
                __web_server.on(
                    "/",
                    HTTP_GET,
                    std::bind(&WiFiManager::__web_root, this));
                __web_server.on(
                    "/api/network_list",
                    HTTP_GET,
                    std::bind(&WiFiManager::__api_network_list, this));
                __web_server.on(
                    "/api/save_network",
                    HTTP_POST,
                    std::bind(&WiFiManager::__api_save_network, this));
                __web_server.on(
                    "/api/clear_saved_networks",
                    HTTP_GET,
                    std::bind(&WiFiManager::__api_clear_saved_networks, this));

                // Start the webserver
                __web_server.begin();
            }

            void WiFiManager::__web_root()
            {
                __web_server.send(200, "text/json", "Empty page");
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

            void WiFiManager::__api_save_network()
            {
                String ssid = __web_server.arg("ssid");
                String password = __web_server.arg("password");

                // Check if this network is already known
                for (const auto &network : __known_networks)
                {
                    if (network.ssid == ssid.c_str())
                    {
                        __web_server.send(200, "text/json", "{ \"saved\": false, \"reason\": 1 }");
                        return;
                    }
                }

                std::lock_guard<std::mutex> lock(__wifi_lock);
                // Test the WiFi connection. Connection should be done within
                // 10 seconds or it will fail
                WiFi.begin(ssid.c_str(), password.c_str());
                // TODO: Make connection-timeout configurable
                WiFi.waitForConnectResult(10000);

                if (WiFi.isConnected())
                {
                    // Could connect to this WiFi successfully
                    WiFi.disconnect();

                    // Add network to known networks
                    WiFiNetwork network;
                    network.ssid = ssid.c_str();
                    network.password = password.c_str();
                    __known_networks.emplace_back(network);

                    // Save credentials to flash
                    std::stringstream field_ssid;
                    uint16_t index = __known_networks.size() - 1;
                    field_ssid << "ssid_" << index;
                    std::stringstream field_password;
                    field_password << "password_" << index;
                    __preferences.putUInt("count", __known_networks.size());
                    __preferences.putString(field_ssid.str().c_str(), ssid);
                    __preferences.putString(field_password.str().c_str(), password);

                    __web_server.send(200, "text/json", "{ \"saved\": true, \"reason\": 0 }");
                    return;
                }

                // Couldn't connect
                __web_server.send(200, "text/json", "{ \"saved\": false, \"reason\": 0 }");
                return;
            }

            void WiFiManager::__api_clear_saved_networks()
            {
                __preferences.clear();
                __web_server.send(200, "text/json", "{ \"cleard\": true }");
                return;
            }

            void WiFiManager::update_wifi_list()
            {
                while (true)
                {
                    // TODO: Make interval configurable
                    delay(5000);
                    // Set WiFi count to zero. We increase it later to the
                    // number of WiFi networks
                    uint16_t ssid_count = 0;
                    {
                        std::lock_guard<std::mutex> lock(__wifi_lock);
                        Serial.println("Scanning WiFi");

                        // Get the WiFi networks
                        ssid_count = WiFi.scanNetworks(false, true);
                        WiFi.mode(WIFI_AP_STA);
                        Serial.println("Scanning WiFi is done");
                    };

                    __networks.clear();
                    for (uint16_t ssid_id = 0; ssid_id < ssid_count; ssid_id++)
                    {
                        __networks.push_back(
                            {WiFi.SSID(ssid_id).c_str(),
                             WiFi.BSSIDstr(ssid_id).c_str(),
                             WiFi.channel(ssid_id),
                             WiFi.RSSI(ssid_id),
                             WiFi.encryptionType(ssid_id),
                             ""});
                    }
                }
            }
        };
    };
};