# Arayüz Sözleşmesi v1.0 — Verici Etiket İçin Uygulama Rehberi

Bu belge, soğuk zincir BLE sıcaklık etiketinin (verici) yayınlaması gereken reklam paketi formatını tanımlar. Format, alıcı alt sistemi (ESP32 ağ geçidi) tarafından belirlenmiştir. Bu formata uyan etiketin paketleri ağ geçidi tarafından çözülür, kaydedilir ve alarm değerlendirmesine katılır.

## 1. Paket düzeni

Reklam paketinde bir adet "üreticiye özel veri" (Manufacturer Specific Data, AD tipi 0xFF) alanı bulunur. Bu alanın içeriği 12 bayttır:

| Bayt | Alan | Tip | Değer / birim |
|---|---|---|---|
| 0–1 | Company ID | uint16 | 0xFFFF |
| 2 | Etiket kimliği | uint8 | Sabit, 1–254 |
| 3–4 | Sıra numarası | uint16 | Her yeni ölçümde +1; 65535'ten sonra 0 |
| 5–6 | Sıcaklık | int16 | 0,01 °C (4,25 °C = 425); sensör hatası: −32768 |
| 7–10 | Zaman damgası | uint32 | `millis()`, ms |
| 11 | Pil | uint8 | %0–100; bilinmiyor: 255 |

**Bayt sırası:** Little-endian (çok baytlı değerlerde önce düşük bayt).

**Test vektörü:** Etiket kimliği 1, sıra numarası 2, sıcaklık 4,25 °C, zaman damgası 0x11223344, pil %100 için alan içeriği:

```
FF FF 01 02 00 A9 01 44 33 22 11 64
```

Etiketin ürettiği baytlar bu örnekle karşılaştırılarak format doğrulanabilir.

## 2. Kurallar

1. **Reklam türü:** Bağlanılamaz, yalnızca yayın (non-connectable, non-scannable).
2. **Sıra numarası:** Yalnızca yeni bir sıcaklık ölçümü yapıldığında artırılır. Aynı ölçüm birden fazla reklam olayında tekrar yayınlanabilir.
3. **Zaman damgası:** Paket oluşturulduğu anda `millis()` değeri yazılır. Gerçek tarih-saat (RTC) gerekmez.
4. **Ölçüm ve yayın aralığı:** En fazla 10 dakika (WHO PQS E006/TR06.4 ölçüm aralığı şartı). Aralık, verici projenin pil bütçesine göre seçilir.
5. **Sensör hatası:** Sensör okunamazsa paket yine gönderilir ve sıcaklık alanına −32768 yazılır.
6. **Pil:** Pil yüzdesi hesaplanamıyorsa 255 gönderilir.

## 3. Örnek kod (Seeed XIAO nRF52840, Arduino)

Aşağıdaki örnek Seeed nRF52 (mbed olmayan, Adafruit Bluefruit tabanlı) Arduino çekirdeği için yazılmıştır ve **donanım üzerinde doğrulanmamıştır**. `contract.h` dosyası ([firmware/common/contract.h](../firmware/common/contract.h)) Arduino'ya bağımlı değildir ve sketch klasörüne kopyalanarak kullanılabilir.

```cpp
#include <bluefruit.h>
#include "contract.h"

uint16_t seq = 0;

int16_t readTemperatureCenti() {
  // Sensör okuma kodu buraya (ör. TMP117). Hata durumunda contract::TEMP_INVALID döndür.
  return contract::TEMP_INVALID;
}

uint8_t readBatteryPercent() {
  return contract::BATT_UNKNOWN;   // pil ölçümü eklenince değiştir
}

void publish() {
  contract::TagPacket p;
  p.tag_id      = 1;
  p.seq         = seq++;
  p.temp_centi  = readTemperatureCenti();
  p.tx_ms       = millis();
  p.battery_pct = readBatteryPercent();

  uint8_t raw[31];
  contract::encode_adv_payload(p, raw);       // raw[5..16] = 12 baytlık üreticiye özel veri

  Bluefruit.Advertising.stop();
  Bluefruit.Advertising.clearData();
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addData(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, raw + 5, contract::MFG_DATA_LEN);
  Bluefruit.Advertising.start(0);
}

void setup() {
  Serial.begin(115200);
  Bluefruit.begin();
  Bluefruit.setTxPower(0);                    // 0 dBm
  Bluefruit.Advertising.setType(BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED);
  Bluefruit.Advertising.setInterval(1600, 1600);   // 0,625 ms birimi: 1600 = 1000 ms
  Bluefruit.Advertising.restartOnDisconnect(false);
}

void loop() {
  publish();
  delay(1000);   // ölçüm aralığı; pil bütçesine göre değiştirilir (en fazla 10 dk)
}
```

## 4. Uyumluluk testi

Etiket bu formatı uyguladıktan sonra ağ geçidiyle birlikte şu kontroller yapılır:
- Ağ geçidinin seri kaydında etiketin paketleri `RX` satırları olarak görünüyor mu?
- Etiketin kendi seri çıktısındaki sıcaklık değeri, ağ geçidi kaydındaki değerle aynı mı?
- Sıra numarası her ölçümde 1 artıyor mu?

Telefonla ön kontrol için nRF Connect uygulaması kullanılabilir: Uygulamada etiketin "Manufacturer data" alanı yukarıdaki düzenle görünmelidir.
