// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#include <server.h>

/********** TCP Server Implementation **********/

namespace ESP32WebServer
{

    // Initialize static member variables
    ESP32WebServer::MiniServer *ESP32WebServer::MiniServer::_instance = nullptr;
    std::vector<ESP32WebServer::MiniServer::Connection> ESP32WebServer::MiniServer::connections;

    ESP32WebServer::MiniServer *MiniServer::instance()
    {
        if (_instance == nullptr)
        {
            _instance = new MiniServer();
        }
        return _instance;
    }

    /**
     ***********************************************
     ************************************************
     * Setting up the MiniServer with Constructor and Destructor:
     * - Create a socket
     * - Bind it to the specified IP and port
     * - Listen for incoming connections
     *
     **/
    MiniServer::MiniServer()
    {
        is_running = false;
    }
    MiniServer::~MiniServer()
    {
        closeServer();
    }

    void MiniServer::closeServer()
    {
        close(server_socket);
    }

    WiFiClass MiniServer::connectWiFi(const std::string &ssid, const std::string &password)
    {
        setupWiFi(ssid, password, true);
        return WiFi;
    }

    void MiniServer::clearWiFi()
    {
        clearWiFiConfig();
    }

    void MiniServer::disableAdmin()
    {
        is_admin_enabled = false;
    }

    void MiniServer::defaultAdminSalt(std::string salt)
    {
        setDefaultAdminSalt(salt);
    }

    void MiniServer::defaultAdminCredentials(std::string username, std::string password)
    {
        setDefaultAdminCredentials(username, password);
    }

    /************************************************
     ************************************************
     * Listening for Clients:
     * - Listen for incoming connections
     * - Accept the connection and get a new socket for communication
     * - Handle the client requests (index serving, GET/POST handling, etc.)
     *
     **/
    void MiniServer::handleClient(int client_socket)
    {
        char buffer[1024];

        int bytesRead = read(client_socket, buffer, sizeof(buffer) - 1);

        if (bytesRead <= 0)
        {
            Serial.printf("Failed to read from client socket %d\n", client_socket);
            return;
        }

        buffer[bytesRead] = '\0'; // Null-terminate the buffer to make it a valid C-string

        // Parse the raw HTTP request into a structured Request object
        Request request = Request::parse(buffer);
        Response response = Response();

        Serial.println("\n\n\n--- Raw HTTP Request ---");
        Serial.println(buffer);
        Serial.println("--------------------------");

        // Search for a matching route handler
        const std::string routeKey = request.method + " " + request.path;

        if (routes.find(routeKey) != routes.end())
        {
            // Retrieve the handler function for the matched route and execute it
            const auto route = routes[routeKey];
            for (const auto &handler : route)
            {
                handler(request, response);

                Serial.printf("Executed handler for route: %s\n", routeKey.c_str());
                Serial.printf("Response status code: %d\n", response.status_code);
                Serial.printf("Finalized: %s\n", response.finalized ? "true" : "false");

                if (response.finalized)
                {
                    break;
                }
            }

            if (response.responseMode == "file")
            {
                serveFile(client_socket, response);
            }
            else
            {
                std::string header = response.getHeaders();
                write(client_socket, header.c_str(), header.size());
                write(client_socket, response.body.c_str(), response.body.size());
            }
        }

        else
        {
            Serial.printf("No handler found for route: %s\n", routeKey.c_str());
            response.NotFound();
            std::string header = response.getHeaders();
            write(client_socket, header.c_str(), header.size());
            write(client_socket, response.body.c_str(), response.body.size());
        }
    }

    void MiniServer::serveFile(int client_socket, Response &res)
    {

        File file = LittleFS.open(res.filePath.c_str(), "r");

        const std::string header = res.getHeaders();
        write(client_socket, header.c_str(), header.size());

        char chunk[512];
        size_t totalSent = 0;

        file.seek(0); // Ensure we start from the beginning
        while (file.available() && totalSent < res.fileSize)
        {
            size_t n = file.readBytes(chunk, sizeof(chunk));
            if (n > 0)
            {
                write(client_socket, chunk, n);
                totalSent += n;
            }
            else
            {
                Serial.println("⚠️  WARNING: Read error while serving file!");
                break;
            }
        }

        file.close();
        Serial.printf("✅ File transfer completed: %zu bytes sent\n", totalSent);
    }

    /************************************************
     ************************************************
     * Serve index or handle GET/POST requests:
     *
     **/
    void MiniServer::index(const std::string &index_path)
    {
        staticFile("/", index_path);
        staticFile("/index", index_path);
        staticFile("/index.html", index_path);
    }

    void MiniServer::addRoute(const std::string &method, const std::string &path, std::vector<RequestHandler> handlers)
    {
        routes.insert({method + " " + path, handlers});
    }
    void MiniServer::addRoute(const std::string &method, const std::string &path, RequestHandler handler)
    {
        addRoute(method, path, std::vector<RequestHandler>{handler});
    }

    void MiniServer::staticFile(const std::string &path, const std::string &file_path)
    {
        Serial.printf("Adding file response: %s -> %s\n", path.c_str(), file_path.c_str());

        auto handler = [file_path](const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
        {
            std::string ext;
            auto dot = file_path.find_last_of('.');
            if (dot != std::string::npos)
            {
                ext = file_path.substr(dot);
            }

            res.file(file_path);

            if (ext == ".css")
                res.header("Content-Type", "text/css");
            else if (ext == ".html")
                res.header("Content-Type", "text/html; charset=utf-8");
        };
        addRoute("GET", path, handler);
    }

    void MiniServer::route(const std::string &method, const std::string &path, RequestHandler handler)
    {
        addRoute(method, path, handler);
    }

    void MiniServer::get(const std::string &path, RequestHandler handler)
    {
        addRoute("GET", path, handler);
    }

    void MiniServer::post(const std::string &path, RequestHandler handler)
    {
        addRoute("POST", path, handler);
    }

    void MiniServer::put(const std::string &path, RequestHandler handler)
    {
        addRoute("PUT", path, handler);
    }

    void MiniServer::patch(const std::string &path, RequestHandler handler)
    {
        addRoute("PATCH", path, handler);
    }

    void MiniServer::del(const std::string &path, RequestHandler handler)
    {
        addRoute("DELETE", path, handler);
    }

    void MiniServer::registerRouter(const ESP32WebServer::Router &router)
    {
        for (const auto &route : router.routes)
        {
            Serial.printf("Registering route: %s %s\n", route.method.c_str(), route.path.c_str());
            addRoute(route.method, route.path, route.handler);
        }
    }

    /************************************************
     ************************************************
     * Handle multiple connections and cleanup:
     *
     **/
    void MiniServer::workerTask(void *param)
    {

        MiniServer *server = static_cast<MiniServer *>(param);
        int client_socket;

        while (true)
        {
            // Hier wartet der Task, verbraucht 0% CPU währenddessen
            if (xQueueReceive(server->handleQueue, &client_socket, portMAX_DELAY))
            {
                // --- TIMEOUT SETUP START ---
                struct timeval tv;
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                // --- TIMEOUT SETUP END ---

                Serial.printf("Worker handling client on socket %d\n", client_socket);

                server->handleClient(client_socket);

                Serial.printf("Worker finished handling client on socket %d\n", client_socket);

                shutdown(client_socket, SHUT_RDWR);
                close(client_socket);
            }
        }
    }

    void MiniServer::dispatcherTask(void *param)
    {

        MiniServer *server = static_cast<MiniServer *>(param);

        while (true)
        {

            for (auto con = server->connections.begin(); con != server->connections.end();)
            {

                // const int current_sec = millis() / 1000;

                // if (current_sec - con->last_active_sec > CONNECTION_TIMEOUT_SEC)
                //{
                //     Serial.printf("Removing inactive connection on socket %d\n", con->socket);
                //     con = server->connections.erase(con);
                //     shutdown(con->socket, SHUT_RDWR);
                //     close(con->socket);
                // }
                // else
                //{
                Serial.printf("Dispatching client on socket %d\n", con->socket);
                xQueueSend(server->handleQueue, &con->socket, 0);
                con->last_active_sec = millis() / 1000;
                server->connections.erase(con);
                //}
            }

            vTaskDelay(100 / portTICK_PERIOD_MS); // Small delay to prevent CPU hogging
        }
    }
    void MiniServer::acceptClientTask(void *param)
    {

        // Accept is blocking, no further delay needed here.

        MiniServer *server = static_cast<MiniServer *>(param);
        const int server_socket = server->server_socket;

        while (true)
        {
            struct sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &len);

            if (client_socket < 0)
            {
                Serial.println("Failed to accept client connection");
                return;
            }
            else if (connections.size() >= ESP32WebServer::CONNECTION_LIMIT)
            {
                Serial.println("Maximum connections reached. Rejecting new client.");
                write(client_socket, "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 19\r\n\r\nService Unavailable", 75);
                close(client_socket);
            }
            else
            {
                Serial.printf("Accepted new client on socket %d\n", client_socket);
                Connection con;
                con.socket = client_socket;
                con.created_at_sec = millis() / 1000;
                con.last_active_sec = millis() / 1000;
                connections.push_back(con);
            }
        }
    }

    int MiniServer::start(std::string ip_addr, int port)
    {

        if (is_running)
        {
            Serial.println("Server is already running");
            return 0;
        }

        if (is_admin_enabled)
        {
            this->registerRouter(ESP32WebServer::AdminRouter());
        }

        if (!isWiFiConnected())
        {
            Serial.println("Starting WiFi setup...");
            setupWiFi();
        }

        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address_len = sizeof(address);
        if (inet_pton(AF_INET, ip_addr.c_str(), &address.sin_addr) <= 0)
        {
            Serial.println("Invalid IP address");
            return 1;
        }

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            Serial.println("Failed to create socket");
            return 1;
        }

        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            Serial.println("Failed to set socket options");
            return 1;
        }

        if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            Serial.println("Failed to bind socket");
            return 1;
        }

        if (listen(server_socket, 3) < 0)
        {
            Serial.println("Failed to listen on socket");
            return 1;
        }

        Serial.println("Starting accept client task...");
        xTaskCreatePinnedToCore(acceptClientTask, "accept", 8192, this, 1, NULL, 0);
        Serial.println("Starting dispatcher task...");
        xTaskCreatePinnedToCore(dispatcherTask, "dispatch", 8192, this, 1, NULL, 1);
        Serial.println("Starting worker tasks...");
        for (int i = 0; i < ESP32WebServer::WORKER_TASK_COUNT; i++)
        {
            std::string taskName = "worker" + std::to_string(i);
            xTaskCreatePinnedToCore(workerTask, taskName.c_str(), 8192, this, 1, NULL, i % 2);
        }

        Serial.println("Server started and listening for clients...");
        is_running = true;
        return 0;
    }

}