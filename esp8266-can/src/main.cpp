// #include <Arduino.h>

// #define CS    15   // D8
// #define SCK   14   // D5
// #define MISO  12   // D6
// #define MOSI  13   // D7

// #define RESET 0xC0
// #define READ  0x03
// #define WRITE 0x02

// #define CANSTAT 0x0E
// #define CANCTRL 0x0F

// uint8_t spiByte(uint8_t tx)
// {
//     uint8_t rx = 0;

//     for (int8_t i = 7; i >= 0; i--)
//     {
//         digitalWrite(MOSI, (tx & (1 << i)) ? HIGH : LOW);

//         // MCP2515 samples on rising edge
//         digitalWrite(SCK, HIGH);

//         rx <<= 1;
//         if (digitalRead(MISO))
//             rx |= 1;

//         digitalWrite(SCK, LOW);
//     }

//     return rx;
// }

// void csLow()
// {
//     digitalWrite(CS, LOW);
// }

// void csHigh()
// {
//     digitalWrite(CS, HIGH);
// }

// void resetMCP()
// {
//     csLow();
//     spiByte(RESET);
//     csHigh();

//     delay(10);
// }

// uint8_t readReg(uint8_t address)
// {
//     uint8_t value;

//     csLow();

//     spiByte(READ);
//     spiByte(address);
//     value = spiByte(0x00);

//     csHigh();

//     return value;
// }

// void writeReg(uint8_t address, uint8_t value)
// {
//     csLow();

//     spiByte(WRITE);
//     spiByte(address);
//     spiByte(value);

//     csHigh();
// }

// void setup()
// {
//     Serial.begin(115200);
//     delay(1000);

//     pinMode(CS, OUTPUT);
//     pinMode(SCK, OUTPUT);
//     pinMode(MOSI, OUTPUT);
//     pinMode(MISO, INPUT);

//     digitalWrite(CS, HIGH);
//     digitalWrite(SCK, LOW);
//     digitalWrite(MOSI, LOW);

//     Serial.println();
//     Serial.println("=== MCP2515 REGISTER TEST ===");

//     Serial.println("RESET");
//     resetMCP();

//     Serial.print("CANSTAT = 0x");
//     Serial.println(readReg(CANSTAT), HEX);

//     Serial.print("CANCTRL = 0x");
//     Serial.println(readReg(CANCTRL), HEX);

//     Serial.println();
//     Serial.println("WRITE CANCTRL = 0x87");

//     writeReg(CANCTRL, 0x87);

//     delay(2);

//     Serial.print("CANCTRL = 0x");
//     Serial.println(readReg(CANCTRL), HEX);
// }

// void loop()
// {
// }


#include <Arduino.h>

#define CS    15   // D8
#define SCK   14   // D5
#define MISO  12   // D6
#define MOSI  13   // D7

// ================= MCP2515 SPI COMMANDS =================

#define RESET       0xC0
#define READ        0x03
#define WRITE       0x02
#define RTS_TX0     0x81

// ================= MCP2515 REGISTERS ====================

#define CANSTAT     0x0E
#define CANCTRL     0x0F

#define CNF1        0x2A
#define CNF2        0x29
#define CNF3        0x28

#define TXB0CTRL    0x30
#define TXB0SIDH    0x31
#define TXB0SIDL    0x32
#define TXB0DLC     0x35
#define TXB0D0      0x36

#define RXB0CTRL    0x60
#define RXB0SIDH    0x61
#define RXB0SIDL    0x62
#define RXB0DLC     0x65
#define RXB0D0      0x66

// CANINTF
#define CANINTF     0x2C
#define RX0IF       0x01
#define TX0IF       0x04

// ================= SPI =================

uint8_t spiByte(uint8_t tx)
{
    uint8_t rx = 0;

    for (int8_t i = 7; i >= 0; i--)
    {
        digitalWrite(MOSI, (tx & (1 << i)) ? HIGH : LOW);

        // MCP2515 SPI mode 0
        digitalWrite(SCK, HIGH);

        rx <<= 1;

        if (digitalRead(MISO))
            rx |= 1;

        digitalWrite(SCK, LOW);
    }

    return rx;
}

void csLow()
{
    digitalWrite(CS, LOW);
}

void csHigh()
{
    digitalWrite(CS, HIGH);
}

// ================= MCP2515 BASIC COMMANDS ===============

void resetMCP()
{
    csLow();

    spiByte(RESET);

    csHigh();

    delay(10);
}

uint8_t readReg(uint8_t address)
{
    uint8_t value;

    csLow();

    spiByte(READ);
    spiByte(address);

    value = spiByte(0x00);

    csHigh();

    return value;
}

void writeReg(uint8_t address, uint8_t value)
{
    csLow();

    spiByte(WRITE);
    spiByte(address);
    spiByte(value);

    csHigh();
}

void writeRegs(uint8_t address, const uint8_t *data, uint8_t length)
{
    csLow();

    spiByte(WRITE);
    spiByte(address);

    for (uint8_t i = 0; i < length; i++)
    {
        spiByte(data[i]);
    }

    csHigh();
}

// ================= MCP2515 MODE ==========================

void setMode(uint8_t mode)
{
    uint8_t ctrl = readReg(CANCTRL);

    ctrl &= 0x1F;
    ctrl |= mode;

    writeReg(CANCTRL, ctrl);

    delay(10);
}

// MCP2515 mode bits
#define MODE_NORMAL      0x00
#define MODE_SLEEP       0x20
#define MODE_LOOPBACK    0x40
#define MODE_LISTENONLY  0x60
#define MODE_CONFIG      0x80

// ================= CAN CONFIG ============================
//
// MCP2515 8 MHz oscillator
// Target: 500 kbps
//
// CNF1 = 0x00
// CNF2 = 0xD1
// CNF3 = 0x81
//
// This gives:
//
// BRP = 0
// TQ = 2 * (BRP + 1) / Fosc
//     = 2 / 8MHz
//     = 250 ns
//
// 8 TQ/bit -> 500 kbps
//

void configure500K()
{
    setMode(MODE_CONFIG);

    writeReg(CNF1, 0x00);
    writeReg(CNF2, 0xD1);
    writeReg(CNF3, 0x81);

    Serial.println("CAN bitrate configured: 500 kbps");
}

// ================= TX ================================

void loadTXBuffer()
{
    uint16_t id = 0x123;

    uint8_t data[8] =
    {
        0x11,
        0x22,
        0x33,
        0x44,
        0x55,
        0x66,
        0x77,
        0x88
    };

    // Standard 11-bit ID
    uint8_t sidh = id >> 3;
    uint8_t sidl = (id & 0x07) << 5;

    writeReg(TXB0SIDH, sidh);
    writeReg(TXB0SIDL, sidl);

    // No extended ID
    writeReg(TXB0DLC, 8);

    writeRegs(TXB0D0, data, 8);

    Serial.println("TX buffer loaded");

    Serial.print("ID   = 0x");
    Serial.println(id, HEX);

    Serial.print("DATA = ");

    for (uint8_t i = 0; i < 8; i++)
    {
        if (data[i] < 0x10)
            Serial.print("0");

        Serial.print(data[i], HEX);
        Serial.print(" ");
    }

    Serial.println();
}

// ================= REQUEST TRANSMISSION ==================

void requestTX0()
{
    csLow();

    spiByte(RTS_TX0);

    csHigh();

    Serial.println("TX request sent");
}

// ================= RX ================================

bool rxAvailable()
{
    uint8_t intf = readReg(CANINTF);

    return (intf & RX0IF);
}

void readRXBuffer()
{
    uint8_t sidh = readReg(RXB0SIDH);
    uint8_t sidl = readReg(RXB0SIDL);

    uint16_t id =
        ((uint16_t)sidh << 3) |
        ((sidl >> 5) & 0x07);

    uint8_t dlc = readReg(RXB0DLC) & 0x0F;

    Serial.println();
    Serial.println("========== RX FRAME ==========");

    Serial.print("ID   = 0x");

    if (id < 0x100)
        Serial.print("0");

    Serial.println(id, HEX);

    Serial.print("DLC  = ");
    Serial.println(dlc);

    Serial.print("DATA = ");

    for (uint8_t i = 0; i < dlc; i++)
    {
        uint8_t value = readReg(RXB0D0 + i);

        if (value < 0x10)
            Serial.print("0");

        Serial.print(value, HEX);
        Serial.print(" ");
    }

    Serial.println();
    Serial.println("==============================");
}

// ================= LOOPBACK TEST ========================

bool runLoopbackTest()
{
    Serial.println();
    Serial.println("================================");
    Serial.println("       MCP2515 LOOPBACK TEST");
    Serial.println("================================");

    // Reset
    Serial.println("Resetting MCP2515...");
    resetMCP();

    Serial.print("CANSTAT after reset = 0x");
    Serial.println(readReg(CANSTAT), HEX);

    Serial.print("CANCTRL after reset = 0x");
    Serial.println(readReg(CANCTRL), HEX);

    // Configure CAN
    Serial.println();
    Serial.println("Entering configuration mode...");

    setMode(MODE_CONFIG);

    Serial.print("CANSTAT = 0x");
    Serial.println(readReg(CANSTAT), HEX);

    // Configure 500 kbps
    configure500K();

    // Clear interrupts
    writeReg(CANINTF, 0x00);

    // Configure RXB0 to receive everything
    //
    // RXM1:RXM0 = 00
    // Receive all valid messages
    //
    writeReg(RXB0CTRL, 0x60);

    // Enter loopback mode
    Serial.println();
    Serial.println("Entering LOOPBACK mode...");

    setMode(MODE_LOOPBACK);

    uint8_t ctrl = readReg(CANCTRL);

    Serial.print("CANCTRL = 0x");
    Serial.println(ctrl, HEX);

    uint8_t stat = readReg(CANSTAT);

    Serial.print("CANSTAT = 0x");
    Serial.println(stat, HEX);

    // Check mode
    if ((stat & 0xE0) != MODE_LOOPBACK)
    {
        Serial.println("ERROR: MCP2515 did not enter loopback mode!");
        return false;
    }

    Serial.println("Loopback mode OK");

    // Load TX
    loadTXBuffer();

    // Transmit
    requestTX0();

    Serial.println("Waiting for RX...");

    // Wait up to 1 second
    unsigned long start = millis();

    while (millis() - start < 1000)
    {
        if (rxAvailable())
        {
            Serial.println("RX message received!");

            readRXBuffer();

            return true;
        }

        delay(1);
    }

    Serial.println("ERROR: RX timeout");

    Serial.print("CANINTF = 0x");
    Serial.println(readReg(CANINTF), HEX);

    Serial.print("TXB0CTRL = 0x");
    Serial.println(readReg(TXB0CTRL), HEX);

    return false;
}

// ================= SETUP ================================

void setup()
{
    Serial.begin(115200);

    delay(1000);

    pinMode(CS, OUTPUT);
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);
    pinMode(MISO, INPUT);

    digitalWrite(CS, HIGH);
    digitalWrite(SCK, LOW);
    digitalWrite(MOSI, LOW);

    Serial.println();
    Serial.println("================================");
    Serial.println("       MCP2515 REGISTER TEST");
    Serial.println("================================");

    // Basic register test
    Serial.println("RESET");

    resetMCP();

    Serial.print("CANSTAT = 0x");
    Serial.println(readReg(CANSTAT), HEX);

    Serial.print("CANCTRL = 0x");
    Serial.println(readReg(CANCTRL), HEX);

    Serial.println();

    Serial.println("WRITE CANCTRL = 0x87");

    writeReg(CANCTRL, 0x87);

    delay(2);

    Serial.print("CANCTRL = 0x");
    Serial.println(readReg(CANCTRL), HEX);

    // Run loopback
    bool result = runLoopbackTest();

    Serial.println();

    if (result)
    {
        Serial.println("********************************");
        Serial.println("* MCP2515 LOOPBACK PASS        *");
        Serial.println("********************************");
    }
    else
    {
        Serial.println("********************************");
        Serial.println("* MCP2515 LOOPBACK FAIL        *");
        Serial.println("********************************");
    }
}

void loop()
{
}