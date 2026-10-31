

#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <time.h>

#include <router/router.h>

namespace routes_server_manager
{
    // ---------------------------------------------------------------
    // Nacht-Zeitplan: Server fuer die naechtlichen Jobs (Snapshots,
    // Scrub, Repo-Sync) wecken und danach wieder herunterfahren,
    // aber nur, wenn der ESP ihn selbst geweckt hat.
    // ---------------------------------------------------------------
    constexpr int SCHED_WAKE_HOUR = 22;       // 22:00 wecken, falls offline
    constexpr int SCHED_WAKE_MINUTE = 0;
    constexpr int SCHED_SHUTDOWN_HOUR = 4;    // ab 04:30 herunterfahren, wenn kein Job laeuft
    constexpr int SCHED_SHUTDOWN_MINUTE = 30;
    constexpr int SCHED_DEADLINE_HOUR = 7;    // spaetestens 07:00, auch wenn noch ein Job laeuft

    class ServerState {
        public:
            static ServerState *instance();

            volatile int isShuttingDown = false;
            volatile int isPoweringUp = false;
            volatile int isOnline = false;

            // Nachtlauf: true, wenn der ESP den Server per Zeitplan geweckt hat
            volatile int scheduledBoot = false;
            // true, waehrend der Server meldet, dass ein Wartungsjob laeuft
            volatile int maintenanceActive = false;
            // true, sobald NTP eine plausible Zeit geliefert hat
            volatile int timeSynced = false;
            // Tag (tm_yday), an dem zuletzt geweckt wurde, damit es nur einmal pro Nacht passiert
            volatile int lastWakeDay = -1;

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

            xTaskCreatePinnedToCore(statusTask, "status", 16384, ServerState::instance(), 1, NULL, 0);
            xTaskCreatePinnedToCore(scheduleTask, "schedule", 16384, ServerState::instance(), 1, NULL, 0);
        }

    private:
        static void statusTask(void *params);
        static void scheduleTask(void *params);

        static bool sendWakeOnLan();
        static int sendShutdown();
        static bool fetchMaintenanceActive();

        static void get_shutdown(const EspWeb::Request &req, EspWeb::Response &res);
        static void get_startup(const EspWeb::Request &req, EspWeb::Response &res);
        static void get_status(const EspWeb::Request &req, EspWeb::Response &res);
    };

}
