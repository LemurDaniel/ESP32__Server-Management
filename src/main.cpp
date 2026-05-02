#include <Arduino.h>

#include <../lib/server/server.h>

#include <routes/routes.example.h>
#include <routes/routes.server.manager.h>

void setup()
{
  Serial.begin(115200);

  ESP32WebServer::MiniServer *Server = new ESP32WebServer::MiniServer();

  // Will start enter WiFi setup, if this function isn't used.
  // Credentials are permanently stored via LittleFs.
  // Server->connectWiFi("<SSID / Wlan Name >", "***<PASSWORD>***");

  // For testing purposes, remove WiFi config to trigger AP mode
  // Server->clearWiFi();

  // Hardcode default credentials (Can be set via Dashoard without hardcoding!)
  // Server->defaultAdminCredentials("admin", "admin");
  // Server->defaultAdminSalt("");

  // Disables admin routes entirly
  // Server->disableAdmin();

  Server->root("/web");
  Server->index("/web/index.html");

  Server->registerRouter(routes_example::Router());
  Server->registerRouter(routes_server_manager::Router());

  Server->start(80);
}

void loop()
{
  while (true)
  {
    delay(10);
  }
}
