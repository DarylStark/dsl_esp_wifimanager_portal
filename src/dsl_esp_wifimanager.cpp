#include "dsl_esp_wifimanager.h"

namespace dsl
{
    namespace esp
    {
        namespace apps
        {
            WiFiManager::WiFiManager(unsigned long serial_baudrate /* = 9600 */)
                : __baudrate(serial_baudrate)
            {
            }

            void WiFiManager::setup()
            {
                // Start the Serial
                Serial.begin(__baudrate);

                // Start the setup for
                _setup();
            }

            void WiFiManager::loop()
            {
                _loop();

                Serial.println("LOOP");
                delay(200);
            }
        };
    };
};