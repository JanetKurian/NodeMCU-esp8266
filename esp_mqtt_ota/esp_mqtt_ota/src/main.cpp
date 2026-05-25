#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// const char* ssid = "YOUR_WIFI";
// const char* password = "YOUR_PASS";
// const char* ssid = "BHIVE Wifi-Zone";
// const char* password = "BH!v3@2k25";

const char* ssid = "moto g85 5G_4516";
const char* password = "Moto1234x";
//const char* mqtt_server ="10.225.113.200";
//const char* mqtt_server = "192.168.1.10";
 const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

String firmwareURL = "https://raw.githubusercontent.com/JanetKurian/esp_mqtt_ota/main/firmware.bin";

#define FW_VERSION "1.1"

void checkForUpdate()
{
    WiFiClientSecure httpsClient;
    httpsClient.setInsecure();

    HTTPClient https;

  
   String versionURL =
"https://raw.githubusercontent.com/JanetKurian/esp_mqtt_ota/main/version.json";

    Serial.println("Checking for OTA Update...");
    Serial.print("Heap Before HTTPS: ");
    Serial.println(ESP.getFreeHeap());


    if (!https.begin(httpsClient, versionURL))
    {
        Serial.println("HTTPS Begin Failed");
        return;
    }

    int httpCode = https.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);

        https.end();
        return;
    }

    String payload = https.getString();

    Serial.println("Version JSON:");
    Serial.println(payload);

    //DynamicJsonDocument doc(512);
    JsonDocument doc;
  //deserializeJson(doc, payload);

    DeserializationError error =
        deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("JSON Parse Failed: ");
        Serial.println(error.c_str());

        https.end();
        return;
    }

    String latestVersion =
        doc["version"].as<String>();

    String firmwareURL =
        doc["firmware"].as<String>();

    Serial.print("Current Version: ");
    Serial.println(FW_VERSION);

    Serial.print("Latest Version: ");
    Serial.println(latestVersion);

    https.end();

    // ---------------- VERSION CHECK ----------------

    if (latestVersion == FW_VERSION)
    {
        Serial.println("Already Latest Firmware");
        return;
    }

    Serial.println("New Firmware Detected");

    // ---------------- OTA UPDATE ----------------

   // WiFiClientSecure otaClient;
   // otaClient.setInsecure();
   WiFiClient otaClient;

    // t_httpUpdate_return ret =
    //     ESPhttpUpdate.update(otaClient,
    //                          firmwareURL);
    ESPhttpUpdate.rebootOnUpdate(true);

ESPhttpUpdate.setFollowRedirects(
    HTTPC_STRICT_FOLLOW_REDIRECTS);

Serial.println("Firmware URL:");
Serial.println(firmwareURL);

ESPhttpUpdate.onStart([]() {
    Serial.println("OTA Start");
});

ESPhttpUpdate.onEnd([]() {
    Serial.println("OTA End");
});

ESPhttpUpdate.onProgress([](int cur, int total) {
    Serial.printf("OTA Progress: %d%%\n",
                  (cur * 100) / total);
});

ESPhttpUpdate.onError([](int err) {
    Serial.printf("OTA Error: %d\n", err);
});

// t_httpUpdate_return ret =
//     ESPhttpUpdate.update(
//         otaClient,
//         firmwareURL);
t_httpUpdate_return ret =
ESPhttpUpdate.update(
    otaClient,
    "10.225.113.129",
    8000,
    "/firmware.bin");

    switch (ret)
    {
        case HTTP_UPDATE_FAILED:

            Serial.printf("OTA Failed Error (%d): %s\n",
                          ESPhttpUpdate.getLastError(),
                          ESPhttpUpdate.getLastErrorString().c_str());

            break;

        case HTTP_UPDATE_NO_UPDATES:

            Serial.println("No OTA Updates");

            break;

        case HTTP_UPDATE_OK:

            Serial.println("OTA Update Success");

            break;
    }
}

void setup_wifi()
{
    delay(10);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.println(WiFi.localIP());

Serial.print("Heap After WiFi: ");
Serial.println(ESP.getFreeHeap());
}

// void performOTA(String url)
// {
//     WiFiClientSecure client;
//     client.setInsecure();

//     t_httpUpdate_return ret =
//         ESPhttpUpdate.update(client, url);

//     switch (ret)
//     {
//         case HTTP_UPDATE_FAILED:
//             Serial.printf("OTA Failed: %s\n",
//                           ESPhttpUpdate.getLastErrorString().c_str());
//             break;

//         case HTTP_UPDATE_OK:
//             Serial.println("OTA Success");
//             break;

//         case HTTP_UPDATE_NO_UPDATES:
//             Serial.println("No Updates");
//             break;
//     }
// }

// void callback(char* topic, byte* payload, unsigned int length)
// {
//     String msg = "";

//     for (unsigned int i = 0; i < length; i++)
//     {
//         msg += (char)payload[i];
//     }

//     Serial.println("Message received:");
//     Serial.println(msg);

//     // Payload example:
//     // http://192.168.1.10/fw.bin

//     performOTA(msg);
// }

// void reconnect()
// {
//     while (!client.connected())
//     {
//         Serial.println("Connecting MQTT...");

//         if (client.connect("ESP8266_OTA"))
//         {
//             Serial.println("Connected");

//             // client.subscribe("device/fw/update");
//             client.subscribe("janet/device001/fw/update");
//         }
//         else
//         {
//             Serial.print("Failed rc=");
//             Serial.println(client.state());

//             delay(2000);
//         }
//     }
// }

void callback(char* topic, byte* payload, unsigned int length)
{
    String msg = "";

    for (unsigned int i = 0; i < length; i++)
    {
        msg += (char)payload[i];
    }

    Serial.println("Message received:");
    Serial.println(msg);

    if (msg == "check")
    {
        checkForUpdate();
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.println("Connecting MQTT...");

        String clientId =
            "ESP8266_" + String(ESP.getChipId());

        if (client.connect(clientId.c_str()))
        {
            Serial.println("Connected");

            client.subscribe(
                "janet/device001/fw/update");
        }
        else
        {
            Serial.print("Failed rc=");
            Serial.println(client.state());

            delay(2000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    setup_wifi();

    client.setServer(mqtt_server, 1883);

    client.setCallback(callback);
}

unsigned long lastCheck = 0;
// void loop()
// {
//     // if (!client.connected())
//     // {
//     //     reconnect();
//     // }

//     //     if (millis() - lastCheck > 300000)
//     // {
//     //     lastCheck = millis();

//     //     checkForUpdate();
//     // }

//     // client.loop();
//     if (WiFi.status() != WL_CONNECTED)
// {
//     Serial.println("WiFi Lost");

//     WiFi.disconnect();
//     WiFi.begin(ssid, password);

//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print(".");
//     }

//     Serial.println("WiFi Reconnected");
// }
// }

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi Lost");

        WiFi.disconnect();
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        Serial.println("WiFi Reconnected");
    }

    if (!client.connected())
    {
        reconnect();
    }

    client.loop();
}