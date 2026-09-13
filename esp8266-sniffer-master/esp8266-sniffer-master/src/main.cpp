#include <Arduino.h>

extern "C" {
    #include <user_interface.h>
}

#include <stdio.h>
#include <string.h>

#define DATA_LENGTH             112

#define TYPE_MANAGEMENT         0x00
#define TYPE_CONTROL            0x01
#define TYPE_DATA               0x02

#define SUBTYPE_PROBE_REQUEST   0x04

#define MAX_DEVICES             40

#define CHANNEL_HOP_INTERVAL_MS 1000
#define REPORT_INTERVAL_MS      5000

#define DISABLE                 0
#define ENABLE                  1


/* ============================================================
 * ESP8266 PROMISCUOUS RX STRUCTURES
 * ============================================================ */

struct RxControl {
    signed rssi:8;

    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;

    unsigned legacy_length:12;

    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;

    unsigned MCS:7;
    unsigned CWB:1;

    unsigned HT_length:16;

    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;

    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;

    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;

    unsigned channel:4;
    unsigned:12;
};


struct SnifferPacket {
    struct RxControl rx_ctrl;

    uint8_t data[DATA_LENGTH];

    uint16_t cnt;
    uint16_t len;
};


/* ============================================================
 * DEVICE INFORMATION
 * ============================================================ */

struct DeviceInfo {
    uint8_t mac[6];

    int8_t lastRSSI;

    uint32_t packetCount;

    uint32_t lastSeen;

    uint8_t channel;

    char ssid[33];
};


DeviceInfo devices[MAX_DEVICES];

uint16_t deviceCount = 0;

volatile uint32_t totalProbeRequests = 0;
volatile uint32_t totalManagementFrames = 0;

uint32_t channelPackets[14] = {0};


/* ============================================================
 * FUNCTION DECLARATIONS
 * ============================================================ */

static void sniffer_callback(
    uint8_t *buffer,
    uint16_t length
);

static void processPacket(
    SnifferPacket *packet,
    uint16_t length
);

static bool parseProbeRequest(
    SnifferPacket *packet,
    uint16_t length
);

static int findDevice(
    const uint8_t *mac
);

static int addDevice(
    const uint8_t *mac
);

static void printMAC(
    const uint8_t *mac
);

static void printSSID(
    const uint8_t *data,
    uint16_t offset,
    uint16_t length
);

static void channelHop(void);

static void printReport(void);


/* ============================================================
 * MAC COMPARISON
 * ============================================================ */

static bool macEqual(
    const uint8_t *a,
    const uint8_t *b
)
{
    for (int i = 0; i < 6; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}


/* ============================================================
 * PRINT MAC
 * ============================================================ */

static void printMAC(
    const uint8_t *mac
)
{
    char buffer[18];

    snprintf(
        buffer,
        sizeof(buffer),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0],
        mac[1],
        mac[2],
        mac[3],
        mac[4],
        mac[5]
    );

    Serial.print(buffer);
}


/* ============================================================
 * FIND DEVICE
 * ============================================================ */

static int findDevice(
    const uint8_t *mac
)
{
    for (uint16_t i = 0; i < deviceCount; i++) {

        if (macEqual(devices[i].mac, mac)) {
            return i;
        }
    }

    return -1;
}


/* ============================================================
 * ADD DEVICE
 * ============================================================ */

static int addDevice(
    const uint8_t *mac
)
{
    if (deviceCount >= MAX_DEVICES) {
        return -1;
    }

    memcpy(
        devices[deviceCount].mac,
        mac,
        6
    );

    devices[deviceCount].lastRSSI = 0;
    devices[deviceCount].packetCount = 0;
    devices[deviceCount].lastSeen = millis();
    devices[deviceCount].channel = 0;

    devices[deviceCount].ssid[0] = '\0';

    deviceCount++;

    return deviceCount - 1;
}


/* ============================================================
 * PRINT SSID SAFELY
 * ============================================================ */

static void printSSID(
    const uint8_t *data,
    uint16_t offset,
    uint16_t length
)
{
    for (uint16_t i = 0; i < length; i++) {

        uint8_t c = data[offset + i];

        if (c >= 32 && c <= 126) {
            Serial.write(c);
        }
        else {
            Serial.print('.');
        }
    }
}


/* ============================================================
 * PROCESS PROBE REQUEST
 * ============================================================ */

static bool parseProbeRequest(
    SnifferPacket *packet,
    uint16_t length
)
{
    /*
     * Minimum 802.11 management header is 24 bytes.
     */

    if (length < 24) {
        return false;
    }

    uint16_t frameControl =
        ((uint16_t)packet->data[1] << 8) |
        packet->data[0];


    uint8_t frameType =
        (frameControl >> 2) & 0x03;

    uint8_t frameSubType =
        (frameControl >> 4) & 0x0F;


    if (frameType != TYPE_MANAGEMENT) {
        return false;
    }


    if (frameSubType != SUBTYPE_PROBE_REQUEST) {
        return false;
    }


    /*
     * Probe request:
     *
     * 0-1   Frame control
     * 2-3   Duration
     * 4-9   Destination
     * 10-15 Source
     * 16-21 BSSID
     * 22-23 Sequence control
     * 24... Information Elements
     */


    if (length < 26) {
        return false;
    }


    const uint8_t *sourceMAC =
        &packet->data[10];


    int index = findDevice(sourceMAC);


    if (index < 0) {

        index = addDevice(sourceMAC);

        if (index < 0) {
            return false;
        }

        Serial.println();
        Serial.println("[NEW DEVICE]");
    }


    devices[index].lastRSSI =
        packet->rx_ctrl.rssi;

    devices[index].packetCount++;

    devices[index].lastSeen =
        millis();

    devices[index].channel =
        wifi_get_channel();


    /*
     * Probe-request information elements start
     * at byte 24.
     */

    uint16_t offset = 24;

    uint16_t ssidLength = 0;

    bool foundSSID = false;


    /*
     * Walk through Information Elements:
     *
     * Element ID
     * Length
     * Data
     */

    while (offset + 2 <= length &&
           offset + 2 <= DATA_LENGTH) {

        uint8_t elementID =
            packet->data[offset];

        uint8_t elementLength =
            packet->data[offset + 1];


        if (offset + 2 + elementLength > length) {
            break;
        }


        if (elementID == 0) {

            ssidLength = elementLength;

            if (ssidLength > 32) {
                ssidLength = 32;
            }

            memcpy(
                devices[index].ssid,
                &packet->data[offset + 2],
                ssidLength
            );

            devices[index].ssid[ssidLength] =
                '\0';

            foundSSID = true;

            break;
        }


        offset += 2 + elementLength;
    }


    totalProbeRequests++;

    uint8_t channel =
        wifi_get_channel();

    if (channel <= 13) {
        channelPackets[channel]++;
    }


    /*
     * Print packet information
     */

    Serial.print("[PROBE] ");

    printMAC(sourceMAC);

    Serial.print(
        " RSSI="
    );

    Serial.print(
        packet->rx_ctrl.rssi
    );

    Serial.print(
        " CH="
    );

    Serial.print(channel);

    Serial.print(
        " SSID="
    );


    if (foundSSID) {

        printSSID(
            (const uint8_t *)devices[index].ssid,
            0,
            strlen(devices[index].ssid)
        );

    }
    else {

        Serial.print(
            "<unknown>"
        );
    }


    Serial.println();

    return true;
}


/* ============================================================
 * PROCESS PACKET
 * ============================================================ */

static void processPacket(
    SnifferPacket *packet,
    uint16_t length
)
{
    if (length < 2) {
        return;
    }


    uint16_t frameControl =
        ((uint16_t)packet->data[1] << 8) |
        packet->data[0];


    uint8_t frameType =
        (frameControl >> 2) & 0x03;


    if (frameType == TYPE_MANAGEMENT) {

        totalManagementFrames++;

        parseProbeRequest(
            packet,
            length
        );
    }
}


/* ============================================================
 * PROMISCUOUS CALLBACK
 * ============================================================ */

static void ICACHE_FLASH_ATTR
sniffer_callback(
    uint8_t *buffer,
    uint16_t length
)
{
    if (buffer == nullptr) {
        return;
    }


    if (length == 0) {
        return;
    }


    SnifferPacket *packet =
        (SnifferPacket *)buffer;


    processPacket(
        packet,
        length
    );
}


/* ============================================================
 * CHANNEL HOPPING
 * ============================================================ */

static void channelHop(void)
{
    uint8_t channel =
        wifi_get_channel();


    channel++;


    if (channel > 13) {
        channel = 1;
    }


    wifi_set_channel(
        channel
    );
}


/* ============================================================
 * PERIODIC REPORT
 * ============================================================ */

static void printReport(void)
{
    Serial.println();
    Serial.println(
        "========================================"
    );

    Serial.println(
        "       WIFI MONITOR REPORT"
    );

    Serial.println(
        "========================================"
    );


    Serial.print(
        "Total management frames: "
    );

    Serial.println(
        totalManagementFrames
    );


    Serial.print(
        "Probe requests: "
    );

    Serial.println(
        totalProbeRequests
    );


    Serial.print(
        "Unique devices: "
    );

    Serial.println(
        deviceCount
    );


    Serial.println();
    Serial.println(
        "DEVICE TABLE"
    );

    Serial.println(
        "----------------------------------------"
    );


    for (uint16_t i = 0;
         i < deviceCount;
         i++) {

        printMAC(
            devices[i].mac
        );

        Serial.print(
            "  RSSI="
        );

        Serial.print(
            devices[i].lastRSSI
        );

        Serial.print(
            "  CH="
        );

        Serial.print(
            devices[i].channel
        );

        Serial.print(
            "  COUNT="
        );

        Serial.print(
            devices[i].packetCount
        );

        Serial.print(
            "  SSID="
        );


        if (devices[i].ssid[0] != '\0') {

            Serial.print(
                devices[i].ssid
            );

        }
        else {

            Serial.print(
                "<unknown>"
            );
        }


        Serial.println();
    }


    Serial.println();
    Serial.println(
        "CHANNEL ACTIVITY"
    );

    Serial.println(
        "----------------------------------------"
    );


    for (uint8_t ch = 1;
         ch <= 13;
         ch++) {

        Serial.print(
            "CH "
        );

        Serial.print(ch);

        Serial.print(
            ": "
        );

        Serial.println(
            channelPackets[ch]
        );
    }


    Serial.println(
        "========================================"
    );
}
static os_timer_t channelHop_timer;

/* ============================================================
 * SETUP
 * ============================================================ */

void setup()
{
    Serial.begin(115200);

    delay(100);


    Serial.println();
    Serial.println(
        "ESP8266 Wi-Fi Monitor"
    );


    /*
     * Station mode
     */

    wifi_set_opmode(
        STATION_MODE
    );


    /*
     * Start on channel 1
     */

    wifi_set_channel(1);


    /*
     * Disable promiscuous mode
     * before configuring callback.
     */

    wifi_promiscuous_enable(
        DISABLE
    );


    delay(10);


    wifi_set_promiscuous_rx_cb(
        sniffer_callback
    );


    delay(10);


    wifi_promiscuous_enable(
        ENABLE
    );


    /*
     * Reset statistics
     */

    memset(
        devices,
        0,
        sizeof(devices)
    );


    memset(
        channelPackets,
        0,
        sizeof(channelPackets)
    );


    /*
     * Configure channel hopping.
     */

   os_timer_disarm(
        &channelHop_timer
    );

    os_timer_setfn(
        &channelHop_timer,
        (os_timer_func_t *)channelHop,
        NULL
    );

    os_timer_arm(
        &channelHop_timer,
        CHANNEL_HOP_INTERVAL_MS,
        1
    );


    Serial.println(
        "[SYSTEM] Promiscuous mode enabled"
    );

    Serial.println(
        "[SYSTEM] Channel hopping enabled"
    );
}


/* ============================================================
 * LOOP
 * ============================================================ */

void loop()
{
    static uint32_t lastReport = 0;


    uint32_t now = millis();


    if (now - lastReport >= REPORT_INTERVAL_MS) {

        lastReport = now;

        printReport();
    }


    delay(10);
}