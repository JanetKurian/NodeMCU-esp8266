// #include <Arduino.h>

// String rxBuffer = "";

// void setup()
// {
//     Serial.begin(115200);

//     Serial.println();
//     Serial.println("ESP8266 Ready");
// }

// void loop()
// {
//     while (Serial.available())
//     {
//         char ch = Serial.read();

//         /* Echo received character */
//         Serial.print(ch);

//         /* Enter key detected */
//         if (ch == '\r' || ch == '\n')
//         {
//             rxBuffer.trim();

//             if (rxBuffer.length() > 0)
//             {
//                 Serial.println();

//                 Serial.print("Received: ");
//                 Serial.println(rxBuffer);

//                 if (rxBuffer == "AT")
//                 {
//                     Serial.println("OK");
//                 }
//                 else if (rxBuffer == "AT+LEDON")
//                 {
//                     Serial.println("LED ON");
//                     Serial.println("OK");
//                 }
//                 else
//                 {
//                     Serial.println("ERROR");
//                 }

//                 rxBuffer = "";
//             }
//         }
//         else
//         {
//             rxBuffer += ch;
//         }
//     }
// }

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

String rxBuffer = "";

/* WiFi + MQTT */
WiFiClient espClient;
PubSubClient client(espClient);

/* WiFi Credentials */
// const char* ssid = "BHIVE Wifi-Zone";
// const char* password = "BH!v3@2k25";

// /* MQTT Broker */
// const char* mqtt_server = "broker.hivemq.com";

/* WiFi Credentials */
String wifi_ssid     = "";
String wifi_password = "";

/* MQTT */
String mqtt_broker = "";
int mqtt_port = 1883;

/* ========================================= */
/* MQTT Connect */
/* ========================================= */

// void MQTT_Connect()
// {
//     while (!client.connected())
//     {
//         Serial.println("MQTT CONNECTING...");

//         if(client.connect("ESP8266Client"))
//         {
//             Serial.println("MQTT CONNECTED");
//         }
//         else
//         {
//             Serial.print("MQTT FAILED:");
//             Serial.println(client.state());

//             delay(2000);
//         }
//     }
// }
void MQTT_Connect()
{
   
        String clientId =
            "ESP8266_" + String(ESP.getChipId());

        Serial.println("MQTT CONNECTING...");

        if(client.connect(clientId.c_str()))
        {
            Serial.println("MQTT CONNECTED");
        }
        else
        {
            Serial.print("FAILED rc=");
            Serial.println(client.state());

            delay(2000);
        }
    
}

/* ========================================= */
/* Parse Commands */
/* ========================================= */

void ParseATCommand(String cmd)
{
    cmd.trim();

    /* ===================================== */
    /* Basic AT */
    /* ===================================== */

    if(cmd == "AT")
    {
        Serial.println("OK");
    }

    /* ===================================== */
    /* WiFi Mode */
    /* AT+CWMODE=1 */
    /* ===================================== */

    else if(cmd.startsWith("AT+CWMODE="))
    {
        Serial.println("OK");
    }

    /* ===================================== */
    /* Connect WiFi */
    /* AT+CWJAP="ssid","password" */
    /* ===================================== */

    else if(cmd.startsWith("AT+CWJAP="))
    {
        int firstQuote  = cmd.indexOf('"');
        int secondQuote = cmd.indexOf('"', firstQuote + 1);

        int thirdQuote  = cmd.indexOf('"', secondQuote + 1);
        int fourthQuote = cmd.indexOf('"', thirdQuote + 1);

        if(firstQuote > 0 && secondQuote > 0 &&
           thirdQuote > 0 && fourthQuote > 0)
        {
            wifi_ssid =
                cmd.substring(firstQuote + 1,
                              secondQuote);

            wifi_password =
                cmd.substring(thirdQuote + 1,
                              fourthQuote);

            Serial.println("WIFI CONNECTING...");

            WiFi.begin(wifi_ssid.c_str(),
                       wifi_password.c_str());

            while(WiFi.status() != WL_CONNECTED)
            {
                delay(500);
                Serial.print(".");
            }

            Serial.println();
            Serial.println("WIFI CONNECTED");
            Serial.println(WiFi.localIP());

            Serial.println("OK");
        }
        else
        {
            Serial.println("ERROR");
        }
    }

    /* ===================================== */
    /* MQTT Connect */
    /* AT+MQTTCONN="broker",1883 */
    /* ===================================== */

    else if(cmd.startsWith("AT+MQTTCONN="))
    {
        int firstQuote  = cmd.indexOf('"');
        int secondQuote = cmd.indexOf('"', firstQuote + 1);

        int commaIndex  = cmd.indexOf(',');

        mqtt_broker =
            cmd.substring(firstQuote + 1,
                          secondQuote);

        mqtt_port =
            cmd.substring(commaIndex + 1).toInt();

        client.setServer(mqtt_broker.c_str(),
                         mqtt_port);

        MQTT_Connect();

        Serial.println("OK");
    }

    /* ===================================== */
    /* MQTT Publish */
    /* AT+MQTTPUB="topic","message" */
    /* ===================================== */

    else if(cmd.startsWith("AT+MQTTPUB="))
    {
        int q1 = cmd.indexOf('"');
        int q2 = cmd.indexOf('"', q1 + 1);

        int q3 = cmd.indexOf('"', q2 + 1);
        int q4 = cmd.indexOf('"', q3 + 1);

        String topic =
            cmd.substring(q1 + 1, q2);

        String payload =
            cmd.substring(q3 + 1, q4);
        if(!client.connected())
            {
                MQTT_Connect();
            }

           // client.loop();

            delay(100);

        bool status =
            client.publish(topic.c_str(),
                           payload.c_str());

        if(status)
        {
            Serial.println("PUBLISH OK");
        }
        else
        {
            //Serial.println("PUBLISH FAIL");
            Serial.print("PUBLISH FAIL rc=");
            Serial.println(client.state());
        }

        Serial.println("OK");
    }

    else
    {
        Serial.println("ERROR");
    }
}

/* ========================================= */
/* Setup */
/* ========================================= */

void setup()
{
    Serial.begin(115200);

    Serial.println();
    Serial.println("ESP8266 CUSTOM AT READY");
}

/* ========================================= */
/* Loop */
/* ========================================= */

void loop()
{
    client.loop();

    while(Serial.available())
    {
        char ch = Serial.read();

        if(ch == '\n' || ch == '\r')
        {
            if(rxBuffer.length() > 0)
            {
                ParseATCommand(rxBuffer);

                rxBuffer = "";
            }
        }
        else
        {
            rxBuffer += ch;
        }
    }
}