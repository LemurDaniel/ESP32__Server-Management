

#include <Arduino.h>
#include <WiFi.h>

#include <../lib/server/router.h>

namespace routes_example
{
    class Router : public ESP32WebServer::Router
    {
    public:
        Router()
        {
            route("GET", "/hello", get_hello);
            route("GET", "/status", get_status);
            route("GET", "/example", get_example);
            route("POST", "/data", post_data);
        }

    private:
        static void get_hello(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void get_example(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
        static void post_data(const ESP32WebServer::Request &req, ESP32WebServer::Response &res);
    };

}