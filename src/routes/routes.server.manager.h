

#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>

#include <router/router.h>

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

    class Router : public EspWeb::Router
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

        static void get_shutdown(const EspWeb::Request &req, EspWeb::Response &res);
        static void get_startup(const EspWeb::Request &req, EspWeb::Response &res);
        static void get_status(const EspWeb::Request &req, EspWeb::Response &res);
    };

}