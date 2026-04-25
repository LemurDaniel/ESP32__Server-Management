// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// This is work in progress!

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>

#include <vector>
#include <string>

#include <utility.file.h>

#define WIFI_CONFIG_FILE "/WiFiConfig.json"

namespace ESP32WebServer
{

    // Will enter AP mode if connection cannot be established within this timeout.
    const int WIFI_TIMEOUT_SEC = 30;
    const std::string DEFAULT_WIFI_SSID = "ESP32_MiniWebServer";

    struct WiFiConfig
    {
        std::string ssid;
        std::string password;
        std::string signalStrength;
        std::string ipAddress;
    };

    inline int isApMode()
    {
        return WiFi.getMode() == WIFI_AP;
    }

    inline int isWiFiConnected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    inline void clearWiFiConfig()
    {
        removeFile(WIFI_CONFIG_FILE);
    }

    inline std::vector<WiFiConfig> scanNetworks()
    {
        std::vector<WiFiConfig> ssids;

        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i)
        {
            WiFiConfig config;
            config.ssid = WiFi.SSID(i).c_str();
            config.signalStrength = std::to_string(WiFi.RSSI(i)) + " dBm";
            config.ipAddress = ""; // IP address is not available during scan
            ssids.push_back(config);

            Serial.printf("Found network: %s (Signal: %s)\n", config.ssid.c_str(), config.signalStrength.c_str());
        }

        return ssids;
    }

    inline void setWiFiConfig(const std::string &ssid, const std::string &password)
    {
        JsonDocument wifiConfig;
        wifiConfig["ssid"] = ssid;
        wifiConfig["password"] = password;
        writeJsonFile(WIFI_CONFIG_FILE, wifiConfig);
    }

    inline WiFiConfig getWiFiConfig()
    {
        JsonDocument wifiConfig;

        if (fileExists(WIFI_CONFIG_FILE))
        {
            wifiConfig = readJsonFile(WIFI_CONFIG_FILE);
        }
        else
        {
            Serial.println("No WiFi config file found, returning default config...");
        }

        WiFiConfig config;
        config.ssid = wifiConfig["ssid"] | "Not Configured";
        config.password = wifiConfig["password"] | "";

        if (WiFi.status() == WL_CONNECTED)
        {
            config.signalStrength = std::to_string(WiFi.RSSI()) + std::string(" dBm");
            ;
            config.ipAddress = WiFi.localIP().toString().c_str();
        }
        else
        {
            config.signalStrength = wifiConfig["signalStrength"] | "0 dBm";
            config.ipAddress = wifiConfig["ipAddress"] | " Not Connected";
        }

        return config;
    }

    inline void setupWiFi(const std::string &ssid, const std::string &password, bool forceReconnect = false)
    {

        /**
         * If a password and SSID are provided:
         * - ESP32 will attempt to connect to the WiFi network using the provided credentials.
         *
         * If no password and SSID are provided:
         * - ESP32 will check if a WiFi config file exists on the device with valid
         *      If an WiFiConfig file exists
         *      - will attempt to connect to the WiFi network using the credentials in the file
         *
         *      else
         *      - will start in AP mode with SSID "ESP32_MiniWebServer".
         *        Default Credentials for admin page:
         *          Username: admin
         *          Password: admin
         *
         **/

        std::string ssidToUse = ssid;
        std::string passwordToUse = password;

        if (WiFi.status() == WL_CONNECTED && !forceReconnect)
        {
            Serial.println("Already connected to WiFi, skipping setup.");
            return;
        }

        if (fileExists(WIFI_CONFIG_FILE) && ssidToUse.empty() && passwordToUse.empty())
        {
            Serial.println("WiFi config file found, attempting to connect using stored credentials...");

            WiFiConfig wifiConfig = getWiFiConfig();
            ssidToUse = wifiConfig.ssid;
            passwordToUse = wifiConfig.password;
        }
        else if (ssidToUse.empty() && passwordToUse.empty())
        {
            Serial.println("No WiFi credentials provided and no config file found, starting in AP mode...");

            WiFi.mode(WIFI_AP);
            WiFi.softAP(DEFAULT_WIFI_SSID.c_str());
            Serial.printf("Local IP: %s\n", WiFi.softAPIP().toString().c_str());
            return;
        }
        else
        {
            Serial.println("WiFi credentials provided, writing to config file...");
            setWiFiConfig(ssidToUse, passwordToUse);
        }

        
        WiFi.begin(ssidToUse.c_str(), passwordToUse.c_str());

        const int timeStart = millis() / 1000;

        Serial.printf("\nSSID: %s\n", ssidToUse.c_str());
        Serial.print("Connecting to WiFi...");
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");

            const int timeNow = millis() / 1000;
            if (timeNow - timeStart >= WIFI_TIMEOUT_SEC)
            {
                Serial.println("Connection Failed! Timeout reached.");
                WiFi.mode(WIFI_AP);
                WiFi.softAP(DEFAULT_WIFI_SSID.c_str());
                Serial.printf("Local IP: %s\n", WiFi.softAPIP().toString().c_str());
                Serial.println("\nWiFi connection timed out, started in AP mode...");
                return;
            }
        }

        Serial.println("Connected!");
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.println();
    }
    inline void setupWiFi(const std::string &ssid, const std::string &password)
    {
        setupWiFi(ssid, password, false);
    }
    inline void setupWiFi()
    {
        setupWiFi("", "", false);
    }
}