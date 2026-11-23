# Soğuk Zincir BLE Sıcaklık Etiketi — Alıcı Alt Sistemi (ESP32 Ağ Geçidi)

KKÜ Elektrik-Elektronik Mühendisliği · Bitirme Projesi · Bireysel proje

## 1. Konu

Bu proje, soğuk zincir sıcaklık etiketlerinin yaydığı BLE reklam paketlerini dinleyen bir ESP32 ağ geçidi geliştirmektedir. Ağ geçidi gelen her ölçümü zaman damgasıyla kaydeder, sıcaklık WHO aşı buzdolabı alarm koşullarını aştığında alarm üretir, kayıp paket oranını ve uçtan uca gecikmeyi ölçer.

**Terimler:**
- **BLE (Bluetooth Low Energy):** Düşük enerjiyle kısa mesafede veri aktaran kablosuz haberleşme teknolojisi.
- **Reklam paketi (advertising):** Bir cihazın bağlantı kurmadan, düzenli aralıklarla çevresine yaydığı kısa veri paketi. "Reklam" burada "yayın yapmak, kendini duyurmak" anlamındadır.
- **Ağ geçidi (gateway):** Etiketlerden gelen veriyi toplayan, kaydeden ve işleyen ara cihaz.

## 2. Kapsam

### 2.1 Girdiler
- **Etiketin reklam paketleri:** Etiket kimliği, sıra numarası, sıcaklık, gönderim zamanı, pil durumu (bkz. Bölüm 5).
- **Zaman referansı:** Ağ geçidinin dahili saati; isteğe bağlı olarak Wi-Fi üzerinden NTP (internet zaman sunucularıyla saat ayarlama protokolü) ile ayarlanır. Etiket ile ağ geçidi saatleri arasındaki fark, gecikme ölçümünde GPIO darbeleriyle belirlenir.
- **Ayarlar:** Alarm eşikleri ve süre koşulları (WHO PQS E006/TR06.4, Type 1), histerezis payı ve test için zaman ölçeği.
  - **Süre koşulu:** Alarmın açılması için sıcaklığın eşik dışında kesintisiz olarak en az belirli bir süre kalması şartı. Kapı açılışı gibi kısa sıçramaların alarm üretmesini önler.
  - **Histerezis:** Açık bir alarmın, sıcaklık eşik çevresinde salınırken erken kapanmasını önleyen güvenlik payı.
- **Güç:** USB besleme.

### 2.2 Çıktılar
- **Zaman damgalı kayıt (seri port üzerinden CSV):** Her paket için alış zamanı, etiket adresi ve kimliği, sıcaklık, sıra numarası, gönderim zamanı ve RSSI (alınan sinyal gücü, dBm cinsinden; 1 miliwatt'a göre desibel).
- **Alarm:** Alarm süresince yanan LED ve kayda yazılan olay satırı (açılış ve kapanış zamanıyla).
- **Kayıp paket oranı:** Sıra numaralarındaki boşluklardan hesaplanır.
- **Uçtan uca gecikme:** Etiketin paketi gönderdiği an ile ağ geçidinin paketi aldığı an arasındaki süre; saat ofseti ve kayması düzeltilmiş olarak hesaplanır.

### 2.3 Kapsam dışı
- **Verici etiket:** Donanımı, sensörü, pili ve yazılımı ayrı bir projenin konusudur. İki proje arasındaki tek ortak nokta reklam paketinin veri formatıdır (bkz. Bölüm 5). Testlerde verici etiketin yerine, onu taklit eden ikinci bir ESP32 (etiket simülatörü) kullanılır.
- Telefon uygulaması.
- Bulut, sunucu veya web paneli; veriler yalnızca ağ geçidinde ve bağlı bilgisayarda tutulur.
- BLE bağlantısı (GATT) üzerinden veri indirme; yalnızca reklam paketleri dinlenir. GATT, iki BLE cihazının bağlantı kurarak karşılıklı veri alışverişi yaptığı yapıdır.
- Sensör doğruluğu ve kalibrasyonu.
- Pil ile çalışma ve enerji optimizasyonu.
- Şifreleme ve güvenlik.
- PCB ve mekanik kutu tasarımı.
- Çok sayıda etiketin eşzamanlı yönetimi (yazılım en fazla 8 etiketi ayırt eder; testler tek etiketle yapılır).

## 3. Literatür özeti

BLE'nin IoT ekosistemindeki yeri, reklam ve tarama mekanizmaları ve gecikme gibi teknik özellikleri derleme çalışmalarında ele alınmış [1]; bağlantısız moddaki uygulamaları ile sınırlı veri yükü ve diğer 2,4 GHz teknolojileriyle girişim gibi zorlukları da incelenmiştir [2]. Kablosuz sensör ağlarında paket kaybı oranı, paket teslim oranı ve uçtan uca gecikme yaygın başarım ölçütleri olarak kullanılmaktadır [4]–[7]. Ancak bu çalışmaların büyük bölümü benzetim ortamlarında yürütülmüş; fiziksel donanım üzerinde ölçüm yapan çalışmalar sınırlı sayıdadır [3], [8]. İncelenen çalışmalarda BLE reklam paketlerinin ağ geçidi tarafındaki kaybı, iki cihaz arasındaki saat ofseti düzeltilerek ölçülen uçtan uca gecikme ve histerezis ile süre koşulu içeren eşik alarmının doğruluğu birlikte ele alınmamıştır. Bu proje, soğuk zincir sıcaklık izleme bağlamında bu üç ölçütü gerçek bir ESP32 ağ geçidi üzerinde ölçerek bu boşluğa katkı sunmayı amaçlamaktadır.

Kaynak listesi, arama süreci, çalışmaların karşılaştırması ve hedef değerlerin dayandığı standart ve kaynaklar: [docs/literatur.md](docs/literatur.md)

## 4. Başarı ölçütleri

Üç ölçüt tek bir test koşusunda birlikte ölçülür: Etiket simülatörü önceden tanımlı senaryoyu (3155 paket, yaklaşık 53 dakika) yayınlar; ağ geçidi ve simülatör kayıtları `tools/analyze.py` ile değerlendirilir.

| Ölçüt | Ölçüm yöntemi (test kurulumu, süre, formül) | Hedef değer | Birim | Dayanak |
|---|---|---|---|---|
| **(a) Kayıp paket oranı** | **Kurulum:** Etiket simülatörü ağ geçidinden 1 m ve 3 m uzaklıkta, saniyede bir yeni sıra numaralı reklam paketi yayınlar; her paket tek bir reklam olayında gönderilir. Ağ geçidinde yinelenen paket filtresi kapalıdır; tarama penceresi ve tarama aralığı 100 ms'dir.<br>**Süre:** 3155 paket (~53 dk), her mesafede 3 tekrar.<br>**Formül:** N_beklenen = simülatörün gönderdiği sıra numarası sayısı; N_alınan = ağ geçidinin aldığı farklı sıra numarası sayısı.<br>Kayıp (%) = (N_beklenen − N_alınan) / N_beklenen × 100 | ≤ 5 (1 m) | % | Ağ geçitlerinde iyi koşullarda dahi %5–50 kayıp gözlemi [B]; tekrarlı yayınla kaybın ~%0'a indirilebildiğini gösteren deney [C]. 3 m sonucu hedefsiz raporlanır. |
| **(b) Uçtan uca gecikme** | **Kurulum:** Simülatör her pakete kendi saatine göre gönderim zamanını (t_gönderim, ms) yazar; ağ geçidi alış anını (t_alış) kendi saatiyle kaydeder. Simülatör 10 saniyede bir kablo üzerinden ağ geçidine bir GPIO darbesi gönderir; iki kart darbeyi kendi saatleriyle kaydeder. θ = t_alış,darbe − t_gönderim,darbe; θ'nın zamana bağlı değişimi (saat kayması) doğrusal modelle belirlenir.<br>**Süre:** (a) ile aynı koşu; her sıra numarasının ilk kopyası kullanılır.<br>**Formül:** d_i = t_alış,i − t_gönderim,i − θ(t_alış,i); medyan, 95. yüzdelik ve saat kayması (ppm) raporlanır. | Medyan ≤ 100<br>p95 ≤ 1000 | ms | p95 sınırı bir yayın aralığına (1000 ms) eşittir: Bir paketin bir sonraki paketten önce ulaşması gerekir. Medyan hedefi bir mühendislik tercihidir ve ilk ölçümden sonra gözden geçirilecektir. Saat kayması bağlamı: ticari bir kaydedicide ±25 ppm ≈ 2,5 s/gün [D]. |
| **(c) Eşik alarmı** | **Kurulum:** Simülatör önceden bilinen senaryoyu yayınlar: 3 gerçek aşım (2 yüksek, 1 düşük) ve 4 tuzak bölüm (eşik üstünde ancak süre koşulundan kısa iki bölüm; eşik çevresinde salınım; açık alarm sırasında histerezis bandında salınım). Senaryo: `firmware/common/scenario.h`.<br>**Alarm kuralı:** Yüksek alarm: +8 °C veya üstünde kesintisiz 10 saat. Düşük alarm: −0,5 °C veya altında kesintisiz 60 dakika. Testte süreler 1/60 oranında ölçeklenir (10 dk ve 1 dk). Kapanma: yüksek alarm 7,50 °C'nin altında, düşük alarm 0,00 °C'nin üstünde (histerezis 0,50 °C).<br>**Formül:** N_yanlış = senaryoda beklenmeyen her alarm olayı (tuzakta açılma, salınımda erken kapanma); N_kaçırılan = alarm açılmayan gerçek aşım sayısı. | N_yanlış = 0<br>N_kaçırılan = 0 | adet | Eşik ve süreler: WHO PQS E006/TR06.4 Type 1 [A]. WHO şartnamesi alarmın kapanma koşulunu tanımlamaz; histerezis payı, WHO çözünürlük şartının (±0,2 °C) üzerinde seçilmiştir ve sensör gürültüsü ölçüldüğünde doğrulanacaktır. |

**Gerekçeler:**
- **(a)** Soğuk zincirde kaybolan her paket, o andaki sıcaklığın bilinmemesi anlamına gelir; bu nedenle alıcının kaçırdığı paket sayısının nicel olarak bilinmesi gerekir.
- **(b)** Alarmın ne kadar gecikmeyle fark edildiğini gösterir. İki cihazın saatleri farklı olduğundan, ofset düzeltilmeden ölçülen gecikme saat farkını da içerir ve yanıltıcı olur.
- **(c)** Sık yanlış alarm, kullanıcının alarmları göz ardı etmesine; kaçırılan alarm ise bozulmuş ürünün kullanılmasına yol açar. Süre koşulu kısa aşımların alarm açmasını, histerezis ise açık alarmın eşik çevresindeki salınımda erken kapanmasını önler.

**Not:** Hedef değerler ilk ölçüme kadar öneri niteliğindedir ve ilk ölçüm sonuçlarıyla danışmanla birlikte gözden geçirilecektir. 2–8 °C ürünün saklama aralığıdır; alarm eşikleri ise WHO şartnamesindeki alarm koşullarından alınmıştır (düşük alarm, donma riski nedeniyle −0,5 °C'dedir).

**Ölçüm ortamı:** Üç ölçüt de etiket simülatörüyle ölçülür. Gerçek etiketle gecikme ölçümü, etiket yazılımında saat senkronizasyonu (SYNC) desteği gerektirdiğinden kapsam dışıdır. Gerçek etiketle yalnızca uyumluluk testi yapılır (bkz. Bölüm 5.3).

## 5. Arayüz sözleşmesi (BLE reklam paketi) — v1.0

Arayüz sözleşmesi alıcı alt sistemi tarafından tanımlanmıştır. Bu formata uyan her verici (gerçek etiket veya etiket simülatörü) ağ geçidi tarafından çözülebilir. Verici etiket projesi için uygulama rehberi ve örnek kod: [docs/arayuz-sozlesmesi.md](docs/arayuz-sozlesmesi.md). Kod karşılığı: `firmware/common/contract.h`.

### 5.1 Paket düzeni

Reklam paketinin "üreticiye özel veri" (AD tipi 0xFF) alanındaki düzen aşağıdadır. Bayt konumları bu alanın başından sayılır.

| Alan | Bayt konumu | Uzunluk (bayt) | Veri tipi | Birim / çözünürlük | Not |
|---|---|---|---|---|---|
| Company ID | 0–1 | 2 | uint16 | — | 0xFFFF (Bluetooth SIG'in test ve iç kullanım için ayırdığı değer) |
| Etiket kimliği | 2 | 1 | uint8 | — | Her etikete sabit, benzersiz bir değer (1–254) |
| Sıra numarası | 3–4 | 2 | uint16 | — | Her yeni ölçümde 1 artar; 65535'ten sonra 0'a döner. Ölçüt (a) için gereklidir |
| Sıcaklık | 5–6 | 2 | int16 | 0,01 °C | Örnek: 4,25 °C = 425; −1,00 °C = −100. Sensör okunamazsa −32768 (0x8000) gönderilir |
| Zaman damgası (gönderim zamanı) | 7–10 | 4 | uint32 | 1 ms | Etiketin açılışından beri geçen süre (Arduino `millis()`); ~49,7 günde sıfıra döner. Ölçüt (b) için gereklidir |
| Pil | 11 | 1 | uint8 | %1 | 0–100; bilinmiyorsa 255 (0xFF) |

- **Toplam uzunluk:** 12 bayt (Company ID dahil); 31 baytlık reklam yükü sınırının altındadır.
- **Bayt sırası:** Little-endian (önce düşük bayt). Örnek: 425 = 0x01A9 → `A9 01`.
- **Test vektörü:** Etiket kimliği 1, sıra numarası 2, sıcaklık 4,25 °C, zaman damgası 0x11223344, pil %100 → `FF FF 01 02 00 A9 01 44 33 22 11 64`.

### 5.2 Verici için kurallar

1. **Reklam türü:** Bağlanılamaz, yalnızca yayın (non-connectable advertising).
2. **Sıra numarası:** Yalnızca yeni bir sıcaklık ölçümü yapıldığında artırılır. Aynı ölçümün birden fazla reklam olayında tekrar yayınlanmasına izin verilir; ağ geçidi kopyaları sıra numarasından ayıklar.
3. **Zaman damgası:** Paket oluşturulduğu anda `millis()` değeri yazılır.
4. **Ölçüm ve yayın aralığı:** En fazla 10 dakika (WHO ölçüm aralığı şartı [A]). Aralığın kendisi verici projenin pil bütçesine göre seçilir; ağ geçidi her aralığı destekler. Etiket simülatörü testlerde 1000 ms kullanır.
5. **Sensör hatası:** Sıcaklık okunamazsa paket yine gönderilir ve sıcaklık alanına −32768 yazılır; ağ geçidi bu paketi kaydeder ancak alarm değerlendirmesine katmaz.

### 5.3 Gerçek etiketle uyumluluk testi

Gerçek etiket bu formatı uyguladığında ağ geçidiyle birlikte şu kontroller yapılır:
- Ağ geçidi etiketin paketlerini çözebiliyor mu (kayıtta `RX` satırları)?
- Etiketin kendi seri çıktısındaki sıcaklık değeri ile ağ geçidinin kaydettiği değer aynı mı?
- Sıra numarası her ölçümde 1 artıyor mu?

### 5.4 Sürüm geçmişi

- **v1.0:** Alıcı alt sistemi tarafından kesinleştirildi; sensör hatası (−32768) ve bilinmeyen pil (255) değerleri eklendi.
- **v0.1:** Verici projenin ilk önerisi (sıcaklık, saniye çözünürlüklü zaman damgası, pil; 7 bayt) temel alınarak hazırlandı. Kayıp ölçümü için sıra numarası eklendi; milisaniye düzeyinde gecikme ölçümü için zaman damgası milisaniyeye ve etikette RTC bulunmadığından "açılıştan beri geçen süre"ye çevrildi.

## 6. Sistem mimarisi

### 6.1 Haberleşme ve arayüzler

| Arayüz | Yön | Görev |
|---|---|---|
| BLE reklam yayını ve pasif tarama (2,4 GHz, kanal 37/38/39) | Etiket → ağ geçidi | Sıcaklık paketlerinin iletimi. Radyo ESP32'nin içindedir, pin kullanmaz. |
| UART (USB üzerinden seri port, 115200 baud) | Ağ geçidi → bilgisayar | Kayıtların CSV satırları olarak aktarılması |
| GPIO kesmesi | Simülatör → ağ geçidi | SYNC darbesinin zaman damgasıyla kaydedilmesi (saat ofseti ölçümü) |
| Wi-Fi + NTP (isteğe bağlı, varsayılan kapalı) | Ağ geçidi → internet | Saatin gerçek tarih-saate ayarlanması |

Ağ geçidinde sensör bulunmadığından I²C kullanılmaz; sensör ve I²C bağlantısı verici etiket projesinin parçasıdır.

### 6.2 Pin bağlantıları

| Kart | Pin | Yön | Bağlantı | Görev |
|---|---|---|---|---|
| Ağ geçidi | GPIO4 | Giriş (kesme) | Simülatör GPIO4 | SYNC darbesinin zaman damgasıyla kaydı |
| Ağ geçidi | GPIO2 | Çıkış | Kart üstü LED veya harici LED + direnç | Alarm göstergesi |
| Ağ geçidi | GND | — | Simülatör GND | Ortak referans (SYNC için gerekli) |
| Ağ geçidi | USB | — | Bilgisayar | Güç ve seri kayıt |
| Simülatör | GPIO4 | Çıkış | Ağ geçidi GPIO4 | 10 saniyede bir SYNC darbesi |
| Simülatör | GND | — | Ağ geçidi GND | Ortak referans |
| Simülatör | USB | — | Bilgisayar | Güç ve seri kayıt (referans kaydı) |

Pin numaraları `gateway.ino` ve `tag_simulator.ino` dosyalarının başındaki `#define` satırlarından değiştirilebilir. İki kart aynı bilgisayara bağlı olsa da GND bağlantısı ayrıca kabloyla yapılmalıdır.

## 7. Malzemeler

### 7.1 Malzeme listesi (prototip)

| Parça | Adet | Gereklilik | Not |
|---|---|---|---|
| ESP32 geliştirme kartı (ağ geçidi) | 1 | Gerekli | Bkz. 7.2. Fiyat: ___ TL |
| ESP32 geliştirme kartı (etiket simülatörü) | 1 | Gerekli (test için) | Ağ geçidiyle aynı modelin seçilmesi kurulumu kolaylaştırır. Verici etiket hazır olduğunda gerçek ölçümde gerekmez. Fiyat: ___ TL |
| USB veri kablosu (kartın konnektörüne uygun: Micro-USB veya USB-C) | 2 | Gerekli | Veri taşıyan kablo olmalıdır; yalnızca şarj kablosu yeterli değildir. Bazı kartlar kabloyla birlikte satılır |
| Dişi-dişi jumper kablo | 2 + yedek | Gerekli | SYNC ve GND bağlantısı; kart pinlerine doğrudan takılır |
| LED + 220–330 Ω direnç | 1 | Karta göre | ESP32 DevKit V1'de GPIO2'ye bağlı kart üstü LED bulunur. DevKitC V4'te kullanıcı LED'i bulunmadığından harici LED gereklidir. Bağlantı: GPIO2 → direnç → LED (+), LED (−) → GND |
| Breadboard (400 delik) | 1 | İsteğe bağlı | Yalnızca harici LED bağlantısı için |
| Şerit metre | 1 | Gerekli | Ölçüt (a) için 1 m ve 3 m mesafe ayarı |
| Bilgisayar (2 USB portu) | 1 | Gerekli | İki kartın kaydının eşzamanlı alınması |

Ağ geçidi USB'den beslendiği ve sensör taşımadığı için sensör, pil ve pil yuvası bu alt sistemin malzeme listesinde yer almaz.

### 7.2 Kart seçim ölçütleri

Kartın "DevKit" adını taşıması gerekmez. Aşağıdaki üç koşulu sağlayan ESP32 geliştirme kartları uygundur:
1. **BLE destekli ESP32 çipi:** Klasik ESP32 (ESP32-WROOM-32 modüllü kartlar), ESP32-C3 veya ESP32-S3.
2. **Kart üstü USB girişi ve USB-seri dönüştürücü:** Programlama ve seri kayıt için.
3. **Pin başlıkları:** GPIO4 ve GND bağlantıları için.

Uygun ticari adlara örnekler: ESP32 DevKit V1 (30 pin), ESP32 DevKitC V4 (38 pin), NodeMCU-32S, LOLIN32. Yazılım klasik ESP32 pin numaralarına göre hazırlanmıştır; ESP32-C3 ve ESP32-S3 kartlarında GPIO numaraları kartın pin şemasına göre kontrol edilmeli ve LED pini açılış (strapping) pini olmayan bir pine taşınmalıdır.

**Uygun olmayan kartlar:**
- **ESP32-S2:** Bluetooth desteği yoktur.
- **ESP8266 tabanlı kartlar (NodeMCU V2/V3, Wemos D1 mini):** Adları benzer olsa da Bluetooth desteği yoktur.
- **Yalnız ESP32 modülü (geliştirme kartı olmadan):** USB girişi ve regülatör içermez; ek USB-seri dönüştürücü ve besleme devresi gerektirir.

Bilgisayar kartı tanımazsa, kart üzerindeki USB-seri çipine (genellikle CP2102 veya CH340) uygun sürücü kurulmalıdır.

### 7.3 Tedarik

Listedeki parçalar yerel elektronik malzeme satıcılarında ve çevrim içi elektronik sitelerinde bulunabilmektedir. Stok durumu değişken olduğundan, satın almadan önce kartın BLE destekli bir ESP32 olduğu (ESP8266 olmadığı) teyit edilmelidir.

## 8. Yazılım gereksinimleri

| Yazılım | Gereklilik | Not |
|---|---|---|
| Arduino IDE 2.x | Gerekli | Kartların programlanması |
| ESP32 kart paketi ("esp32 by Espressif Systems") | Gerekli | Arduino IDE Boards Manager üzerinden kurulur; yerleşik BLE kütüphanesini içerir |
| USB-seri sürücüsü (CP210x veya CH340) | Karta göre | Kart bilgisayar tarafından tanınmazsa |
| Python 3.10 veya üstü | Gerekli | `analyze.py`, Python 3.10 ile gelen `statistics.linear_regression` fonksiyonunu kullanır |
| pyserial (`pip install pyserial`) | Gerekli | `serial_logger.py` için |
| g++ (C++17) | İsteğe bağlı | Yalnızca `tests/test_core.cpp` için |
| 2,4 GHz Wi-Fi ağı | İsteğe bağlı | Yalnızca NTP etkinleştirilirse |

## 9. Kurulum ve kullanım

### 9.1 Depo yapısı

```
firmware/
  common/          Donanımdan bağımsız ortak mantık (asıl kaynak)
    contract.h     Arayüz sözleşmesi: paket kodlama ve çözme
    alarm.h        Süre koşullu ve histerezisli alarm dedektörü
    scenario.h     Ölçüt (c) test senaryosu
  gateway/         ESP32 ağ geçidi yazılımı (alt sistemin çıktısı)
  tag_simulator/   Etiketi taklit eden ikinci ESP32 yazılımı (test aracı)
tools/
  serial_logger.py Seri port kaydedici
  analyze.py       Üç başarı ölçütünün hesaplanması
tests/
  test_core.cpp    contract.h, alarm.h ve senaryo birim testleri
  test_analyze.py  analyze.py'nin yapay kayıtlarla doğrulanması
docs/
  literatur.md     Literatür taraması ve kaynaklar
  arayuz-sozlesmesi.md  Verici etiket için arayüz sözleşmesi uygulama rehberi
```

### 9.2 Derleme

`firmware/gateway/gateway.ino` ve `firmware/tag_simulator/tag_simulator.ino` dosyaları Arduino IDE ile ilgili kartlara ayrı ayrı yüklenir. Sketch klasörlerindeki `.h` dosyaları `firmware/common` içindekilerin kopyasıdır; ortak dosyalarda yapılan değişiklikler kopyalara da aktarılmalıdır.

### 9.3 Donanımsız testler

```
g++ -std=c++17 -I firmware/common tests/test_core.cpp -o test_core && ./test_core
python tests/test_analyze.py
```

### 9.4 Ölçüm koşusu

1. Ağ geçidi çalıştırılır ve kaydı başlatılır: `python tools/serial_logger.py <port1> data/<tarih>_gw.log`
2. Simülatör çalıştırılır ve kaydı başlatılır: `python tools/serial_logger.py <port2> data/<tarih>_sim.log`
3. Senaryonun bir turu (~53 dakika) tamamlandıktan sonra iki kayıt durdurulur.
4. Analiz çalıştırılır: `python tools/analyze.py data/<tarih>_gw.log data/<tarih>_sim.log`

## 10. Proje durumu

- [x] Konu tanımı
- [x] Başarı ölçütleri ve hedef değerler (ilk ölçümde gözden geçirilecek)
- [x] Literatür özeti (Science AI Journal taraması ile standart ve ek kaynaklar)
- [x] Arayüz sözleşmesi v1.0 (alıcı alt sistemi tarafından tanımlandı; verici proje için uygulama rehberi hazırlandı)
- [x] Ağ geçidi, etiket simülatörü ve analiz yazılımlarının ilk sürümü; donanımdan bağımsız mantığın bilgisayar üzerinde test edilmesi
- [ ] Arduino yazılımlarının ESP32 üzerinde derlenmesi ve çalıştırılması
- [ ] Gerçek etiketle uyumluluk testi (verici etiket arayüz sözleşmesi v1.0'ı uyguladıktan sonra)
- [ ] İlk ölçüm koşusu (1 m) ve hedef değerlerin gözden geçirilmesi

## 11. Yapay zekâ kullanım notu

Bu projede yapay zekâ (Claude) aşağıdaki işlerde kullanılmıştır:
- Konu tanımı, kapsam listesi, başarı ölçütü tablosu ve bu README'nin taslaklarının hazırlanması.
- BLE reklam paketi, RSSI, dBm ve e.i.r.p. gibi kavramların açıklanması.
- Science AI Journal (Research Gap Finder) ile yapılan literatür taramasının sonuçlarının projeyle karşılaştırılması ve özetlenmesi. Kullanılan çalışmalar Google Scholar üzerinden doğrulanacaktır.
- WHO PQS şartnamesi ve `docs/literatur.md`'deki kaynaklara dayanan hedef değer önerilerinin hazırlanması.
- Arayüz sözleşmesi önerisinin ve `firmware/`, `tools/`, `tests/` klasörlerindeki yazılımın ilk sürümünün yazılması. Donanımdan bağımsız kısımlar (paket çözme, alarm mantığı, analiz) bilgisayar üzerinde testlerle doğrulanmıştır; Arduino yazılımları henüz ESP32 üzerinde derlenmemiştir.

Yapay zekâ ile üretilen taslaklar ve yazılım, proje sahibi tarafından incelenecek, proje kapsamına göre düzeltilecek ve donanım üzerinde doğrulanacaktır. Proje sahibinin yazılımda yaptığı değişiklikler ve donanım testleri ayrı commit'lerle izlenecektir. Yapay zekâ kullanımı her aşamada bu bölüme ve proje günlüğüne eklenecektir.
