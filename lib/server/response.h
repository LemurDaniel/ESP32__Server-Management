// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

#pragma once

#include <ArduinoJson-v7.4.3.h>

#include <LittleFS.h>
#include <string>
#include <map>

namespace ESP32WebServer
{
    class Response
    {
    public:
        // Flag to indicate if the response has been finalized by some middleware or handler, preventing further modifications
        int finalized = false;

        Response& finalize()
        {
            this->finalized = true;
            return *this;
        }

        std::string responseMode = "body"; // "body" or "file"

        // HTTP status code for the response (e.g., 200 for OK, 404 for Not Found)
        int status_code = 200;

        // Custom headers to include in the response
        std::map<std::string, std::string> headers;

        // Default content type is text/plain, but can be set to application/json or others as needed
        std::string body;

        // For static file responses, this will be set to the file path to serve
        size_t fileSize;
        std::string filePath;

        Response& header(const std::string &key, const std::string &value)
        {
            headers[key] = value;
            return *this;
        }

        std::string getHeaders()
        {

            if (responseMode == "file")
            {
                this->header("Content-Length", std::to_string(fileSize));
            }
            else
            {
                this->header("Content-Length", std::to_string(body.size()));
            }

            std::string header = "HTTP/1.1 " + std::to_string(status_code) + "\r\n";

            for (const auto &h : headers)
            {
                header += h.first + ": " + h.second + "\r\n";
            }

            header += "Connection: close\r\n\r\n";

            return header;
        }

        /**
         **************************************************
         **************************************************
         * Common status codes:
         * 200 - OK
         * 404 - Not Found
         * 500 - Internal Server Error
         */

        Response& OK()
        {
            if (this->body.empty())
            {
                this->text("OK");
            }
            this->status_code = 200;
            return *this;
        }

        Response& NotFound()
        {
            if (this->body.empty())
            {
                this->text("Not Found");
            }
            this->status_code = 404;
            return *this;
        }

        Response& InternalServerError()
        {
            if (this->body.empty())
            {
                this->text("Internal Server Error");
            }
            this->status_code = 500;
            return *this;
        }

        Response& status(int status)
        {
            this->status_code = status;
            return *this;
        }

        /**
         **************************************************
         **************************************************
         * Response body helpers for different content types
         *
         */
        Response& file(const std::string &path)
        {
            this->binaryFile(path);

            // Set content to text, so browser displays instead of downloading.
            this->header("Content-Type", "text/html; charset=utf-8;");

            return *this;
        }

        Response& binaryFile(const std::string &path)
        {
            // Check if LittleFS is already mounted, if not, mount it
            if (!LittleFS.begin(true)) // Format if mount fails
            {
                Serial.println("❌ CRITICAL: LittleFS mount failed completely!");
                this->InternalServerError().text("Internal Server Error: Filesystem Unavailable");
                return *this;
            }

            // Try to open the requested file
            File file = LittleFS.open(path.c_str(), "r");
            if (!file)
            {
                Serial.println("❌ File not found - sending 404");
                this->NotFound().text("File Not Found");
                file.close();
                return *this;
            }

            // Check file size and existence
            if (file.size() == 0)
            {
                Serial.println("⚠️  WARNING: File size is 0 bytes!");
            }

            this->header("Content-Type", "application/octet-stream"); // Set content type for binary file
            this->fileSize = file.size();
            this->filePath = path;
            this->responseMode = "file";

            Serial.printf("✅ File '%s' is ready to be served (size: %d bytes)\n", path.c_str(), this->fileSize);

            file.close();
            return *this;
        }

        Response& text(const std::string &text)
        {
            this->body = text;
            this->responseMode = "body";
            this->header("Content-Type", "text/plain");
            return *this;
        }

        Response& json(JsonDocument bodyJson)
        {
            char body[2048];
            serializeJson(bodyJson, body, sizeof(body));

            this->body = body;
            this->responseMode = "body";
            this->header("Content-Type", "application/json");
            return *this;
        }

        Response& html(const std::string &htmlBody)
        {
            this->text(htmlBody);
            this->header("Content-Type", "text/html; charset=utf-8");
            return *this;
        }

    private:
    };
}