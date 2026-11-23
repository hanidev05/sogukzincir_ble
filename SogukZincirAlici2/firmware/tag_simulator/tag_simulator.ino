// tag_simulator.ino — Etiketi taklit eden ikinci ESP32 (yalnız test aracı)
//
// Arkadaşın etiketi hazır olmadan ölçüt (a), (b), (c) testlerini yapabilmek için
// arayüz sözleşmesine (contract.h) uygun reklam paketleri yayınlar.
// Sıcaklıklar gerçek sensörden değil, önceden bilinen senaryodan (scenario.h) gelir.
//
// Seri çıktı (115200 baud): gerçeğin kaydı (ground truth)
//   TX,seq,tx_ms,temp_centi,segment
//   SYNC,tx_ms
//
// Bağlantı (saat ofseti için): bu kartın SYNC_OUT_PIN'i -> ağ geçidinin SYNC_IN_PIN'i, GND -> GND.

#include <BLEDevice.h>
#include "esp_gap_ble_api.h"
#include "contract.h"
#include "scenario.h"

// ---------------- Ayarlar ----------------
#define TAG_ID          1
#define UPDATE_MS       1000   // her saniye yeni sıra numarası (senaryo 1 paket/s varsayar)
#define ADV_INTERVAL_MS 1000   // = UPDATE_MS -> her sıra numarası tek reklam olayında gider
                               // (ör. 200 yaparsan her paket ~5 kopya gider: tekrarlı yayın testi)
#define SYNC_OUT_PIN    4
#define SYNC_PERIOD_MS  10000  // 10 s'de bir SYNC darbesi
// -----------------------------------------

static esp_ble_adv_params_t advParams;
static uint16_t seq = 0;
static uint32_t k = 0;         // senaryonun kaçıncı saniyesi

static void publish() {
  const char* label = nullptr;
  contract::TagPacket p;
  p.tag_id      = TAG_ID;
  p.seq         = seq;
  p.temp_centi  = scenario::temp_at(k, &label);
  p.battery_pct = 100;
  p.tx_ms       = millis();    // gönderim zamanı (etiket saati)

  uint8_t raw[31];
  size_t n = contract::encode_adv_payload(p, raw);

  esp_ble_gap_stop_advertising();
  esp_ble_gap_config_adv_data_raw(raw, n);
  esp_ble_gap_start_advertising(&advParams);

  Serial.printf("TX,%u,%lu,%d,%s\n", p.seq, (unsigned long)p.tx_ms, p.temp_centi, label);
  seq++;
  k++;
}

void setup() {
  Serial.begin(115200);
  pinMode(SYNC_OUT_PIN, OUTPUT);
  digitalWrite(SYNC_OUT_PIN, LOW);

  BLEDevice::init("");

  uint16_t units = (uint16_t)(ADV_INTERVAL_MS / 0.625);   // BLE birimi: 0,625 ms
  memset(&advParams, 0, sizeof(advParams));
  advParams.adv_int_min       = units;
  advParams.adv_int_max       = units;
  advParams.adv_type          = ADV_TYPE_NONCONN_IND;      // bağlanılamaz, yalnız yayın
  advParams.own_addr_type     = BLE_ADDR_TYPE_PUBLIC;
  advParams.channel_map       = ADV_CHNL_ALL;              // 37, 38, 39
  advParams.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

  Serial.printf("INFO,simulator basladi, tag=%d, senaryo=%lu s\n", TAG_ID,
                (unsigned long)scenario::total_seconds());
}

void loop() {
  static uint32_t lastUpdate = 0, lastSync = 0;
  uint32_t now = millis();

  if (now - lastUpdate >= UPDATE_MS) {
    lastUpdate += UPDATE_MS;
    publish();
  }
  if (now - lastSync >= SYNC_PERIOD_MS) {
    lastSync += SYNC_PERIOD_MS;
    uint32_t t = millis();
    digitalWrite(SYNC_OUT_PIN, HIGH);
    delayMicroseconds(200);
    digitalWrite(SYNC_OUT_PIN, LOW);
    Serial.printf("SYNC,%lu\n", (unsigned long)t);
  }
}
