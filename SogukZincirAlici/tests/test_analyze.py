"""analyze.py'yi bilinen değerlerle üretilmiş sahte kayıtlarla doğrular (donanım gerekmez).

    python tests/test_analyze.py
"""
import os
import random
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))
import analyze  # noqa: E402

# firmware/common/scenario.h ile aynı senaryo
SEGMENTS = [("N", 500, 500, 0, 120), ("T1", 820, 820, 0, 300), ("N", 500, 500, 0, 60),
            ("G1", 900, 900, 0, 660), ("N", 500, 500, 0, 60), ("T2", 790, 810, 2, 900),
            ("G2", 1000, 1000, 0, 620), ("T3", 770, 810, 2, 120), ("N", 500, 500, 0, 60),
            ("T4", -60, -60, 0, 45), ("N", 400, 400, 0, 60), ("G3", -100, -100, 0, 90),
            ("N", 400, 400, 0, 60)]


def seg_at(k):
    for label, a, b, alt, dur in SEGMENTS:
        if k < dur:
            return label, (a if alt == 0 or (k // alt) % 2 == 0 else b)
        k -= dur
    raise IndexError


def make_logs(tmp, loss=0.03, offset_ms=123456.0, drift_ppm=20.0, extra_alarms=(), drop_alarms=()):
    rnd = random.Random(42)
    total = sum(s[4] for s in SEGMENTS)
    sim_start = 5000  # simülatör açılıştan 5 s sonra yayına başlar

    def gw_of(sim_ms):  # ağ geçidi saati = simülatör saati * (1+kayma) + ofset
        return sim_ms * (1 + drift_ppm * 1e-6) + offset_ms

    sim_lines, gw_lines, true_lat = [], [], []
    for k in range(total):
        label, temp = seg_at(k)
        tx = sim_start + k * 1000
        sim_lines.append(f"TX,{k},{tx},{temp},{label}")
        if rnd.random() >= loss:
            lat = rnd.uniform(5, 35)
            true_lat.append(lat)
            gw_lines.append(f"RX,{int(gw_of(tx + lat) * 1000)},0,aa:bb,-60,1,{k},{temp},{tx},100")
    for j in range(total // 10):
        t = sim_start + j * 10000 + 3
        sim_lines.append(f"SYNC,{t}")
        gw_lines.append(f"SYNC,{int(gw_of(t) * 1000)}")

    # Doğru alarm olayları (simülatör saatine göre, saniye cinsinden senaryo anı)
    events = [(1080, "HIGH", "OPEN"), (1140, "HIGH", "CLOSE"),
              (2700, "HIGH", "OPEN"), (2845, "HIGH", "CLOSE"),
              (3065, "LOW", "OPEN"), (3100, "LOW", "CLOSE")]
    events = [e for e in events if e[0] not in drop_alarms] + list(extra_alarms)
    for k, kind, edge in sorted(events):
        gw_lines.append(f"ALARM,{int(gw_of(sim_start + k * 1000 + 20) * 1000)},1,{kind},{edge},0")

    gp, sp = os.path.join(tmp, "gw.log"), os.path.join(tmp, "sim.log")
    open(gp, "w").write("\n".join(gw_lines) + "\n")
    open(sp, "w").write("\n".join(sim_lines) + "\n")
    return gp, sp, true_lat


def run(gp, sp):
    g, s = analyze.read_gateway(gp), analyze.read_sim(sp)
    a, b, _, _ = analyze.fit_offset(g.sync_ms, s.sync_ms, analyze.rough_offset(g))
    return a, b, analyze.packet_loss(g, s), analyze.latency(g, a, b), analyze.alarm_accuracy(g, s, a, b)


def main():
    fails = 0
    with tempfile.TemporaryDirectory() as tmp:
        gp, sp, true_lat = make_logs(tmp)
        a, b, pl, lt, al = run(gp, sp)
        import statistics
        exp_med = statistics.median(true_lat)
        print(f"kayma tahmini {b*1e6:.2f} ppm (gerçek 20), kayıp %{pl['kayip_yuzde']:.2f} (hedef ~3), "
              f"medyan {lt['medyan_ms']:.2f} ms (gerçek {exp_med:.2f}), "
              f"yanlış {al['N_yanlis']}, kaçırılan {al['N_kacirilan']}")
        fails += abs(b * 1e6 - 20) > 1
        fails += not (2.0 < pl["kayip_yuzde"] < 4.0)
        fails += abs(lt["medyan_ms"] - exp_med) > 1.0
        fails += (al["N_yanlis"], al["N_kacirilan"]) != (0, 0)

        # Tuzakta (T1) yanlış alarm + G3 kaçırılırsa
        gp, sp, _ = make_logs(tmp, extra_alarms=[(250, "HIGH", "OPEN"), (430, "HIGH", "CLOSE")],
                              drop_alarms=(3065, 3100))
        _, _, _, _, al = run(gp, sp)
        print(f"hatalı senaryo: yanlış {al['N_yanlis']} (beklenen 1), kaçırılan {al['N_kacirilan']} (beklenen 1)")
        fails += (al["N_yanlis"], al["N_kacirilan"]) != (1, 1)

    print("TUM TESTLER GECTI" if fails == 0 else f"{fails} KONTROL BASARISIZ")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
