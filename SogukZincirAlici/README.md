# Soğuk Zincir BLE Sıcaklık Etiketi — Alıcı Alt Sistemi (ESP32 Ağ Geçidi)

KKÜ Elektrik-Elektronik Mühendisliği · Bitirme Projesi · Bireysel proje

## Konu

Bu proje, soğuk zincir sıcaklık etiketlerinin yaydığı BLE reklam paketlerini dinleyen bir ESP32 ağ geçidi geliştirir. Ağ geçidi gelen her ölçümü zaman damgasıyla kaydeder, sıcaklık WHO aşı buzdolabı alarm koşullarını aşınca alarm verir, kayıp paket oranını ve uçtan uca gecikmeyi ölçer.

**Terimler:**
- **BLE (Bluetooth Low Energy):** Az enerji harcayarak kısa mesafede veri gönderen kablosuz haberleşme türü.
- **Reklam paketi (advertising):** Etiketin bağlantı kurmadan, düzenli aralıklarla çevresine yaydığı kısa veri paketi; "reklam" burada "kendini duyurmak, yayın yapmak" anlamındadır.
- **Ağ geçidi (gateway):** Etiketlerden gelen veriyi toplayıp kaydeden ve işleyen ara cihaz.

## Kapsam

### Girdiler
- **Etiketin reklam paketleri:** Etiket kimliği, sıra numarası, sıcaklık, gönderim zamanı, pil durumu (ayrıntılar: Arayüz sözleşmesi).
- **Zaman referansı:** Ağ geçidinin dahili saati; isteğe bağlı olarak Wi-Fi üzerinden NTP ile ayarlanır. NTP, cihaz saatini internetteki zaman sunucularına göre ayarlayan protokoldür. Gecikme ölçümünde etiket ile ağ geçidi saatleri arasındaki fark GPIO darbeleriyle ölçülür.
- **Ayarlar:** Alarm eşikleri ve süre koşulları (WHO PQS E006/TR06.4, Type 1), histerezis payı, test için zaman ölçeği. Histerezis, sıcaklık sınır çevresinde gidip gelirken alarmın sürekli açılıp kapanmasını önleyen küçük güvenlik payıdır. Süre koşulu, alarmın açılması için sıcaklığın sınır dışında kesintisiz en az belirli bir süre kalması şartıdır; kapı açılışı gibi kısa sıçramaların alarm vermesini önler.
- **Güç:** USB besleme.

### Çıktılar
- **Zaman damgalı kayıt (seri port → CSV):** Her paket için alış zamanı, etiket adresi ve kimliği, sıcaklık, sıra numarası, gönderim zamanı, RSSI. RSSI, alınan sinyalin gücüdür (dBm, 1 miliwatt'a göre desibel).
- **Alarm:** Alarm açıkken yanan LED ve kayda düşen olay satırı (açılış ve kapanış zamanıyla).
- **Kayıp paket oranı:** Sıra numaralarındaki boşluklardan hesaplanır.
- **Uçtan uca gecikme:** Etiketin paketi gönderdiği an ile ağ geçidinin aldığı an arasındaki süre (saat ofseti ve kayması düzeltilmiş).

### Kapsam dışı
- Verici etiket: donanımı, sensörü, pili ve yazılımı ayrı bir projedir. İki proje arasındaki tek ortak nokta reklam paketinin veri formatıdır (Arayüz sözleşmesi). Arayüz sözleşmesi, hangi baytın neyi taşıdığını, birimini ve bayt sırasını belirleyen, iki tarafın birlikte yazdığı ortak kurallar belgesidir. Testlerde etiketin yerine onu taklit eden ikinci bir ESP32 (etiket simülatörü) kullanılır.
- Telefon uygulaması.
- Bulut, sunucu veya web paneli; veriler yalnızca ağ geçidinde ya da bağlı PC'de tutulur.
- BLE bağlantısı (GATT) ile veri indirme; yalnızca reklam paketleri dinlenir. GATT, iki BLE cihazının bağlantı kurup karşılıklı veri alışverişi yaptığı yapıdır.
- Sensör doğruluğu ve kalibrasyonu.
- Pil ile çalışma ve enerji optimizasyonu.
- Şifreleme ve güvenlik.
- PCB ve mekanik kutu tasarımı.
- Çok sayıda etiketin aynı anda yönetilmesi (kod en fazla 8 etiketi ayırt eder; test tek etiketle yapılır).

## Literatür özeti

BLE'nin IoT ekosistemindeki yeri, reklam ve tarama mekanizmaları ve gecikme gibi teknik özellikleri derleme çalışmalarında ele alınmış [1]; bağlantısız moddaki uygulamaları ve sınırlı veri yükü, diğer 2,4 GHz teknolojileriyle girişim gibi zorlukları da incelenmiştir [2]. Kablosuz sensör ağlarında paket kaybı oranı, paket teslim oranı ve uçtan uca gecikme yaygın başarım ölçütleri olarak kullanılmaktadır [4]–[7]. Ancak bu çalışmaların büyük bölümü benzetim ortamlarında yürütülmüş; fiziksel donanım üzerinde ölçüm yapan çalışmalar sınırlı sayıdadır [3], [8]. İncelenen çalışmalarda BLE reklam paketlerinin ağ geçidi tarafındaki kaybı, iki cihaz arasındaki saat ofseti düzeltilerek ölçülen uçtan uca gecikme ve histerezis ile süre koşulu içeren eşik alarmının doğruluğu birlikte ele alınmamıştır. Bu proje, soğuk zincir sıcaklık izleme bağlamında bu üç ölçütü gerçek bir ESP32 ağ geçidi üzerinde ölçerek bu boşluğa katkı sunmayı amaçlamaktadır.

Kaynak listesi, arama süreci, çalışmaların karşılaştırması ve hedef değerlerin dayandığı standart ve kaynaklar: [docs/literatur.md](docs/literatur.md)

## Başarı ölçütleri

Üç ölçüt **tek bir test koşusunda** birlikte ölçülür: Etiket simülatörü senaryoyu (3155 paket, ~53 dk) yayınlar; ağ geçidi ve simülatör kayıtları `tools/analyze.py` ile değerlendirilir.

| Ölçüt | Nasıl ölçülür (test kurulumu, süre, formül) | Hedef değer | Birim | Dayanak |
|---|---|---|---|---|
| **(a) Kayıp paket oranı** | **Kurulum:** Etiket simülatörü ağ geçidinden **1 m** ve **3 m** uzakta, saniyede bir yeni sıra numaralı reklam paketi yayar (tek yayın: her paket tek reklam olayında). Ağ geçidinde yinelenen paket filtresi kapalı, tarama penceresi = tarama aralığı = 100 ms.<br>**Süre:** 3155 paket (~53 dk), her mesafede **3 tekrar**.<br>**Formül:** N_beklenen = simülatörün gönderdiği sıra numarası sayısı; N_alınan = ağ geçidinin aldığı farklı sıra numarası sayısı.<br>Kayıp (%) = (N_beklenen − N_alınan) / N_beklenen × 100 | **≤ 5** (1 m) | % | Ağ geçitlerinde iyi koşullarda bile %5–50 kayıp gözlemi (Ruuvi) [B]; tekrarlı yayınla kaybın ~%0'a indirilebildiği deney [C]. 3 m sonucu hedefsiz raporlanır. |
| **(b) Uçtan uca gecikme** | **Kurulum:** Simülatör her pakete kendi saatine göre gönderim zamanını (t_gönderim, ms) yazar; ağ geçidi alış anını (t_alış) kendi saatiyle kaydeder. Simülatör 10 s'de bir kabloyla ağ geçidine bir GPIO darbesi yollar; iki kart darbeyi kendi saatiyle kaydeder. θ = t_alış,darbe − t_gönderim,darbe; θ'nın zamanla değişimi (saat kayması) doğrusal modelle bulunur.<br>**Süre:** (a) ile aynı koşu, her sıra numarasının ilk kopyası.<br>**Formül:** d_i = t_alış,i − t_gönderim,i − θ(t_alış,i); medyan ve 95. yüzdelik raporlanır. Saat kayması (ppm) ayrıca raporlanır. | **Medyan ≤ 100**<br>**p95 ≤ 1000** | ms | p95 sınırı bir yayın aralığıdır (1000 ms): bir paket sonraki paketten önce ulaşmalı. Medyan hedefi bir mühendislik tercihidir; ilk ölçümden sonra gözden geçirilecektir. Saatlerin kayması bağlamı: ticari kaydedicide ±25 ppm ≈ 2,5 s/gün [D]. |
| **(c) Eşik alarmı** | **Kurulum:** Simülatör önceden bilinen senaryoyu yayınlar: **3 gerçek aşım** (2 yüksek, 1 düşük) ve **4 tuzak** (eşik üstünde ama süre koşulundan kısa iki bölüm; eşik çevresinde salınım; açık alarm sırasında histerezis bandında salınım). Senaryo: `firmware/common/scenario.h`.<br>**Alarm kuralı:** Yüksek alarm: sıcaklık **+8 °C veya üstünde** kesintisiz **10 saat**; düşük alarm: **−0,5 °C veya altında** kesintisiz **60 dk**. Testte süreler **1/60** ölçeklenir (10 dk ve 1 dk). Kapanma: yüksek alarm **7,50 °C**'nin altında, düşük alarm **0,00 °C**'nin üstünde (histerezis **0,50 °C**).<br>**Formül:** N_yanlış = senaryoda beklenmeyen her alarm olayı (tuzakta açılma, salınımda erken kapanma); N_kaçırılan = alarm açılmayan gerçek aşım sayısı. | **N_yanlış = 0**<br>**N_kaçırılan = 0** | adet | Eşik ve süreler: WHO PQS E006/TR06.4 Type 1 [A]. WHO kapanma koşulunu tanımlamaz; histerezis payı WHO çözünürlük şartının (±0,2 °C) üstünde seçildi, sensör gürültüsü ölçülünce doğrulanacak. |

**Gerekçeler:**
- **(a)** Soğuk zincirde kaybolan her paket, o andaki sıcaklığın bilinmemesi demektir; bu yüzden alıcının kaç paketi kaçırdığının sayıyla bilinmesi gerekir.
- **(b)** Alarmın ne kadar geç fark edildiğini gösterir; iki cihazın saati farklı olduğundan ofset düzeltilmeden ölçülen gecikme saat farkını da içerir ve yanıltıcı olur.
- **(c)** Sık yanlış alarm kullanıcının alarmları görmezden gelmesine, kaçırılan alarm ise bozulmuş ürünün kullanılmasına yol açar. Süre koşulu kısa aşımların alarm açmasını, histerezis ise açık alarmın eşik çevresindeki salınımda erken kapanmasını önler.

**Not:** Hedef değerler ilk ölçüme kadar öneridir; ilk ölçüm sonuçlarıyla danışmanla birlikte gözden geçirilecektir. 2–8 °C ürünün saklama aralığıdır; alarm eşikleri ise WHO şartnamesindeki alarm koşullarından alınmıştır (düşük alarm, donma riski nedeniyle −0,5 °C'dedir).

## Arayüz sözleşmesi (BLE reklam paketi) — v0.1, verici tarafın onayı bekleniyor

Reklam paketinin "üreticiye özel veri" (AD tipi 0xFF) alanındaki düzen. Bayt konumları bu alanın başından sayılır; ilk iki bayt Company ID'dir. Kod karşılığı: `firmware/common/contract.h`.

| Alan | Bayt konumu | Uzunluk (bayt) | Veri tipi | Birim / çözünürlük | Not |
|---|---|---|---|---|---|
| Company ID | 0–1 | 2 | uint16 | — | 0xFFFF (Bluetooth SIG'in test/iç kullanım değeri) |
| Etiket kimliği | 2 | 1 | uint8 | — | Birden fazla etiketi ayırt etmek için |
| Sıra numarası | 3–4 | 2 | uint16 | — | Ölçüt (a) için gerekli; taşma: 65535'ten sonra 0'a döner, alıcı farkı mod 65536 ile hesaplar |
| Sıcaklık | 5–6 | 2 | int16 | 0,01 °C (4,25 °C = 425) | Verici tarafın önerisiyle aynı |
| Zaman damgası (gönderim zamanı) | 7–10 | 4 | uint32 | 1 ms | Ölçüt (b) için gerekli; etiket saatinin referansı: etiketin açılış anı (~49,7 günde sıfıra döner) |
| Pil | 11 | 1 | uint8 | %1 (0–100) | Verici tarafın önerisiyle aynı |

- **Toplam:** 12 bayt (Company ID dahil); 31 baytlık reklam yükü sınırının altında.
- **Bayt sırası:** little-endian (önce düşük bayt). Örnek: 425 = 0x01A9 → `A9 01`.
- **Reklam türü:** bağlanılamaz, yalnız yayın (ADV_NONCONN_IND).
- **Yayın aralığı:** Testte 1000 ms (simülatör). Gerçek etikette verici tarafın pil hesabına göre belirlenecek; WHO ölçüm aralığı şartı en fazla 10 dakikadır [A].
- **Sözleşme sürümü:** v0.1
- **Verici tarafın önerisinden farklar:** (1) sıra numarası eklendi (kayıp ölçümü için şart); (2) zaman damgası saniye yerine milisaniye ve "açılıştan beri geçen süre" olarak tanımlandı (milisaniyelik gecikme ölçümü için şart; etikette RTC yok).

## Kod

```
firmware/
  common/          Ortak, donanımdan bağımsız mantık (asıl kaynak)
    contract.h     Arayüz sözleşmesi: paket kodlama/çözme
    alarm.h        Süre koşullu + histerezisli alarm dedektörü
    scenario.h     Ölçüt (c) test senaryosu
  gateway/         ESP32 ağ geçidi (Arduino) — projenin asıl çıktısı
  tag_simulator/   Etiketi taklit eden ikinci ESP32 (yalnız test aracı)
tools/
  serial_logger.py Seri port kaydedici (pip install pyserial)
  analyze.py       Üç ölçütü hesaplayan analiz
tests/
  test_core.cpp    contract.h / alarm.h / senaryo birim testleri
  test_analyze.py  analyze.py'nin sahte kayıtlarla doğrulanması
```

**Kullanılan haberleşme ve arayüzler:**
- **BLE reklam yayını ve pasif tarama** (2,4 GHz, reklam kanalları 37/38/39): etiket → ağ geçidi. Radyo ESP32'nin içinde olduğu için pin kullanmaz.
- **UART (USB üzerinden seri port, 115200 baud):** ağ geçidi → bilgisayar (CSV kayıt). Kartın USB-seri dönüştürücüsü üzerinden.
- **GPIO kesmesi:** SYNC darbesi (saat ofseti ölçümü için); bir protokol değil, tek bir dijital sinyal.
- **Wi-Fi + NTP (isteğe bağlı):** ağ geçidi saatini gerçek tarih-saate ayarlamak için. Varsayılan olarak kapalı.
- Ağ geçidinde sensör olmadığı için I²C kullanılmaz; sensör ve I²C verici etiketin (ayrı projenin) parçasıdır.

### Pin bağlantıları

| Kart | Pin | Yön | Bağlantı | Görev |
|---|---|---|---|---|
| Ağ geçidi | GPIO4 | Giriş (kesme) | Simülatör GPIO4 | SYNC darbesini zaman damgasıyla kaydetmek |
| Ağ geçidi | GPIO2 | Çıkış | Kart üstü LED (veya harici LED + direnç) | Alarm göstergesi |
| Ağ geçidi | GND | — | Simülatör GND | Ortak referans (SYNC için şart) |
| Ağ geçidi | USB | — | Bilgisayar | Güç + seri kayıt |
| Simülatör | GPIO4 | Çıkış | Ağ geçidi GPIO4 | 10 s'de bir SYNC darbesi |
| Simülatör | GND | — | Ağ geçidi GND | Ortak referans |
| Simülatör | USB | — | Bilgisayar | Güç + seri kayıt (gerçeğin kaydı) |

Pin numaraları `gateway.ino` ve `tag_simulator.ino` başındaki `#define` satırlarından değiştirilebilir. Kart üstü LED her DevKit'te GPIO2'de olmayabilir; kartının pin şemasına bak.

### Malzeme listesi (prototip)

| Parça | Adet | Durum | Not |
|---|---|---|---|
| ESP32 geliştirme kartı — ağ geçidi | 1 | Alınacak (yerel elektronikçi veya online) | Fiyat: ___ TL. Bkz. "Kart seçimi" |
| ESP32 geliştirme kartı — etiket simülatörü | 1 | Alınacak / ödünç | Ağ geçidiyle aynı model olması kolaylık sağlar. Arkadaşın etiketi hazır olunca gerçek testte gerekmez. Fiyat: ___ TL |
| USB veri kablosu (kartın konnektörüne uygun: Micro-USB veya USB-C) | 2 | Alınacak / elimde | Yalnız şarj kablosu olmamalı, veri taşımalı. Bazı kartlar kabloyla satılır |
| Dişi-dişi jumper kablo | 2 + yedek | Alınacak / elimde | SYNC ve GND; kart pinlerine doğrudan takılır, breadboard gerekmez |
| Harici LED + 220–330 Ω direnç | 1 | Karta göre gerekli | ESP32 DevKit V1'de GPIO2'ye bağlı mavi LED vardır; DevKitC V4'te kullanıcı LED'i yoktur (yalnız güç LED'i), bu kartta harici LED şarttır. Bağlantı: GPIO2 → direnç → LED (+) → LED (−) → GND |
| Breadboard | 1 | İsteğe bağlı | Harici LED kullanılırsa. ESP32 kartları geniş olduğu için tek breadboard'u neredeyse kaplar |
| Şerit metre | 1 | Elimde / ödünç | Ölçüt (a): 1 m ve 3 m mesafe |
| Bilgisayar (2 USB portu) | 1 | Elimde | İki kartın kaydı aynı anda alınır |

#### Kart seçimi

Kartın "DevKit" olması şart değildir. Şu üç koşulu sağlayan her ESP32 geliştirme kartı uygundur:
1. **Bluetooth (BLE) olan bir ESP32 çipi:** klasik ESP32 (ESP32-WROOM-32 modüllü kartlar), ESP32-C3 veya ESP32-S3.
2. **Kart üstünde USB girişi ve USB-seri dönüştürücü:** bilgisayardan programlama ve kayıt için.
3. **Pin başlıkları (header):** GPIO4 ve GND'ye kablo takabilmek için.

Satıcılarda sık geçen ve uygun adlar: "ESP32 DevKit V1 (30 pin)", "ESP32 DevKitC V4 (38 pin)", "NodeMCU-32S", "ESP32-WROOM-32 geliştirme kartı", "LOLIN32". Tercih sırası: klasik ESP32 (kodun pin numaraları buna göre yazıldı) > ESP32-C3 / ESP32-S3 (çalışır, pin numaraları ve LED kontrol edilmeli).

**Almaman gerekenler:**
- **ESP32-S2 kartları:** Bluetooth yok, bu projede çalışmaz.
- **Tek başına ESP32 modülü (yalnız metal kapaklı modül, geliştirme kartı değil):** USB girişi ve regülatör yok; ayrıca USB-seri dönüştürücü ve besleme devresi gerekir.
- **ESP8266 (NodeMCU V2/V3, Wemos D1 mini):** Adı benzer ama Bluetooth yok.
- Sensör, pil, pil yuvası: Ağ geçidi USB'den beslenir ve sensör taşımaz; bunlar verici etiket projesine ait.

Bilgisayar kartı tanımazsa USB-seri sürücüsü kurulmalıdır (kartın üzerindeki küçük çipte genelde CP2102 veya CH340 yazar).

#### Yazılım gereksinimleri

| Yazılım | Gerekli mi | Not |
|---|---|---|
| Arduino IDE 2.x | Evet | Kartları programlamak için |
| ESP32 kart paketi (Espressif, "esp32 by Espressif Systems") | Evet | Arduino IDE → Boards Manager'dan kurulur; yerleşik BLE kütüphanesini içerir |
| USB-seri sürücüsü (CP210x veya CH340) | Karta göre | Bilgisayar kartı tanımazsa |
| Python 3.10 veya üstü | Evet | `analyze.py` 3.10 ile gelen `statistics.linear_regression` kullanır |
| pyserial (`pip install pyserial`) | Evet | `serial_logger.py` için |
| g++ (C++17) | İsteğe bağlı | Yalnız `tests/test_core.cpp` için; Windows'ta yoksa atlanabilir |
| 2,4 GHz Wi-Fi ağı | İsteğe bağlı | Yalnız NTP açılırsa; varsayılan kapalı |

#### Kart değişirse kontrol edilecekler

- Kod **klasik ESP32** için yazıldı (GPIO2 LED, GPIO4 SYNC).
- ESP32-C3 / ESP32-S3 kullanılırsa: GPIO numaralarını kartın pin şemasına göre kontrol et; LED pinini açılış (strapping) pini olmayan bir pine taşı.
- İki kartın GND'si ayrıca kabloyla bağlanmalı; aynı bilgisayara takılı olsalar da SYNC sinyali için ortak referans kablosu bırakılmamalı.

#### Nereden alınır

- **Yerel elektronik malzeme satıcıları (ör. Ankara Ulus):** ESP32 kartı, jumper kablo, LED, direnç ve USB kablo genelde bulunur. Stok değişken olduğu için gitmeden telefonla sor; yukarıdaki kart adlarını ve "Bluetooth'lu ESP32 mi, ESP8266 mı?" sorusunu sor.
- **Online elektronik siteleri:** Yerelde bulunamazsa. Kargo süresini plana kat.

**Derleme (Arduino IDE):** ESP32 kart paketini kur, `firmware/gateway/gateway.ino` ve `firmware/tag_simulator/tag_simulator.ino` dosyalarını ayrı ayrı yükle. Sketch klasörlerindeki `.h` dosyaları `firmware/common` içindekilerin kopyasıdır; ortak dosyada değişiklik yaparsan kopyaları güncelle.

**Bilgisayarda test (donanım gerekmez):**
```
g++ -std=c++17 -I firmware/common tests/test_core.cpp -o test_core && ./test_core
python tests/test_analyze.py
```

**Ölçüm koşusu:**
1. Ağ geçidini aç, kaydı başlat: `python tools/serial_logger.py <port1> data/<tarih>_gw.log`
2. Simülatörü aç, kaydı başlat: `python tools/serial_logger.py <port2> data/<tarih>_sim.log`
3. ~53 dk bekle (senaryonun bir turu), iki kaydı durdur.
4. `python tools/analyze.py data/<tarih>_gw.log data/<tarih>_sim.log`

## Şu anki durum

- [x] Konu tanımı yazıldı
- [x] Başarı ölçütleri ve hedef değerler yazıldı (ilk ölçümde gözden geçirilecek)
- [x] Literatür özeti yazıldı (Science AI Journal taraması + standart/kaynaklar)
- [x] Arayüz sözleşmesi v0.1 önerildi (verici tarafın onayı bekleniyor)
- [x] Ağ geçidi, etiket simülatörü ve analiz kodunun ilk sürümü yazıldı; donanımdan bağımsız mantık bilgisayarda test edildi
- [ ] Arduino kodlarının ESP32 üzerinde derlenip çalıştırılması (henüz yapılmadı)
- [ ] Gerçek etiketle gecikme ölçümü için verici tarafla SYNC bağlantısı ve kaydı üzerinde anlaşma (şu an yalnız etiket simülatörü destekliyor)
- [ ] İlk ölçüm koşusu (1 m) ve hedeflerin gözden geçirilmesi

## Yapay zekâ kullanım notu

Bu projede yapay zekâ (Claude) şu işler için kullanılmıştır:
- Konu tanımı, kapsam listesi, başarı ölçütü tablosu ve bu README'nin taslaklarının hazırlanması.
- BLE reklam paketi, RSSI, dBm ve e.i.r.p. gibi kavramların açıklanması.
- Literatür taraması Science AI Journal (Research Gap Finder) ile yapılmış; çıkan karşılaştırma tablosu Claude yardımıyla projeyle karşılaştırılıp özetlenmiştir. Kullanılan çalışmalar Google Scholar üzerinden doğrulanacaktır.
- Hedef değer önerileri, WHO PQS şartnamesi ve docs/literatur.md'deki kaynaklara dayanarak Claude ile hazırlanmıştır.
- Arayüz sözleşmesi önerisi, `firmware/`, `tools/` ve `tests/` klasörlerindeki kodun ilk sürümü Claude tarafından yazılmıştır. Donanımdan bağımsız kısımlar (paket çözme, alarm mantığı, analiz) bilgisayarda testlerle doğrulanmıştır; Arduino kodları henüz ESP32 üzerinde derlenmemiştir.

Taslaklar ve kod tarafımdan okunup anlaşılacak, proje kapsamına göre düzeltilecek ve donanım üzerinde doğrulanacaktır. Kodda yaptığım değişiklikler ve donanım testleri ayrı commit'lerle izlenecektir. Yapay zekâ kullanımı her aşamada bu bölüme ve proje günlüğüne eklenecektir.
