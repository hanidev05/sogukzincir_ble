// Bilgisayarda çalışan birim testleri (donanım gerekmez):
//   g++ -std=c++17 -I firmware/common tests/test_core.cpp -o test_core && ./test_core
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "contract.h"
#include "alarm.h"
#include "scenario.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("HATA: %s (satır %d)\n", #cond, __LINE__); ++failures; } } while (0)

static void test_roundtrip() {
  contract::TagPacket in{7, 65535, -123, 4294967000u, 87}, out{};
  uint8_t buf[32];
  size_t n = contract::encode_adv_payload(in, buf);
  CHECK(n == 17);
  CHECK(contract::decode_adv_payload(buf, n, out));
  CHECK(out.tag_id == 7 && out.seq == 65535 && out.temp_centi == -123);
  CHECK(out.tx_ms == 4294967000u && out.battery_pct == 87);
}

static void test_known_bytes() {
  // 4,25 °C = 425 = 0x01A9 -> little-endian A9 01 (README'deki örnek)
  contract::TagPacket t{1, 2, 425, 0x11223344, 100};
  uint8_t buf[32];
  contract::encode_adv_payload(t, buf);
  const uint8_t expected_mfg[12] = {0xFF, 0xFF, 0x01, 0x02, 0x00, 0xA9, 0x01,
                                    0x44, 0x33, 0x22, 0x11, 0x64};
  CHECK(std::memcmp(buf + 5, expected_mfg, 12) == 0);
}

static void test_rejects_foreign() {
  contract::TagPacket out{};
  // Başka üreticinin (ör. Apple 0x004C) verisi reddedilmeli
  uint8_t foreign[] = {2, 0x01, 0x06, 13, 0xFF, 0x4C, 0x00, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  CHECK(!contract::decode_adv_payload(foreign, sizeof(foreign), out));
  // Kesik paket reddedilmeli
  uint8_t truncated[] = {2, 0x01, 0x06, 13, 0xFF, 0xFF, 0xFF, 1, 2};
  CHECK(!contract::decode_adv_payload(truncated, sizeof(truncated), out));
}

static void test_invalid_markers() {
  // Sensör hatası ve bilinmeyen pil değerleri kodlamada korunmalı
  contract::TagPacket in{3, 9, contract::TEMP_INVALID, 1000, contract::BATT_UNKNOWN}, out{};
  uint8_t buf[32];
  size_t n = contract::encode_adv_payload(in, buf);
  CHECK(contract::decode_adv_payload(buf, n, out));
  CHECK(out.temp_centi == -32768 && out.battery_pct == 255);
  CHECK(buf[5 + 5] == 0x00 && buf[5 + 6] == 0x80);   // 0x8000 little-endian: 00 80
}

static void test_seq_wrap() {
  CHECK(contract::seq_delta(65535, 0) == 1);
  CHECK(contract::seq_delta(10, 13) == 3);
}

// Senaryoyu saniyede bir paketle alarm dedektöründen geçirir; beklenen olayları kontrol eder.
static void test_scenario_alarms() {
  alarm::Detector det(alarm::who_type1(/*hyst*/ 50, /*scale*/ 60));
  struct Got { std::string seg; alarm::Kind kind; alarm::Edge edge; };
  std::vector<Got> got;
  uint32_t total = scenario::total_seconds();
  CHECK(total == 3155);
  for (uint32_t k = 0; k < total; ++k) {
    const char* label = nullptr;
    int16_t t = scenario::temp_at(k, &label);
    alarm::Event e = det.update(t, k * 1000);
    if (e.valid) got.push_back({label, e.kind, e.edge});
  }
  // Beklenen: G1 içinde OPEN, sonraki N'de CLOSE; G2 OPEN, T3 sonrası N'de CLOSE; G3 OPEN, N'de CLOSE.
  CHECK(got.size() == 6);
  if (got.size() == 6) {
    CHECK(got[0].seg == "G1" && got[0].kind == alarm::Kind::HIGH_ALARM && got[0].edge == alarm::Edge::OPEN);
    CHECK(got[1].seg == "N"  && got[1].edge == alarm::Edge::CLOSE);
    CHECK(got[2].seg == "G2" && got[2].kind == alarm::Kind::HIGH_ALARM && got[2].edge == alarm::Edge::OPEN);
    CHECK(got[3].seg == "N"  && got[3].edge == alarm::Edge::CLOSE);
    CHECK(got[4].seg == "G3" && got[4].kind == alarm::Kind::LOW_ALARM && got[4].edge == alarm::Edge::OPEN);
    CHECK(got[5].seg == "N"  && got[5].edge == alarm::Edge::CLOSE);
  }
  for (auto& g : got) std::printf("  olay: bölüm=%-3s %s %s\n", g.seg.c_str(),
      g.kind == alarm::Kind::HIGH_ALARM ? "YUKSEK" : "DUSUK",
      g.edge == alarm::Edge::OPEN ? "OPEN" : "CLOSE");
}

// Histerezis olmasaydı: süre koşulu yeniden açılmayı engeller, ama G2 alarmı T3 salınımında
// (sıcaklık hâlâ aralıklı olarak 8,10 °C iken) ERKEN kapanır. Bu bir yanlış olaydır.
static void test_without_hysteresis_closes_early() {
  alarm::Detector det(alarm::who_type1(/*hyst*/ 0, /*scale*/ 60));
  std::string g2_close_seg;
  bool after_g2_open = false;
  for (uint32_t k = 0; k < scenario::total_seconds(); ++k) {
    const char* label = nullptr;
    alarm::Event e = det.update(scenario::temp_at(k, &label), k * 1000);
    if (!e.valid) continue;
    if (e.edge == alarm::Edge::OPEN && std::string(label) == "G2") after_g2_open = true;
    else if (after_g2_open && e.edge == alarm::Edge::CLOSE) { g2_close_seg = label; after_g2_open = false; }
  }
  std::printf("  histerezis=0 iken G2 alarmının kapandığı bölüm: %s (histerezisli: N)\n", g2_close_seg.c_str());
  CHECK(g2_close_seg == "T3");
}

int main() {
  test_roundtrip();
  test_known_bytes();
  test_rejects_foreign();
  test_seq_wrap();
  test_invalid_markers();
  test_scenario_alarms();
  test_without_hysteresis_closes_early();
  std::printf(failures == 0 ? "TUM TESTLER GECTI\n" : "%d TEST BASARISIZ\n", failures);
  return failures == 0 ? 0 : 1;
}
