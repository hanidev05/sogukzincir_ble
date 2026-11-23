// contract.h — Arayüz sözleşmesi v0.1 (BLE reklam paketi, üreticiye özel veri alanı)
//
// Hem alıcı (ESP32 ağ geçidi) hem verici (etiket / etiket simülatörü) bu dosyayı kullanır.
// Arduino bağımlılığı yoktur; bilgisayarda g++ ile test edilebilir (tests/test_core.cpp).
//
// Üreticiye özel veri alanı (AD tipi 0xFF) içeriği, toplam 12 bayt:
//
//   Bayt   Alan              Tip      Birim / çözünürlük
//   0-1    Company ID        uint16   0xFFFF (Bluetooth SIG: yalnız test/iç kullanım)
//   2      Etiket kimliği    uint8    —
//   3-4    Sıra numarası     uint16   — (65535'ten sonra 0'a döner)
//   5-6    Sıcaklık          int16    0,01 °C   (4,25 °C = 425)
//   7-10   Zaman damgası     uint32   1 ms, etiketin açılışından beri (~49,7 günde taşar)
//   11     Pil               uint8    %1 (0-100)
//
// Tüm çok baytlı alanlar little-endian'dır (önce düşük bayt).

#pragma once
#include <stdint.h>
#include <stddef.h>

namespace contract {

constexpr uint16_t COMPANY_ID   = 0xFFFF;
constexpr uint8_t  VERSION      = 1;      // sözleşme v0.1
constexpr size_t   MFG_DATA_LEN = 12;     // company ID dahil

struct TagPacket {
  uint8_t  tag_id;
  uint16_t seq;
  int16_t  temp_centi;   // 0,01 °C
  uint32_t tx_ms;        // etiket saatiyle gönderim anı
  uint8_t  battery_pct;
};

inline uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
inline uint32_t rd32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
inline void wr16(uint8_t* p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
inline void wr32(uint8_t* p, uint32_t v) {
  p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

// Üreticiye özel veri alanını (company ID dahil) çözer.
inline bool decode_mfg(const uint8_t* d, size_t len, TagPacket& out) {
  if (d == nullptr || len != MFG_DATA_LEN) return false;
  if (rd16(d) != COMPANY_ID) return false;
  out.tag_id      = d[2];
  out.seq         = rd16(d + 3);
  out.temp_centi  = (int16_t)rd16(d + 5);
  out.tx_ms       = rd32(d + 7);
  out.battery_pct = d[11];
  return true;
}

// Tüm reklam yükünü (AD yapıları dizisi) tarar, 0xFF tipindeki alanı bulup çözer.
// AD yapısı: [uzunluk][tip][veri...], uzunluk = tip + veri bayt sayısı.
inline bool decode_adv_payload(const uint8_t* p, size_t len, TagPacket& out) {
  size_t i = 0;
  while (i < len) {
    uint8_t field_len = p[i];
    if (field_len == 0) break;                    // dolgu
    if (i + 1 + field_len > len) return false;    // bozuk yapı
    uint8_t type = p[i + 1];
    if (type == 0xFF && decode_mfg(p + i + 2, field_len - 1, out)) return true;
    i += 1 + field_len;
  }
  return false;
}

// Verici tarafı için: Flags + üreticiye özel veri içeren ham reklam yükünü üretir.
// Dönen değer yazılan bayt sayısıdır (buf en az 3 + 2 + 12 = 17 bayt olmalı).
inline size_t encode_adv_payload(const TagPacket& t, uint8_t* buf) {
  size_t i = 0;
  buf[i++] = 2; buf[i++] = 0x01; buf[i++] = 0x06;          // Flags: LE General Disc., BR/EDR yok
  buf[i++] = 1 + MFG_DATA_LEN; buf[i++] = 0xFF;             // üreticiye özel veri
  wr16(buf + i, COMPANY_ID); i += 2;
  buf[i++] = t.tag_id;
  wr16(buf + i, t.seq); i += 2;
  wr16(buf + i, (uint16_t)t.temp_centi); i += 2;
  wr32(buf + i, t.tx_ms); i += 4;
  buf[i++] = t.battery_pct;
  return i;
}

// Sıra numarası farkı (taşmaya dayanıklı): 65535 -> 0 geçişi 1 adım sayılır.
inline uint16_t seq_delta(uint16_t prev, uint16_t now) { return (uint16_t)(now - prev); }

}  // namespace contract
