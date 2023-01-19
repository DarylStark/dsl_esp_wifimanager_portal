#include "dsl_esp_wifimanager_portal.h"

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            WiFiManagerPortal::WiFiManagerPortal(const std::string ap_ssid, unsigned long serial_baudrate /* = 9600 */)
                : __baudrate(serial_baudrate), __web_server(80), __ap_ssid(ap_ssid), __mode(Starting), __wifi_manager("dslespwifi")
            {
            }

            void WiFiManagerPortal::setup()
            {
                // Start the Serial
                Serial.begin(__baudrate);

                __mode = Setup;

                // Start the setup for
                _setup();

                // Start connecting
                setup_connecting();

                // Setup for portal
                if (__mode != Connected)
                    setup_portal();
            }

            void WiFiManagerPortal::loop()
            {
                if (__mode == Portal)
                {
                    loop_portal();
                    return;
                }
                loop_connecting();
            }

            void WiFiManagerPortal::setup_connecting()
            {
                __mode = Connecting;

                Serial.println("Setting up connection mode");

                __wifi_manager.load();
                for (const auto &network : __wifi_manager.get_wifi_list())
                {
                    Serial.printf("Connecting to network \"%s\" ... ", network.ssid.c_str());

                    __mode = Connecting;

                    // Connect to the network
                    WiFi.begin(network.ssid.c_str(), network.password.c_str());
                    // TODO: Make this timeout configurable
                    WiFi.waitForConnectResult(20000);

                    if (WiFi.isConnected())
                    {
                        __mode = Connected;
                        Serial.println("CONNECTED!");
                        return;
                    }
                    Serial.println("FAILED!");
                }
            }

            void WiFiManagerPortal::loop_connecting()
            {
            }

            void WiFiManagerPortal::setup_portal()
            {
                __mode = Portal;

                Serial.println("Setting up portal mode");

                // Start the WiFi scanner in different thread
                std::thread scan_thread(std::bind(&WiFiManagerPortal::scan_thread, this));
                scan_thread.detach();

                // Set the WiFi mode to AP
                WiFi.mode(WIFI_AP_STA);

                // Configure the Access Point
                // TODO: Make configurable
                WiFi.setHostname("DSL_ESP_WifiManager");

                // Start the AP
                WiFi.softAP(__ap_ssid.c_str(), NULL);

                // Start the DNS server
                __dns_server.start(53, "*", WiFi.softAPIP());

                // TODO: Rename API endpoints

                // Configure the webserver
                __web_server.on(
                    "/generate_204",
                    HTTP_GET,
                    std::bind(&WiFiManagerPortal::__web_root, this));
                __web_server.on(
                    "/",
                    HTTP_GET,
                    std::bind(&WiFiManagerPortal::__web_root, this));
                __web_server.on(
                    "/api/network_list",
                    HTTP_GET,
                    std::bind(&WiFiManagerPortal::__api_network_list, this));
                __web_server.on(
                    "/api/save_network",
                    HTTP_POST,
                    std::bind(&WiFiManagerPortal::__api_save_network, this));
                __web_server.on(
                    "/api/clear_saved_networks",
                    HTTP_GET,
                    std::bind(&WiFiManagerPortal::__api_clear_saved_networks, this));

                // TODO: Endpoint for known networks

                // Start the webserver
                __web_server.begin();
            }

            void WiFiManagerPortal::loop_portal()
            {
                __dns_server.processNextRequest();
                __web_server.handleClient();
            }

            void WiFiManagerPortal::__web_root()
            {
                __web_server.send(200, "text/html", "Empty page");
            }

            void WiFiManagerPortal::__api_network_list()
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

            void WiFiManagerPortal::__api_save_network()
            {
                String ssid = __web_server.arg("ssid");
                String password = __web_server.arg("password");

                // Test the WiFi connection. Connection should be done within
                // 10 seconds or it will fail
                std::lock_guard<std::mutex> lock(__wifi_lock);

                Serial.printf("Connecting to %s with password %s ... ", ssid.c_str(), password.c_str());
                WiFi.begin(ssid.c_str(), password.c_str());
                // TODO: Make connection-timeout configurable
                WiFi.waitForConnectResult(10000);

                if (WiFi.isConnected())
                {
                    // Could connect to this WiFi successfully
                    WiFi.disconnect();

                    Serial.println("CONNECTED");

                    // Save the network
                    __wifi_manager.set(std::string(ssid.c_str()), std::string(password.c_str()));
                    __wifi_manager.save();
                    __web_server.send(200, "text/json", "{ \"saved\": true, \"reason\": 0 }");
                    return;
                }

                Serial.println("FAILED");

                // Couldn't connect
                // TODO: Error code as return code
                __web_server.send(200, "text/json", "{ \"saved\": false, \"reason\": 0 }");
                return;
            }

            void WiFiManagerPortal::__api_clear_saved_networks()
            {
                __wifi_manager.clear();
                __web_server.send(200, "text/json", "{ \"cleard\": true }");
                return;
            }

            void WiFiManagerPortal::scan_thread()
            {
                delay(3000);
                while (true)
                {
                    // Set WiFi count to zero. We increase it later to the
                    // number of WiFi networks
                    uint16_t ssid_count = 0;
                    Serial.println("Scanning WiFi");

                    // Get the WiFi networks
                    {
                        std::lock_guard<std::mutex> lock(__wifi_lock);
                        ssid_count = WiFi.scanNetworks(false, true);
                        WiFi.mode(WIFI_AP_STA);
                    }
                    Serial.println("Scanning WiFi is done");

                    __networks.clear();
                    for (uint16_t ssid_id = 0; ssid_id < ssid_count; ssid_id++)
                    {
                        __networks.push_back(
                            {WiFi.SSID(ssid_id).c_str(),
                             "",
                             WiFi.BSSIDstr(ssid_id).c_str(),
                             WiFi.channel(ssid_id),
                             WiFi.RSSI(ssid_id),
                             WiFi.encryptionType(ssid_id)});
                    }

                    // TODO: Make interval configurable
                    delay(5000);
                }
            }

            uint16_t WiFiManagerPortal::get_status() const
            {
                return __mode;
            }
        };
    };
};