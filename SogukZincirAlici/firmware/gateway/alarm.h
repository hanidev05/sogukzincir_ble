// alarm.h — Süre koşullu ve histerezisli eşik alarmı (Arduino bağımlılığı yok)
//
// Eşik ve süreler WHO PQS E006/TR06.4, Type 1 (aşı buzdolabı) değerlerine dayanır:
//   Yüksek alarm: +8 °C veya üstü, kesintisiz 10 saatten uzun
//   Düşük alarm : −0,5 °C veya altı, kesintisiz 60 dakikadan uzun
// WHO şartnamesi alarmın KAPANMA koşulunu tanımlamaz; bu çalışmada histerezis kullanılır:
//   Yüksek alarm, sıcaklık (8 − hyst) °C'nin altına inince kapanır.
//   Düşük alarm,  sıcaklık (−0,5 + hyst) °C'nin üstüne çıkınca kapanır.
//
// Testte 10 saat / 60 dk beklemek pratik olmadığı için süreler TIME_SCALE ile kısaltılabilir;
// alarm mantığı aynı kalır.

#pragma once
#include <stdint.h>

namespace alarm {

struct Config {
  int16_t  high_centi;     // 800  = +8,00 °C
  int16_t  low_centi;      // -50  = −0,50 °C
  int16_t  hyst_centi;     // histerezis payı (0,01 °C)
  uint32_t high_dur_ms;    // yüksek alarm süre koşulu
  uint32_t low_dur_ms;     // düşük alarm süre koşulu
};

// WHO değerleri; scale = 1 gerçek kullanım, scale = 60 ise 10 saat -> 10 dk, 60 dk -> 1 dk.
inline Config who_type1(int16_t hyst_centi, uint32_t scale) {
  Config c;
  c.high_centi  = 800;
  c.low_centi   = -50;
  c.hyst_centi  = hyst_centi;
  c.high_dur_ms = 10UL * 3600UL * 1000UL / scale;
  c.low_dur_ms  = 60UL * 60UL * 1000UL / scale;
  return c;
}

enum class Kind : uint8_t { HIGH_ALARM, LOW_ALARM };
enum class Edge : uint8_t { OPEN, CLOSE };

struct Event {
  bool     valid = false;
  Kind     kind;
  Edge     edge;
  uint32_t t_ms;           // olayın ağ geçidi saatine göre anı
  uint32_t since_ms;       // OPEN için: eşiğin ilk aşıldığı an
};

class Detector {
 public:
  explicit Detector(const Config& c) : cfg_(c) {}

  // Her alınan pakette çağrılır. t_ms: ağ geçidi saati. En fazla bir olay döner.
  Event update(int16_t temp_centi, uint32_t t_ms) {
    Event e;
    // --- Yüksek alarm ---
    if (!high_on_) {
      if (temp_centi >= cfg_.high_centi) {
        if (!high_pending_) { high_pending_ = true; high_since_ = t_ms; }
        else if (t_ms - high_since_ >= cfg_.high_dur_ms) {
          high_on_ = true; high_pending_ = false;
          return make(Kind::HIGH_ALARM, Edge::OPEN, t_ms, high_since_);
        }
      } else {
        high_pending_ = false;               // kesintisiz değil -> sayaç sıfırlanır
      }
    } else if (temp_centi < cfg_.high_centi - cfg_.hyst_centi) {
      high_on_ = false;
      return make(Kind::HIGH_ALARM, Edge::CLOSE, t_ms, 0);
    }
    // --- Düşük alarm ---
    if (!low_on_) {
      if (temp_centi <= cfg_.low_centi) {
        if (!low_pending_) { low_pending_ = true; low_since_ = t_ms; }
        else if (t_ms - low_since_ >= cfg_.low_dur_ms) {
          low_on_ = true; low_pending_ = false;
          return make(Kind::LOW_ALARM, Edge::OPEN, t_ms, low_since_);
        }
      } else {
        low_pending_ = false;
      }
    } else if (temp_centi > cfg_.low_centi + cfg_.hyst_centi) {
      low_on_ = false;
      return make(Kind::LOW_ALARM, Edge::CLOSE, t_ms, 0);
    }
    return e;
  }

  bool any_active() const { return high_on_ || low_on_; }

 private:
  static Event make(Kind k, Edge ed, uint32_t t, uint32_t since) {
    Event e; e.valid = true; e.kind = k; e.edge = ed; e.t_ms = t; e.since_ms = since; return e;
  }
  Config   cfg_;
  bool     high_on_ = false, high_pending_ = false;
  bool     low_on_  = false, low_pending_  = false;
  uint32_t high_since_ = 0, low_since_ = 0;
};

}  // namespace alarm
