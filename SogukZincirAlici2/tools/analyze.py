"""Ağ geçidi ve simülatör kayıtlarından üç başarı ölçütünü hesaplar.

Kullanım:
    python tools/analyze.py data/gateway.log data/sim.log

Girdiler:
    gateway.log : RX, ALARM, SYNC satırları (firmware/gateway)
    sim.log     : TX, SYNC satırları (firmware/tag_simulator) — gerçeğin kaydı

Çıktılar:
    (a) Kayıp paket oranı (%)
    (b) Uçtan uca gecikme: medyan ve 95. yüzdelik (ms), saat ofseti ve kayması düzeltilmiş
    (c) Eşik alarmı: N_yanlış ve N_kaçırılan

Yalnızca Python standart kütüphanesi kullanılır.
"""
from __future__ import annotations

import statistics
import sys
from bisect import bisect_right
from dataclasses import dataclass, field


@dataclass
class GatewayLog:
    rx: list = field(default_factory=list)      # (gw_ms, seq, tx_ms, temp, rssi)
    alarms: list = field(default_factory=list)  # (gw_ms, kind, edge)
    sync_ms: list = field(default_factory=list)


@dataclass
class SimLog:
    tx: list = field(default_factory=list)      # (seq, tx_ms, temp, segment)
    sync_ms: list = field(default_factory=list)


def read_gateway(path: str) -> GatewayLog:
    g = GatewayLog()
    with open(path, encoding="utf-8") as f:
        for line in f:
            p = line.strip().split(",")
            try:
                if p[0] == "RX" and len(p) == 10:
                    g.rx.append((int(p[1]) / 1000.0, int(p[6]), int(p[8]), int(p[7]), int(p[4])))
                elif p[0] == "ALARM" and len(p) == 6:
                    g.alarms.append((int(p[1]) / 1000.0, p[3], p[4]))
                elif p[0] == "SYNC" and len(p) == 2:
                    g.sync_ms.append(int(p[1]) / 1000.0)
            except ValueError:
                continue  # yarım kalmış satır
    return g


def read_sim(path: str) -> SimLog:
    s = SimLog()
    with open(path, encoding="utf-8") as f:
        for line in f:
            p = line.strip().split(",")
            try:
                if p[0] == "TX" and len(p) == 5:
                    s.tx.append((int(p[1]), int(p[2]), int(p[3]), p[4]))
                elif p[0] == "SYNC" and len(p) == 2:
                    s.sync_ms.append(float(p[1]))
            except ValueError:
                continue
    return s


# ---------------- Saat ofseti (θ) ----------------

def rough_offset(g: GatewayLog) -> float:
    """Kaba ofset: alış anı − pakette yazan gönderim anı. Gecikme kadar (birkaç ms) hatalıdır,
    ama SYNC darbelerinden hangisinin hangisiyle eşleştiğini belirlemeye yeter."""
    return statistics.median(gw_ms - tx_ms for gw_ms, _, tx_ms, *_ in g.rx)


def fit_offset(gw_sync: list[float], sim_sync: list[float], theta_rough: float):
    """SYNC darbelerini eşleştirir; θ(t) = a + b·t doğrusal modelini (kayma dahil) bulur.

    Darbeler periyodik olduğu için her kaydırma aynı saçılmayı verir (θ periyot kadar
    kayar). Bu yüzden kartların açılış farkını telafi eden kaydırma, θ'yı kaba ofsete
    en yakın yapan kaydırma olarak seçilir.
    """
    best = None
    max_shift = max(len(gw_sync), len(sim_sync))
    for shift in range(-max_shift, max_shift + 1):
        pairs = [(g, sim_sync[i + shift]) for i, g in enumerate(gw_sync)
                 if 0 <= i + shift < len(sim_sync)]
        if len(pairs) < 3:
            continue
        dist = abs(statistics.median(g - s for g, s in pairs) - theta_rough)
        if best is None or dist < best[0]:
            best = (dist, pairs)
    if best is None:
        raise SystemExit("Yeterli SYNC eşleşmesi yok (en az 3 gerekli). Kabloyu ve GND'yi kontrol et.")
    pairs = best[1]
    xs = [g for g, _ in pairs]
    ys = [g - s for g, s in pairs]
    slope, intercept = statistics.linear_regression(xs, ys)
    residual = statistics.pstdev([y - (intercept + slope * x) for x, y in zip(xs, ys)])
    return intercept, slope, len(pairs), residual


# ---------------- Ölçütler ----------------

def packet_loss(g: GatewayLog, s: SimLog) -> dict:
    received = {seq for _, seq, *_ in g.rx}
    sent = [seq for seq, *_ in s.tx]
    first_seen = min(received) if received else None
    # Ağ geçidi açılmadan önce gönderilen paketleri sayma
    sent = [q for q in sent if first_seen is not None and q >= first_seen]
    n_exp = len(sent)
    n_rx = len(set(sent) & received)
    return {
        "N_beklenen": n_exp,
        "N_alinan": n_rx,
        "kayip_yuzde": 100.0 * (n_exp - n_rx) / n_exp if n_exp else float("nan"),
        "kopya_basina_ortalama": len(g.rx) / n_rx if n_rx else float("nan"),
    }


def percentile(values: list[float], q: float) -> float:
    v = sorted(values)
    idx = (len(v) - 1) * q
    lo, hi = int(idx), min(int(idx) + 1, len(v) - 1)
    return v[lo] + (v[hi] - v[lo]) * (idx - lo)


def latency(g: GatewayLog, a: float, b: float) -> dict:
    first = {}
    for gw_ms, seq, tx_ms, *_ in g.rx:          # her sıra numarasının İLK kopyası
        if seq not in first:
            first[seq] = (gw_ms, tx_ms)
    d = [gw - tx - (a + b * gw) for gw, tx in first.values()]
    return {
        "n": len(d),
        "medyan_ms": statistics.median(d),
        "p95_ms": percentile(d, 0.95),
        "min_ms": min(d),
        "max_ms": max(d),
    }


def alarm_accuracy(g: GatewayLog, s: SimLog, a: float, b: float) -> dict:
    """Her ALARM olayını, o andaki senaryo bölümüyle karşılaştırır.

    Doğru OPEN  : G bölümünde, o bölüm için ilk OPEN
    Doğru CLOSE : G bölümünden sonra açık alarmın N (normal) bölümünde kapanması
    Diğer her olay yanlıştır (ör. tuzakta açılma, salınımda erken kapanma).
    Kaçırılan   : OPEN almamış G bölümü
    """
    tx_times = [tx_ms for _, tx_ms, _, _ in s.tx]
    labels = [seg for *_, seg in s.tx]

    # G bölümlerini ardışık koşular hâlinde numaralandır (senaryo tekrar ederse G1 iki kez geçer)
    run_id, runs, prev = [], [], None
    for seg in labels:
        if seg.startswith("G") and seg != prev:
            runs.append(seg)
        run_id.append(len(runs) - 1 if seg.startswith("G") else None)
        prev = seg

    def segment_at(gw_ms: float):
        sim_ms = gw_ms - (a + b * gw_ms)
        i = bisect_right(tx_times, sim_ms) - 1
        if i < 0:
            return None, None
        return labels[i], run_id[i]

    opened_runs, wrong, details = set(), 0, []
    open_state = {"HIGH": False, "LOW": False}
    for gw_ms, kind, edge in g.alarms:
        seg, rid = segment_at(gw_ms)
        ok = False
        if edge == "OPEN":
            ok = seg is not None and seg.startswith("G") and rid not in opened_runs
            if ok:
                opened_runs.add(rid)
            open_state[kind] = True
        else:
            ok = open_state[kind] and seg == "N"
            open_state[kind] = False
        if not ok:
            wrong += 1
        details.append(f"{kind:4s} {edge:5s} bölüm={seg} -> {'doğru' if ok else 'YANLIŞ'}")

    in_window = [r for r in range(len(runs))]
    missed = sum(1 for r in in_window if r not in opened_runs)
    return {"N_gercek_asim": len(runs), "N_yanlis": wrong, "N_kacirilan": missed, "olaylar": details}


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit("Kullanım: python tools/analyze.py <gateway.log> <sim.log>")
    g, s = read_gateway(sys.argv[1]), read_sim(sys.argv[2])
    if not g.rx or not s.tx:
        sys.exit("Kayıtlarda RX veya TX satırı yok.")

    a, b, n_sync, resid = fit_offset(g.sync_ms, s.sync_ms, rough_offset(g))
    print("== Saat ofseti ==")
    print(f"  SYNC eşleşmesi: {n_sync}, θ başlangıç ≈ {a + b * g.sync_ms[0]:.1f} ms, "
          f"kayma ≈ {b * 1e6:.1f} ppm, artık saçılma ≈ {resid:.2f} ms")

    pl = packet_loss(g, s)
    print("== (a) Kayıp paket oranı ==")
    print(f"  N_beklenen={pl['N_beklenen']}  N_alınan={pl['N_alinan']}  "
          f"kayıp=%{pl['kayip_yuzde']:.2f}  (paket başına ortalama kopya {pl['kopya_basina_ortalama']:.2f})")

    lt = latency(g, a, b)
    print("== (b) Uçtan uca gecikme (ofset düzeltilmiş) ==")
    print(f"  n={lt['n']}  medyan={lt['medyan_ms']:.1f} ms  p95={lt['p95_ms']:.1f} ms  "
          f"[min {lt['min_ms']:.1f}, max {lt['max_ms']:.1f}]")

    al = alarm_accuracy(g, s, a, b)
    print("== (c) Eşik alarmı ==")
    print(f"  gerçek aşım={al['N_gercek_asim']}  N_yanlış={al['N_yanlis']}  N_kaçırılan={al['N_kacirilan']}")
    for line in al["olaylar"]:
        print("   ", line)


if __name__ == "__main__":
    main()
