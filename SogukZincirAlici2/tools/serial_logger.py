"""Seri porttan gelen satırları zaman damgası eklemeden olduğu gibi dosyaya yazar.

Kullanım (iki ayrı terminalde, iki kart için):
    python tools/serial_logger.py COM5 data/2026-xx-xx_gateway.log
    python tools/serial_logger.py COM6 data/2026-xx-xx_sim.log

Gerekli paket: pip install pyserial
Durdurmak için Ctrl+C.
"""
import sys

try:
    import serial
except ImportError:
    sys.exit("pyserial yüklü değil: pip install pyserial")


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit("Kullanım: python serial_logger.py <port> <çıktı_dosyası>")
    port, out_path = sys.argv[1], sys.argv[2]
    n = 0
    with serial.Serial(port, 115200, timeout=1) as ser, open(out_path, "a", encoding="utf-8") as f:
        print(f"{port} dinleniyor -> {out_path} (Ctrl+C ile dur)")
        try:
            while True:
                line = ser.readline().decode("utf-8", errors="replace").strip()
                if not line:
                    continue
                f.write(line + "\n")
                n += 1
                if n % 100 == 0:
                    f.flush()
                    print(f"{n} satır", end="\r")
        except KeyboardInterrupt:
            print(f"\n{n} satır kaydedildi.")


if __name__ == "__main__":
    main()
