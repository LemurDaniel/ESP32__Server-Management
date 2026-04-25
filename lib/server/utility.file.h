// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#include <ArduinoJson-v7.4.3.h>

namespace ESP32WebServer
{

    inline bool fileExists(const std::string &file_path)
    {
        if (!LittleFS.begin(true)) // Format if mount fails
        {
            Serial.println("❌ CRITICAL: LittleFS mount failed completely!");
            return false;
        }

        return LittleFS.exists(file_path.c_str());
    }

    inline int removeFile(const std::string &file_path)
    {
        if (fileExists(file_path))
        {
            if (LittleFS.remove(file_path.c_str()))
            {
                Serial.printf("✅ Successfully removed file %s\n", file_path.c_str());
                return 0;
            }
            else
            {
                Serial.printf("❌ CRITICAL: Failed to remove file %s!\n", file_path.c_str());
                return -1;
            }
        }
        else
        {
            Serial.printf("ℹ️ No file found at %s to remove.\n", file_path.c_str());
            return -1;
        }
    }

    inline JsonDocument readJsonFile(const std::string &file_path)
    {
        if (!fileExists(file_path))
        {
            Serial.printf("❌ CRITICAL: JSON file %s not found!\n", file_path.c_str());
            return JsonDocument();
        }

        File file = LittleFS.open(file_path.c_str(), "r");
        if (!file)
        {
            Serial.printf("❌ CRITICAL: Failed to open JSON file %s for reading!\n", file_path.c_str());
            return JsonDocument();
        }

        size_t size = file.size();
        std::string jsonStr(size, '\0');
        file.readBytes(&jsonStr[0], size);
        file.close();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonStr);
        if (error)
        {
            Serial.printf("❌ CRITICAL: Failed to parse JSON file %s! Error: %s\n", file_path.c_str(), error.c_str());
            return JsonDocument();
        }

        return doc;
    }

    inline bool writeJsonFile(const std::string &file_path, const JsonDocument &doc)
    {
        if (!LittleFS.begin(true)) // Format if mount fails
        {
            Serial.println("❌ CRITICAL: LittleFS mount failed completely!");
            return false;
        }

        File file = LittleFS.open(file_path.c_str(), "w");
        if (!file)
        {
            Serial.printf("❌ CRITICAL: Failed to open JSON file %s for writing!\n", file_path.c_str());
            return false;
        }

        char jsonStr[512];
        serializeJson(doc, jsonStr, sizeof(jsonStr));

        file.print(jsonStr);
        file.close();

        Serial.printf("✅ Successfully wrote JSON file %s\n", file_path.c_str());

        return true;
    }

}