#include "ble_control.h"
#include "state.h"
#include "color_engine.h"
#include "actions.h"
#include "settings.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BLE_SERVICE_UUID        "12345678-1234-1234-1234-1234567890AB"
#define BLE_CHARACTERISTIC_UUID "12345678-1234-1234-1234-1234567890AC"
#define BLE_CMD_MAX 128

static BLECharacteristic *bleCharacteristic;
static QueueHandle_t bleCmdQueue = NULL; 

// =====================================================
//   PLAY / PAUSE / NEXT / PREV   -> kontrol musik
//   OF:MATRIX / ON:MATRIX        -> matrix LED off / on
//   ABOUT                        -> About di OLED 10 detik
//   T:TEKS                       -> running text
//   B:0-255                      -> brightness
//   M:NAMA_MODE                  -> pilih mode
//   C:NAMA_MODE:R,G,B            -> warna solid
//   C:NAMA_MODE:RAINBOW|PEAKHUE|FLOW|PULSE|CYCLE
//   C:NAMA_MODE:RESET            -> warna bawaan
// =====================================================
static void handleBLECommand(String cmd)
{
    cmd.trim();

    Serial.print("BLE CMD: ");
    Serial.println(cmd);

    String up = cmd;
    up.toUpperCase();

    if      (up == "PLAY")      { actionPlay();         return; }
    else if (up == "PAUSE")     { actionPause();        return; }
    else if (up == "NEXT")      { actionNext();         return; }
    else if (up == "PREV")      { actionPrev();         return; }
    else if (up == "OF:MATRIX") { actionMatrix(false);  return; }
    else if (up == "ON:MATRIX") { actionMatrix(true);   return; }
    else if (up == "ABOUT")     { aboutActive = true; aboutStart = millis(); return; }

    if (cmd.length() < 2 || cmd.charAt(1) != ':')
    {
        Serial.println("CMD tidak valid");
        return;
    }

    char type = toupper(cmd.charAt(0));
    String arg = cmd.substring(2);

    // T:TEKS
    if (type == 'T')
    {
        runningText = arg;

        if (runningText.length() == 0)
            runningText = "VELORA";

        settingsMarkDirty();

        Serial.print("Running text: ");
        Serial.println(runningText);
    }

    // B:0-255
    else if (type == 'B')
    {
        actionBrightness(arg.toInt());
    }

    // M:NAMA_MODE
    else if (type == 'M')
    {
        arg.trim();
        arg.toUpperCase();

        int m = parseMode(arg);

        if (m < 0)
        {
            Serial.println("Mode tidak dikenal");
            return;
        }

        actionSetMode(m);
    }

    // C:NAMA_MODE:VALUE
    else if (type == 'C')
    {
        int sep = arg.indexOf(':');

        if (sep <= 0)
        {
            Serial.println("Format: C:MODE:VALUE");
            return;
        }

        String name = arg.substring(0, sep);
        String val  = arg.substring(sep + 1);

        name.trim();
        name.toUpperCase();
        val.trim();
        val.toUpperCase();

        int m = parseMode(name);

        if (m < 0)
        {
            Serial.println("Mode tidak dikenal");
            return;
        }

        if      (val == "RESET")   modeColor[m].type = CT_ORIGINAL;
        else if (val == "RAINBOW") modeColor[m].type = CT_RAINBOW;
        else if (val == "PEAKHUE") modeColor[m].type = CT_PEAKHUE;
        else if (val == "FLOW")    modeColor[m].type = CT_FLOW;
        else if (val == "PULSE")   modeColor[m].type = CT_PULSE;
        else if (val == "CYCLE")   modeColor[m].type = CT_CYCLE;
        else
        {
            // Solid: R,G,B
            int c1 = val.indexOf(',');
            int c2 = val.indexOf(',', c1 + 1);

            if (c1 <= 0 || c2 <= c1)
            {
                Serial.println("Warna tidak valid");
                return;
            }

            int r = constrain(val.substring(0, c1).toInt(), 0, 255);
            int g = constrain(val.substring(c1 + 1, c2).toInt(), 0, 255);
            int b = constrain(val.substring(c2 + 1).toInt(), 0, 255);

            modeColor[m].solid = CRGB(r, g, b);
            modeColor[m].type = CT_SOLID;
        }

        settingsMarkDirty();

        Serial.printf("Color %s -> type %d\n", MODE_NAMES[m], modeColor[m].type);
    }
}

void processBLEQueue()
{
    if (bleCmdQueue == NULL)
        return;

    char buf[BLE_CMD_MAX];

    while (xQueueReceive(bleCmdQueue, buf, 0) == pdTRUE)
    {
        handleBLECommand(String(buf));
    }
}

class BLEControlCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic)
    {
        String cmd = characteristic->getValue().c_str();

        if (cmd.length() > 0 && bleCmdQueue != NULL)
        {
            char buf[BLE_CMD_MAX] = {0};
            cmd.toCharArray(buf, BLE_CMD_MAX);       // terpotong kalau > 127 karakter

            xQueueSend(bleCmdQueue, buf, 0);
        }
    }
};

class BLEServerEvents : public BLEServerCallbacks
{
    void onConnect(BLEServer *server)
    {
        bleConnected = true;
    }

    void onDisconnect(BLEServer *server)
    {
        bleConnected = false;
        BLEDevice::startAdvertising();   // supaya bisa connect lagi
    }
};

void bleInit()
{
    bleCmdQueue = xQueueCreate(8, BLE_CMD_MAX);

    BLEDevice::init("Velora Control");
    BLEDevice::setMTU(185);

    BLEServer *bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new BLEServerEvents());

    BLEService *bleService = bleServer->createService(BLE_SERVICE_UUID);

    bleCharacteristic = bleService->createCharacteristic(
        BLE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR      // "write without response" juga diterima
    );

    bleCharacteristic->setCallbacks(new BLEControlCallbacks());

    bleService->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();

    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->setScanResponse(true);

    BLEDevice::startAdvertising();

    Serial.println("BLE Control ready");
}
