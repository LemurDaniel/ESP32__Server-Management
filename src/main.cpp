#include <Arduino.h>

#include <MiniServer.h>

#include <routes/routes.server.manager.h>

void setup()
{
  Serial.begin(115200);

  EspWeb::MiniServer *Server = new EspWeb::MiniServer();

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

  // Server->root("/web");
  // Server->index("/web/index.html");

  Server->registerRouter(routes_server_manager::Router());

  // Uhrzeit per NTP, Zeitzone Berlin inkl. Sommerzeit. Braucht der Nacht-Zeitplan im Router.
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");

  Server->dns("dev"); // dev.local
  // Server->dns("web"); // web.local

  Server->start(80);
}

void loop()
{
  vTaskDelete(NULL); // Loop-Task komplett entfernen, FreeRTOS gibt Ressourcen frei
}
