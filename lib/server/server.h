// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <WiFi.h>
#include <Arduino.h>
#include <LittleFS.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>

#include <vector>
#include <string>
#include <map>

#include <ArduinoJson-v7.4.3.h>

#include <utility.wifi.h>
#include <utility.admin.h>
#include <router.h>

namespace ESP32WebServer
{

    const int WORKER_TASK_COUNT = 4;
    const int CONNECTION_LIMIT = 5;
    const int CONNECTION_TIMEOUT_SEC = 5;

    class MiniServer
    {
    public:
        static ESP32WebServer::MiniServer *instance(); // Get the singleton instance

        int start(std::string ip_addr, int port);

        // Connect to WiFi network via SSID (Name of WiFi) and password
        // If not used, the server will start in AP mode with SSID "ESP32_MiniWebServer" and a default admin page for WiFi configuration
        WiFiClass connectWiFi(const std::string &ssid, const std::string &password);
        void clearWiFi();

        // This is a blocking call that listens for incoming client connections and handles them
        // May be executed on a different Thread or Core to avoid blocking the main loop
        void listenClient();

        // Serve a static file as index.html on the root path
        void index(const std::string &index_path);

        // Add a static file response for a specific path
        void staticFile(const std::string &path, const std::string &file_path);

        // Register routes with method, path and handler function
        void registerRouter(const ESP32WebServer::Router &router);
        void route(const std::string &method, const std::string &path, RequestHandler handler);

        // Quick functions 
        void get(const std::string &path, RequestHandler handler);
        void post(const std::string &path, RequestHandler handler);
        void put(const std::string &path, RequestHandler handler);
        void patch(const std::string &path, RequestHandler handler);
        // delete is a keyword in c++, hence using del
        void del(const std::string &path, RequestHandler handler);


    private:
        static ESP32WebServer::MiniServer *_instance; // Singleton instance for static task functions
        MiniServer();
        ~MiniServer();

        struct sockaddr_in address;
        unsigned int address_len;
        int server_socket;
        void closeServer();

        int is_running = false;
        int is_admin_enabled = true;
        // Disables the admin dashboard entirly
        void disableAdmin();
        // Overrides the default admin credentials
        void defaultAdminSalt(std::string salt);
        void defaultAdminCredentials(std::string username, std::string password);

        void handleClient(int client_socket);

        // Map of path to file path for static file serving
        void serveFile(int client_socket, Response &res);

        // Map of "METHOD PATH" to handler function for dynamic routes
        std::map<std::string, std::vector<RequestHandler>> routes;
        void addRoute(const std::string &method, const std::string &path, RequestHandler handler);
        void addRoute(const std::string &method, const std::string &path, std::vector<RequestHandler> handlers);

        // Connection Management
        struct Connection
        {
            int socket;

            uint8_t created_at_sec;
            uint8_t last_active_sec;
        };
        // Queue for incoming connections to be processed by worker threads
        QueueHandle_t handleQueue = xQueueCreate(WORKER_TASK_COUNT, sizeof(Connection));
        static void workerTask(void *param);

        static std::vector<Connection> connections;
        static void dispatcherTask(void *param);
        static void acceptClientTask(void *param);
        static void cleanupConnectionsTask(void *param);
    };
}