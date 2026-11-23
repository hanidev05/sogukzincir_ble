# Literatür Taraması

## Arama süreci

- **Araç:** Science AI Journal — Research Gap Finder
- **1. sorgu:** `ESP32 BLE gateway cold chain temperature logging packet loss latency` → Boşluk listesi oluşmadı (dar konuda yeterli çalışma bulunamadı).
- **2. sorgu (danışman önerisi):** `BLE advertising packet loss latency wireless sensor gateway` → 16 çalışmalık karşılaştırma tablosu (14 tam metin, 2 yalnız özet).
- **Gözlem:** 2. sorgunun getirdiği çalışmaların büyük bölümü benzetim tabanlı genel kablosuz sensör ağı (yönlendirme, tıkanıklık kontrolü, LPWAN, 5G/6G) çalışmalarıdır. Dar kapsamlı aramada (soğuk zincir + ESP32 + BLE ağ geçidi) ilgili çalışmaya rastlanmamıştır.

## Projeye en yakın çalışmalar

| Çalışma | Benzerlik | Fark |
|---|---|---|
| [2] Beyond beaconing (2019) | BLE bağlantısız (reklam) modu; RSSI ölçütü; veri yükü sınırı | Derleme; ağ geçidinde kayıp/gecikme ölçümü yok |
| [1] Evolution of Bluetooth Technology (2025) | Reklam/tarama mekanizmaları ve gecikme | Derleme; deney verisi yok |
| [3] Multihop Cluster-Based Agricultural IoT (2024) | Gerçek donanımla BLE tabanlı sensör ağı | Tarım uygulaması; enerji ve ölçeklenebilirlik odaklı |

## Ölçütlerin literatürdeki karşılığı

| Bu projedeki ölçüt | Tabloda kullanan çalışmalar | Not |
|---|---|---|
| (a) Kayıp paket oranı | [4], [5]; paket teslim oranı (PDR) olarak [6], [7] | Hepsi benzetim |
| (b) Uçtan uca gecikme | [4], [6], [7] | Saat ofseti düzeltmesi yok |
| (c) Yanlış alarm sayısı | — | Tabloda kullanan çalışma yok |
| RSSI (ek çıktı) | [2] | Girişim değerlendirmesi için |

## Boşluk

İncelenen çalışmalarda BLE reklam paketlerinin ağ geçidi tarafındaki kaybı, saat ofseti düzeltilmiş uçtan uca gecikme ve histerezis + süre koşullu eşik alarmının doğruluğu birlikte ve gerçek donanım üzerinde ele alınmamıştır. Bu ifade yalnızca incelenen çalışma seti için geçerlidir.

## Kaynaklar

Yazar, cilt, sayı ve DOI bilgileri Google Scholar üzerinden doğrulanarak doldurulacaktır.

1. ___, "Evolution of Bluetooth Technology: BLE in the IoT Ecosystem," *Sensors*, 2025, cilt ___, sayı ___, DOI: ___.
2. ___, "Beyond beaconing: Emerging applications and challenges of BLE," *Ad Hoc Networks*, 2019, cilt ___, DOI: ___.
3. ___, "Hardware Development and Evaluation of Multihop Cluster-Based Agricultural IoT Based on Bluetooth Low-Energy and LoRa Communication Technologies," *Sensors*, 2024, cilt ___, sayı ___, DOI: ___.
4. ___, "New approach of multi-path reliable transmission for marginal wireless sensor network," *Wireless Networks*, 2019, cilt ___, DOI: ___.
5. ___, "Congestion control in wireless sensor and 6LoWPAN networks: toward the Internet of Things," *Wireless Networks*, 2018, cilt ___, DOI: ___.
6. ___, "Energy Efficient Multimedia Transmission in Wireless Sensor Networks using Enhanced Adaptive Transmission Control Algorithm and Measurement Techniques," *Measurement Science Review*, 2024, cilt ___, DOI: ___.
7. ___, "A Game Theoretic Optimization of RPL for Mobile Internet of Things Applications," *IEEE Sensors Journal*, 2018, cilt ___, DOI: ___.
8. ___, "Energy Footprint and Reliability of IoT Communication Protocols for Remote Sensor Networks," *Sensors*, 2025, cilt ___, sayı ___, DOI: ___.

## Hedef değerlerin dayanağı (araç dışı kaynaklar)

Science AI Journal taraması hedef değer verecek kadar yakın çalışma getirmediği için başarı ölçütlerinin hedefleri aşağıdaki standart ve kaynaklara dayandırılmıştır. Akademik dayanak: [A], [C], [E]; bağlam ve risk olarak kullanılanlar: [B], [D], [F].

| Kaynak | Kullanıldığı yer | Alınan bilgi |
|---|---|---|
| [A] WHO PQS E006/TR06.4 | Ölçüt (c), arayüz | Type 1 alarm: +8 °C veya üstü 10 saat; −0,5 °C veya altı 60 dk; ölçüm aralığı en fazla 10 dk; çözünürlük ±0,2 °C. Alarm kapanma koşulu tanımlanmamış (histerezis bu projenin katkısı). |
| [B] Ruuvi blog | Ölçüt (a) | İyi koşullarda bile ağ geçitlerinde %5–50 paket kaybı gözlemi (hedef: ≤ %5). |
| [C] IEEE TMC 2024 (BLE legacy advertising packet loss) | Ölçüt (a) | Tekrarlı yayınla kaybın ~%0'a indirilebildiği; kaybın tarama penceresine bağlı olduğu. |
| [D] LogTag VaxTag broşürü | Ölçüt (b) | WHO ön yeterlilikli kaydedicinin saati ±25 ppm (≈2,5 s/gün): saat kayması düzeltmesinin gerekçesi. |
| [E] Zachariah ve ark. (Berkeley) | Ölçüt (a) | Alım kaybının ağdan değil, ağ geçidinin donanım ve yazılımından kaynaklanabildiği. |
| [F] Espressif ESP32 forumu | Kod ayarları | Yinelenen paket filtresinin kapatılması, tarama penceresi = tarama aralığı önerisi. |

[A] WHO, "PQS performance specification E006/TR06.4: 30-day electronic refrigerator temperature logger," 2022. https://extranet.who.int/prequal/sites/default/files/document_files/PQS_E006_TR06.4.pdf
[B] Ruuvi, "How many Bluetooth beacons I can have in one place?" https://ruuvi.com/?p=2908
[C] ___ (yazarlar), "Dynamic Data Advertising and Packet Loss Analysis Using BLE Legacy Advertising," *IEEE Transactions on Mobile Computing*, 2024.
[D] LogTag, "Vaxtag 30 Day Display Refrigerator Temperature Recorder" (ürün broşürü). https://logtagrecorders.com/app/uploads/VaxtagBrochure.pdf
[E] Zachariah ve ark., "Reliable BLE advertisement networks" (yıl ve yayın yeri ___). https://www.eecs.berkeley.edu/~prabal/pubs/papers/zachariah22reliable.pdf
[F] Espressif ESP32 Forumu, "ESP32 Bluetooth Scan Sensitivity." https://esp32.com/viewtopic.php?p=118375

## Ham çıktılar

Science AI Journal'dan indirilen karşılaştırma tablosu (CSV) ve rapor (PDF) bu klasöre eklenecektir.
