

#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>

#include <router.h>

namespace routes_server_manager
{
    class ServerState {
        public:
            static ServerState *instance();

            struct Status
            {
                int isOnline = false;
            };
            volatile int isShuttingDown = false;
            volatile int isPoweringUp = false;
            volatile int isOnline = false;
            volatile Status gitea;
            volatile Status grafana;
            volatile Status gucacamole;

        private:
            static ServerState *_instance;
    };

    class Router : public ESP32WebServer::Router
    {
    public:
        Router()
        {
            route("GET", "/server/shutdown", get_shutdown);
            route("GET", "/server/startup", get_startup);
            route("GET", "/server/status", get_status);

            xTaskCreatePinnedToCore(statusTask, "status", 8192, ServerState::instance(), 1, NULL, 0);
        }

    private:
        static void statusTask(void *params);

        static void get_shutdown(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_startup(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
    };

}