// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <ArduinoJson-v7.4.3.h>
#include <string>
#include <map>

namespace ESP32WebServer
{
    class Request
    {
    public:
        std::string method;
        std::string path;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;
        std::string bodyRaw; // Raw body as string
        JsonDocument body;   // Parsed JSON body (if applicable)

        static std::map<std::string, std::string> parseCookies(const std::string &cookieHeader)
        {
            std::map<std::string, std::string> cookies;
            size_t start = 0;
            while (start < cookieHeader.length())
            {
                size_t end = cookieHeader.find(';', start);
                if (end == std::string::npos)
                    end = cookieHeader.length();

                std::string cookie = cookieHeader.substr(start, end - start);
                size_t eqPos = cookie.find('=');
                if (eqPos != std::string::npos)
                {
                    std::string key = cookie.substr(0, eqPos);
                    std::string value = cookie.substr(eqPos + 1);
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    cookies[key] = value;
                }
                start = end + 1;
            }
            return cookies;
        }

        static Request parse(std::string requestRaw)
        {
            // fetch first line of request: "GET /path HTTP/1.1"
            std::string requestLine = requestRaw.substr(0, requestRaw.find("\r\n"));

            // split request line into method and path
            size_t firstSpace = requestLine.find(' ');
            size_t secondSpace = requestLine.find(' ', firstSpace + 1);

            std::string method = requestLine.substr(0, firstSpace);
            std::string path = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

            // --- Extract headers ---
            std::map<std::string, std::string> cookies = {};
            std::map<std::string, std::string> headers = {};
            size_t pos = requestRaw.find("\r\n") + 2; // Start after the request line
            while (true)
            {
                size_t endOfLine = requestRaw.find("\r\n", pos);
                if (endOfLine == std::string::npos || endOfLine == pos)
                    break; // End of headers

                std::string headerLine = requestRaw.substr(pos, endOfLine - pos);
                size_t colonPos = headerLine.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string key = headerLine.substr(0, colonPos);
                    std::string value = headerLine.substr(colonPos + 1);
                    // Trim whitespace
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);

                    headers[key] = value;
                }
                pos = endOfLine + 2;
            }

            // --- Process Request Headers and Body ---
            Request request;
            request.method = method;
            request.path = path;
            request.bodyRaw = "";
            request.headers = headers;
            request.cookies = cookies;

            if (headers.find("Cookie") != headers.end())
            {
                request.cookies = parseCookies(headers["Cookie"]);
            }

            // Body extrahieren (falls vorhanden)
            size_t headerEnd = requestRaw.find("\r\n\r\n");
            if (headerEnd != std::string::npos)
            {
                request.bodyRaw = requestRaw.substr(headerEnd + 4);
                if (
                    request.headers.find("Content-Type") != request.headers.end() &&
                    request.headers["Content-Type"] == "application/json")
                {
                    request.bodyRaw = requestRaw.substr(headerEnd + 4);
                    deserializeJson(request.body, request.bodyRaw);
                }
            }

            return request;
        }

    private:
    };
}