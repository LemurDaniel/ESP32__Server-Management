

#include <routes/routes.example.h>

namespace routes_example
{
    void Router::get_hello(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        res.text("Hello, World! This is a simple response from the ESP32.").status(200);
    }

    void Router::get_status(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        JsonDocument status;

        status["device"] = "ESP32";
        status["firmware"] = "1.0.0";
        status["uptime"] = static_cast<double>(millis());
        status["free_heap"] = static_cast<double>(ESP.getFreeHeap());
        status["wifi_rssi"] = static_cast<double>(WiFi.RSSI());

        res.json(status).status(200);
    }

    void Router::get_example(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        res.text("This is an example route!").status(200);
    }

    void Router::post_data(const ESP32WebServer::Request &req, ESP32WebServer::Response &res)
    {
        JsonDocument response;
        response["message"] = "Data received successfully";
        response["timestamp"] = millis();
        res.json(response).status(201);
    }

}