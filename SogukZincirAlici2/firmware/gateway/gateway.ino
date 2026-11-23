// gateway.ino — Soğuk zincir BLE alıcı alt sistemi (ESP32 ağ geçidi)
//
// Görev: BLE reklam paketlerini tarar, arayüz sözleşmesine (contract.h) göre çözer,
// her paketi zaman damgasıyla seri porta CSV olarak yazar, WHO tabanlı eşik alarmını
// (alarm.h) çalıştırır ve saat ofseti ölçümü için SYNC darbelerini zamanlar.
//
// Kart: ESP32 DevKit (Arduino-ESP32 çekirdeği, yerleşik BLE kütüphanesi).
// Derleme: firmware/common/*.h dosyaları bu klasöre kopyalanmış olmalı (README: "Kod").
//
// Seri çıktı (115200 baud), satır türleri:
//   RX,gw_us,unix_ms,addr,rssi,tag_id,seq,temp_centi,tx_ms,batt
//   ALARM,gw_us,tag_id,HIGH|LOW,OPEN|CLOSE,since_gw_us
//   SYNC,gw_us
//   INFO,<metin>

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <sys/time.h>
#include "contract.h"
#include "alarm.h"

// ---------------- Ayarlar ----------------
#define ALARM_LED_PIN   2      // DevKit'lerde genelde dahili LED; kartına göre değiştir
#define SYNC_IN_PIN     4      // simülatörün SYNC çıkışı buraya (+ ortak GND)
#define SCAN_INTERVAL_MS 100   // tarama aralığı
#define SCAN_WINDOW_MS   100   // tarama penceresi = aralık -> %100 tarama
#define HYST_CENTI       50    // histerezis 0,50 °C
#define TIME_SCALE       60    // 1 = gerçek WHO süreleri; 60 = test (10 sa -> 10 dk, 60 dk -> 1 dk)
#define MAX_TAGS         8

// NTP isteğe bağlı. Boş bırakılırsa Wi-Fi açılmaz, unix_ms sütunu 0 yazılır.
// Not: ESP32'de Wi-Fi ve BLE aynı radyoyu paylaşır; kayıp testlerinde Wi-Fi kapalı ölçüm de yap.
#define WIFI_SSID ""
#define WIFI_PASS ""
#if __has_include(<WiFi.h>)
#include <WiFi.h>
#endif
// -----------------------------------------

struct RxItem {
  int64_t gw_us;
  int     rssi;
  char    addr[18];
  contract::TagPacket pkt;
};

static QueueHandle_t rxQueue;
static volatile int64_t syncTimes[16];
static volatile uint8_t syncHead = 0, syncTail = 0;

struct TagState {
  bool     used = false;
  uint8_t  tag_id = 0;
  bool     has_seq = false;
  uint16_t last_seq = 0;
  alarm::Detector* det = nullptr;
};
static TagState tags[MAX_TAGS];
static alarm::Config alarmCfg;

static int64_t now_us() { return esp_timer_get_time(); }

static int64_t unix_ms() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  if (tv.tv_sec < 1700000000) return 0;   // NTP ile ayarlanmamış
  return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void IRAM_ATTR onSync() {
  uint8_t next = (syncHead + 1) & 15;
  if (next != syncTail) { syncTimes[syncHead] = esp_timer_get_time(); syncHead = next; }
}

// BLE yığınının görevinde çalışır: yalnızca zaman damgası al, çöz ve kuyruğa at.
class ScanCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) override {
    int64_t t = now_us();
    RxItem it;
    if (!contract::decode_adv_payload(dev.getPayload(), dev.getPayloadLength(), it.pkt)) return;
    it.gw_us = t;
    it.rssi  = dev.getRSSI();
    String a = String(dev.getAddress().toString().c_str());
    strncpy(it.addr, a.c_str(), sizeof(it.addr) - 1);
    it.addr[sizeof(it.addr) - 1] = '\0';
    xQueueSend(rxQueue, &it, 0);   // kuyruk doluysa düşer; bu durum INFO ile raporlanır
  }
};

static TagState* tagFor(uint8_t id) {
  for (auto& t : tags) if (t.used && t.tag_id == id) return &t;
  for (auto& t : tags) if (!t.used) {
    t.used = true; t.tag_id = id; t.det = new alarm::Detector(alarmCfg); return &t;
  }
  return nullptr;
}

static void handle(const RxItem& it) {
  Serial.printf("RX,%lld,%lld,%s,%d,%u,%u,%d,%lu,%u\n",
                it.gw_us, unix_ms(), it.addr, it.rssi, it.pkt.tag_id, it.pkt.seq,
                it.pkt.temp_centi, (unsigned long)it.pkt.tx_ms, it.pkt.battery_pct);

  TagState* ts = tagFor(it.pkt.tag_id);
  if (!ts) return;
  // Aynı sıra numarasının tekrar kopyaları alarmı ikinci kez beslemez.
  if (ts->has_seq && contract::seq_delta(ts->last_seq, it.pkt.seq) == 0) return;
  ts->has_seq = true;
  ts->last_seq = it.pkt.seq;
  if (it.pkt.temp_centi == contract::TEMP_INVALID) return;   // sensör hatası: kaydedildi, alarma katılmaz

  uint32_t t_ms = (uint32_t)(it.gw_us / 1000);
  alarm::Event e = ts->det->update(it.pkt.temp_centi, t_ms);
  if (e.valid) {
    Serial.printf("ALARM,%lld,%u,%s,%s,%lld\n", it.gw_us, it.pkt.tag_id,
                  e.kind == alarm::Kind::HIGH_ALARM ? "HIGH" : "LOW",
                  e.edge == alarm::Edge::OPEN ? "OPEN" : "CLOSE",
                  e.edge == alarm::Edge::OPEN ? (long long)e.since_ms * 1000 : 0LL);
  }
  bool any = false;
  for (auto& t : tags) if (t.used && t.det->any_active()) any = true;
  digitalWrite(ALARM_LED_PIN, any ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(ALARM_LED_PIN, OUTPUT);
  pinMode(SYNC_IN_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(SYNC_IN_PIN), onSync, RISING);

  alarmCfg = alarm::who_type1(HYST_CENTI, TIME_SCALE);
  rxQueue = xQueueCreate(64, sizeof(RxItem));

#if __has_include(<WiFi.h>)
  if (strlen(WIFI_SSID) > 0) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; ++i) delay(250);
    configTime(3 * 3600, 0, "pool.ntp.org");
    Serial.println(WiFi.status() == WL_CONNECTED ? "INFO,wifi ok, ntp istendi" : "INFO,wifi baglanamadi");
  }
#endif

  BLEDevice::init("");
  BLEScan* scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new ScanCallback(), /*wantDuplicates=*/true);  // yinelenen paket filtresi KAPALI
  scan->setActiveScan(false);              // pasif tarama: yalnız dinle
  scan->setInterval(SCAN_INTERVAL_MS);
  scan->setWindow(SCAN_WINDOW_MS);
  scan->start(0, nullptr, false);          // 0 = süresiz tarama
  Serial.printf("INFO,gateway basladi, contract v%u, scale=%d, hyst=%d\n",
                contract::VERSION, TIME_SCALE, HYST_CENTI);
}

void loop() {
  RxItem it;
  while (xQueueReceive(rxQueue, &it, 0) == pdTRUE) handle(it);

  while (syncTail != syncHead) {
    Serial.printf("SYNC,%lld\n", (long long)syncTimes[syncTail]);
    syncTail = (syncTail + 1) & 15;
  }

  static uint32_t lastClear = 0;
  if (millis() - lastClear > 60000) {      // kütüphanenin sonuç listesini büyütmemek için
    BLEDevice::getScan()->clearResults();
    lastClear = millis();
  }
  delay(1);
}
