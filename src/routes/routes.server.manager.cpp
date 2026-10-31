#include <routes/routes.server.manager.h>

namespace routes_server_manager
{

    routes_server_manager::ServerState *ServerState::_instance = nullptr;
    routes_server_manager::ServerState *ServerState::instance()
    {
        if (_instance == nullptr)
        {
            _instance = new ServerState();
        }
        return _instance;
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Status-Task: prueft alle 15 s, ob der Server erreichbar ist
     *
     **/
    void Router::statusTask(void *param)
    {

        ServerState *STATE = static_cast<ServerState *>(param);

        while (true)
        {

            if (!WiFi.isConnected())
            {
                vTaskDelay(5 * 1000 / portTICK_PERIOD_MS);
                continue;
            }

            {
                HTTPClient http;
                WiFiClientSecure client;
                client.setInsecure();
                http.begin(client, "https://nfs.fritz.box");
                int statusCode = http.GET();
                STATE->isOnline = statusCode > 0;
                Serial.print("https://nfs.fritz.box: ");
                Serial.println(statusCode);
                http.end();
                client.stop();
            }

            if (STATE->isOnline && STATE->isPoweringUp)
            {
                STATE->isPoweringUp = false;
                STATE->isShuttingDown = false;
            }

            if (!STATE->isOnline && STATE->isShuttingDown)
            {
                STATE->isPoweringUp = false;
                STATE->isShuttingDown = false;
            }

            vTaskDelay(15 * 1000 / portTICK_PERIOD_MS);
        }
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Hilfsfunktionen: WOL, Shutdown, Wartungsstatus
     *
     **/
    bool Router::sendWakeOnLan()
    {
        const std::string macAddress = "10:FF:E0:85:15:8B"; // Hardcoded nfs.fritz.box MAC Adress

        byte mac[6];
        sscanf(macAddress.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

        byte packet[102];
        for (int i = 0; i < 6; i++)
        {
            packet[i] = 0xFF;
        }
        for (int i = 1; i <= 16; i++)
        {
            memcpy(&packet[i * 6], mac, 6);
        }

        IPAddress broadcastIP(192, 168, 178, 255);
        WiFiUDP udp;
        udp.beginPacket(broadcastIP, 9);
        udp.write(packet, sizeof(packet));
        return udp.endPacket() == 1;
    }

    int Router::sendShutdown()
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, "https://nfs.fritz.box/api/exec/shutdown/");
        int statusCode = http.GET();
        http.end();
        client.stop();
        return statusCode;
    }

    // Fragt express, ob ein Wartungsjob laeuft (Scrub, Repo-Sync, Backup).
    // Antwort enthaelt "maintenance":{"active":true|false,...}
    bool Router::fetchMaintenanceActive()
    {
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, "https://nfs.fritz.box/api/status");
        int statusCode = http.GET();
        bool active = false;
        if (statusCode == 200)
        {
            // express liefert kompaktes JSON: ..."maintenance":{"active":true,"jobs":[...]}...
            String body = http.getString();
            active = body.indexOf("\"maintenance\":{\"active\":true") >= 0;
        }
        http.end();
        client.stop();
        return active;
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Zeitplan-Task: prueft jede Minute die Uhrzeit
     *
     *  - 22:00  Server offline?  -> WOL senden, scheduledBoot = true
     *  - ab 04:30  scheduledBoot && online && kein Wartungsjob -> Shutdown
     *  - ab 07:00  scheduledBoot && online -> Shutdown, auch wenn noch ein Job laeuft
     *  - Manuelles Ein-/Ausschalten ueber das Dashboard loescht scheduledBoot
     *
     **/
    void Router::scheduleTask(void *param)
    {
        ServerState *STATE = static_cast<ServerState *>(param);

        while (true)
        {
            vTaskDelay(60 * 1000 / portTICK_PERIOD_MS);

            if (!WiFi.isConnected())
                continue;

            struct tm now;
            if (!getLocalTime(&now, 0) || now.tm_year + 1900 < 2024)
            {
                STATE->timeSynced = false;
                continue;
            }
            STATE->timeSynced = true;

            const int minutes = now.tm_hour * 60 + now.tm_min;
            const int wakeAt = SCHED_WAKE_HOUR * 60 + SCHED_WAKE_MINUTE;
            const int shutdownFrom = SCHED_SHUTDOWN_HOUR * 60 + SCHED_SHUTDOWN_MINUTE;
            const int deadline = SCHED_DEADLINE_HOUR * 60;

            // --- Wecken: im Fenster 22:00 bis 22:09, einmal pro Tag ---
            if (minutes >= wakeAt && minutes < wakeAt + 10 && STATE->lastWakeDay != now.tm_yday)
            {
                STATE->lastWakeDay = now.tm_yday;
                if (!STATE->isOnline)
                {
                    Serial.println("[schedule] Server offline, sende WOL");
                    if (sendWakeOnLan())
                    {
                        STATE->scheduledBoot = true;
                        STATE->isPoweringUp = true;
                    }
                }
                else
                {
                    Serial.println("[schedule] Server bereits online, kein Nachtlauf-Flag");
                }
            }

            // --- Herunterfahren: nur wenn der ESP geweckt hat ---
            if (STATE->scheduledBoot && minutes >= shutdownFrom && minutes < 12 * 60)
            {
                if (!STATE->isOnline)
                {
                    // wurde zwischenzeitlich anders heruntergefahren
                    STATE->scheduledBoot = false;
                    continue;
                }

                bool busy = fetchMaintenanceActive();
                STATE->maintenanceActive = busy;

                if (!busy || minutes >= deadline)
                {
                    Serial.printf("[schedule] Shutdown (%s)\n", busy ? "Deadline erreicht" : "kein Job aktiv");
                    int rc = sendShutdown();
                    if (rc == 200)
                    {
                        STATE->isShuttingDown = true;
                        STATE->scheduledBoot = false;
                    }
                }
                else
                {
                    Serial.println("[schedule] Wartungsjob laeuft, warte");
                }
            }
        }
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Request handlers for shutting down and booting up
     *
     **/

    void Router::get_shutdown(const EspWeb::Request &req, EspWeb::Response &res)
    {

        ServerState *STATE = ServerState::instance();

        if (!STATE->isOnline)
        {
            res.status(403).text("Server is already offline");
            return;
        }

        if (STATE->isPoweringUp)
        {
            res.status(403).text("Server is powering up");
            return;
        }

        int statusCode = sendShutdown();
        if (statusCode == 200)
        {
            STATE->isShuttingDown = true;
            STATE->scheduledBoot = false;
            res.OK().text("Sucessfully send shutdown");
        }
        else
        {
            res.status(statusCode).text("Something went wrong");
        }
    }

    void Router::get_startup(const EspWeb::Request &req, EspWeb::Response &res)
    {
        ServerState *STATE = ServerState::instance();

        if (STATE->isOnline)
        {
            res.status(403).text("Server is online");
            return;
        }

        if (STATE->isShuttingDown)
        {
            res.status(403).text("Server is shutting down");
            return;
        }

        sendWakeOnLan();
        STATE->isPoweringUp = true;
        STATE->scheduledBoot = false; // manuell gestartet: bleibt an
        res.OK().text("🪄 Magic Packet was sent");
    }

    /*-------------------------------------------------------------------------------------------------
     *
     * Get current status
     *
     **/
    void Router::get_status(const EspWeb::Request &req, EspWeb::Response &res)
    {
        ServerState *STATE = ServerState::instance();

        JsonDocument status;

        status["online"] = STATE->isOnline;
        status["poweringUp"] = STATE->isPoweringUp;
        status["shuttingDown"] = STATE->isShuttingDown;
        status["scheduledBoot"] = STATE->scheduledBoot;
        status["maintenanceActive"] = STATE->maintenanceActive;
        status["timeSynced"] = STATE->timeSynced;

        struct tm now;
        if (getLocalTime(&now, 0))
        {
            char buf[6];
            snprintf(buf, sizeof(buf), "%02d:%02d", now.tm_hour, now.tm_min);
            status["time"] = buf;
        }
        char sched[48];
        snprintf(sched, sizeof(sched), "wake %02d:%02d, shutdown %02d:%02d-%02d:00",
                 SCHED_WAKE_HOUR, SCHED_WAKE_MINUTE, SCHED_SHUTDOWN_HOUR, SCHED_SHUTDOWN_MINUTE, SCHED_DEADLINE_HOUR);
        status["schedule"] = sched;

        res.json(status).status(200);
    }
}
