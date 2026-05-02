

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

            HTTPClient http;
            WiFiClientSecure client;

            // Ignore Self-Signed SSL Certificate
            client.setInsecure();
            http.begin(client, "https://nfs.fritz.box");

            int statusCode = http.GET();
            STATE->isOnline = statusCode > 0;
            Serial.print("https://nfs.fritz.box: ");
            Serial.println(statusCode);
            http.end();

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

            http.begin(client, "https://nfs.fritz.box/grafana");
            STATE->grafana.isOnline = http.GET() > 0;
            http.end();

            http.begin(client, "https://nfs.fritz.box/guacamole");
            STATE->gucacamole.isOnline = http.GET() > 0;
            http.end();

            http.begin(client, "https://nfs.fritz.box/gitea");
            STATE->gitea.isOnline = http.GET() > 0;
            http.end();

            vTaskDelay(15 * 1000 / portTICK_PERIOD_MS);
        }
    }

    void Router::get_shutdown(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
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

        HTTPClient http;
        WiFiClientSecure client;

        // Ignore Self-Signed SSL Certificate
        client.setInsecure();

        http.begin(client, "https://nfs.fritz.box/api/exec/shutdown/");

        // http.addHeader("Authorization", "Basic TODO");

        if (http.GET() == 200)
        {
            STATE->isShuttingDown = true;
            res.OK().text("Sucessfully send shutdown");
        }
        else
        {
            res.status(http.GET()).text("Something went wrong");
        }

        http.end();
    }

    void Router::get_startup(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
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

        const std::string macAddress = "10:FF:E0:85:15:8B"; // Hardcoded nfs.fritz.box MAC Adress

        byte mac[6];
        // MAC-String in Byte-Array umwandeln
        sscanf(macAddress.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

        byte packet[102];

        // 1. Teil: 6 Bytes mit 0xFF (Sync Stream)
        for (int i = 0; i < 6; i++)
        {
            packet[i] = 0xFF;
        }

        // 2. Teil: Die MAC-Adresse 16 Mal wiederholen
        for (int i = 1; i <= 16; i++)
        {
            memcpy(&packet[i * 6], mac, 6);
        }

        // Paket per UDP an die Broadcast-Adresse senden (Port 9 ist Standard für WOL)
        IPAddress broadcastIP(192, 168, 178, 255);

        WiFiUDP udp;
        udp.beginPacket(broadcastIP, 9);
        udp.write(packet, sizeof(packet));
        udp.endPacket();

        STATE->isPoweringUp = true;
        res.OK().text("🪄 Magic Packet was sent");
    }

    void Router::get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        ServerState *STATE = ServerState::instance();

        JsonDocument status;

        status["online"] = STATE->isOnline;
        status["poweringUp"] = STATE->isPoweringUp;
        status["shuttingDown"] = STATE->isShuttingDown;
        status["gitea"]["online"] = STATE->gitea.isOnline;
        status["grafana"]["online"] = STATE->grafana.isOnline;
        status["gucacamole"]["online"] = STATE->gucacamole.isOnline;

        res.json(status).status(200);
    }
}