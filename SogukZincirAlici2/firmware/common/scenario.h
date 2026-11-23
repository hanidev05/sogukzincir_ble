// scenario.h — Ölçüt (c) için önceden bilinen sıcaklık senaryosu (etiket simülatörü kullanır)
//
// Varsayımlar: 1 paket/saniye, TIME_SCALE = 60
//   -> yüksek alarm süre koşulu 10 saat / 60 = 600 s, düşük alarm 60 dk / 60 = 60 s
//   -> histerezis 0,50 °C
//
// Etiketler:  N = normal,  G = gerçek aşım (alarm BEKLENİR),  T = tuzak (alarm BEKLENMEZ)
//
//   Bölüm  Sıcaklık                         Süre    Beklenen
//   N      5,00 °C                          120 s   —
//   T1     8,20 °C (eşik üstü, kısa)        300 s   alarm yok (600 s'den kısa)
//   N      5,00 °C                           60 s   —
//   G1     9,00 °C                          660 s   yüksek alarm açılır
//   N      5,00 °C                           60 s   G1 alarmı kapanır
//   T2     7,90 / 8,10 °C, 2 s'de bir       900 s   alarm yok (kesintisiz değil)
//   G2     10,00 °C                         620 s   yüksek alarm açılır
//   T3     7,70 / 8,10 °C, 2 s'de bir       120 s   alarm AÇIK KALIR (histerezis; 7,50'nin altına inmiyor)
//   N      5,00 °C                           60 s   G2 alarmı kapanır
//   T4     −0,60 °C (kısa)                   45 s   alarm yok (60 s'den kısa)
//   N      4,00 °C                           60 s   —
//   G3     −1,00 °C                          90 s   düşük alarm açılır
//   N      4,00 °C                           60 s   G3 alarmı kapanır
//
// Toplam 3155 s (~53 dk), 3155 paket. 3 gerçek aşım, 4 tuzak.
// Beklenen olaylar: 3 OPEN + 3 CLOSE. Başka her olay yanlış alarm sayılır.

#pragma once
#include <stdint.h>

namespace scenario {

struct Segment {
  const char* label;     // "N", "G1", "T1", ...
  int16_t     temp_a;    // 0,01 °C
  int16_t     temp_b;    // salınım için ikinci değer; salınım yoksa temp_a ile aynı
  uint16_t    alt_s;     // salınım yarı periyodu (s); 0 = salınım yok
  uint16_t    dur_s;     // bölüm süresi (s)
};

constexpr Segment SEGMENTS[] = {
  {"N",   500,  500, 0, 120},
  {"T1",  820,  820, 0, 300},
  {"N",   500,  500, 0,  60},
  {"G1",  900,  900, 0, 660},
  {"N",   500,  500, 0,  60},
  {"T2",  790,  810, 2, 900},
  {"G2", 1000, 1000, 0, 620},
  {"T3",  770,  810, 2, 120},
  {"N",   500,  500, 0,  60},
  {"T4",  -60,  -60, 0,  45},
  {"N",   400,  400, 0,  60},
  {"G3", -100, -100, 0,  90},
  {"N",   400,  400, 0,  60},
};
constexpr int N_SEGMENTS = sizeof(SEGMENTS) / sizeof(SEGMENTS[0]);

inline uint32_t total_seconds() {
  uint32_t s = 0;
  for (int i = 0; i < N_SEGMENTS; ++i) s += SEGMENTS[i].dur_s;
  return s;
}

// Senaryonun k. saniyesindeki sıcaklık ve bölüm etiketi. Senaryo bitince başa sarar.
inline int16_t temp_at(uint32_t k, const char** label_out) {
  k %= total_seconds();
  for (int i = 0; i < N_SEGMENTS; ++i) {
    const Segment& s = SEGMENTS[i];
    if (k < s.dur_s) {
      if (label_out) *label_out = s.label;
      if (s.alt_s == 0) return s.temp_a;
      return ((k / s.alt_s) % 2 == 0) ? s.temp_a : s.temp_b;
    }
    k -= s.dur_s;
  }
  if (label_out) *label_out = "N";
  return 500;
}

}  // namespace scenario
