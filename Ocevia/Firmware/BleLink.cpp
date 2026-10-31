#include "BleLink.h"
#include "Config.h"
#include "Relays.h"
#include "Sensors.h"
#include "Settings.h"
#include "Utils.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

static BLECharacteristic *statusChar = nullptr;
static QueueHandle_t cmdQueue = nullptr;

static volatile bool connected = false;
static volatile bool statusPending = false;
static volatile bool restartAdv = false;
static volatile uint32_t restartAt = 0;
static uint32_t lastNotify = 0;

// Callback BLE jalan di task lain: HANYA set flag / masukkan ke queue.
// Semua perubahan state relay & NVS dikerjakan di loop().
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    connected = true;
    statusPending = true;
    Serial.println("BLE: CONNECTED");
  }

  void onDisconnect(BLEServer *server) override {
    connected = false;
    restartAt = millis() + 100;
    restartAdv = true;
    Serial.println("BLE: DISCONNECTED");
  }
};

class CharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    auto value = characteristic->getValue();  // std::string (core 2.x) / String (core 3.x)
    if (value.length() == 0 || cmdQueue == nullptr) return;

    char buf[CMD_MAX_LEN];
    strlcpy(buf, value.c_str(), sizeof(buf));
    xQueueSend(cmdQueue, buf, 0);
  }
};

static void buildStatus(char *out, size_t size) {
  char tempStr[8], dateStr[12], timeStr[10];
  char fanOn[8], fanOff[8], lampOn[8], lampOff[8];

  if (tempValid()) snprintf(tempStr, sizeof(tempStr), "%.1f", tempValue());
  else strcpy(tempStr, "--.-");

  bool rtcGood = rtcOk();
  if (rtcGood) {
    ClockTime c = rtcTime();
    snprintf(dateStr, sizeof(dateStr), "%04u-%02u-%02u",
             (unsigned)c.year, (unsigned)c.month, (unsigned)c.day);
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u",
             (unsigned)c.hour, (unsigned)c.minute, (unsigned)c.second);
  } else {
    strcpy(dateStr, "---- -- --");
    strcpy(timeStr, "--:--:--");
  }

  formatClock(settings.lampOn, lampOn, sizeof(lampOn));
  formatClock(settings.lampOff, lampOff, sizeof(lampOff));
  snprintf(fanOn, sizeof(fanOn), "%.1f", settings.fanOn);
  snprintf(fanOff, sizeof(fanOff), "%.1f", settings.fanOff);

  // Key lama dipertahankan agar kompatibel dengan aplikasi; SENSOR ditambah di akhir.
  snprintf(out, size,
           "TEMP=%sC;DATE=%s;TIME=%s;"
           "R1=%s;R1_MODE=%s;R1_TIMER=%s;R1_REMAIN=%lu;R1_ON_TEMP=%s;R1_OFF_TEMP=%s;"
           "R2=%s;R2_MODE=%s;R2_TIMER=%s;R2_REMAIN=%lu;R2_SCHEDULE=%s-%s;"
           "RTC=%s;SENSOR=%s",
           tempStr, dateStr, timeStr,
           relayState(CH_FAN) ? "ON" : "OFF",
           relayIsAuto(CH_FAN) ? "AUTO" : "MANUAL",
           relayTimerActive(CH_FAN) ? "ON" : "OFF",
           (unsigned long)relayRemainingSeconds(CH_FAN), fanOn, fanOff,
           relayState(CH_LAMP) ? "ON" : "OFF",
           relayIsAuto(CH_LAMP) ? "AUTO" : "MANUAL",
           relayTimerActive(CH_LAMP) ? "ON" : "OFF",
           (unsigned long)relayRemainingSeconds(CH_LAMP), lampOn, lampOff,
           rtcGood ? "OK" : "ERROR",
           tempFault() ? "ERROR" : "OK");
}

static void sendStatus(bool verbose) {
  if (!connected || statusChar == nullptr) return;

  char buf[360];
  buildStatus(buf, sizeof(buf));
  statusChar->setValue(buf);
  statusChar->notify();

  if (verbose) {
    Serial.print("BLE STATUS: ");
    Serial.println(buf);
  }
}

void bleBegin() {
  cmdQueue = xQueueCreate(CMD_QUEUE_LENGTH, CMD_MAX_LEN);

  BLEDevice::init(BLE_DEVICE_NAME);
  BLEDevice::setMTU(BLE_MTU);  // status ~230 byte, MTU default 23 hanya muat 20 byte

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(BLE_SERVICE_UUID);
  statusChar = service->createCharacteristic(
    BLE_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY);

  statusChar->addDescriptor(new BLE2902());
  statusChar->setCallbacks(new CharCallbacks());
  statusChar->setValue("Ocevia Ready");

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void bleRequestStatus() {
  statusPending = true;
}

bool bleDequeueCommand(char *buf, size_t size) {
  if (cmdQueue == nullptr || size < CMD_MAX_LEN) return false;
  return xQueueReceive(cmdQueue, buf, 0) == pdTRUE;
}

void bleUpdate() {
  uint32_t t = millis();

  if (restartAdv && (int32_t)(t - restartAt) >= 0) {
    restartAdv = false;
    BLEDevice::startAdvertising();
    Serial.println("BLE: ADVERTISING");
  }

  if (!connected) return;
  if (t - lastNotify < STATUS_MIN_GAP_MS) return;

  if (statusPending) {
    statusPending = false;
    lastNotify = t;
    sendStatus(true);
  } else if (t - lastNotify >= STATUS_NOTIFY_MS) {
    lastNotify = t;
    sendStatus(false);
  }
}
