# Soğuk Zincir BLE Sıcaklık Etiketi — Alıcı Alt Sistemi (ESP32 Ağ Geçidi)

KKÜ Elektrik-Elektronik Mühendisliği · Bitirme Projesi · Bireysel proje

## Konu

Bu proje, soğuk zincir sıcaklık etiketlerinin yaydığı BLE reklam paketlerini dinleyen bir ESP32 ağ geçidi geliştirir. Ağ geçidi gelen her ölçümü zaman damgasıyla kaydeder, sıcaklık 2–8 °C dışına çıkınca alarm verir, kayıp paket oranını ve uçtan uca gecikmeyi ölçer.

**Terimler:**
- **BLE (Bluetooth Low Energy):** Az enerji harcayarak kısa mesafede veri gönderen kablosuz haberleşme türü.
- **Reklam paketi (advertising):** Etiketin bağlantı kurmadan, düzenli aralıklarla çevresine yaydığı kısa veri paketi; "reklam" burada "kendini duyurmak, yayın yapmak" anlamındadır.
- **Ağ geçidi (gateway):** Etiketlerden gelen veriyi toplayıp kaydeden ve işleyen ara cihaz.

## Kapsam

### Girdiler
- **Etiketin reklam paketleri:** Etiket kimliği, sıcaklık, sıra numarası, gönderim zamanı, pil durumu (ayrıntılar: Arayüz sözleşmesi).
- **Zaman referansı:** Ağ geçidinin saati Wi-Fi üzerinden NTP ile ayarlanır. NTP, cihaz saatini internetteki zaman sunucularına göre ayarlayan protokoldür.
- **Ayarlar:** Alarm eşikleri (2 °C ve 8 °C), histerezis payı, süre koşulu, takip edilecek etiket kimlikleri. Histerezis, sıcaklık sınır çevresinde gidip gelirken alarmın sürekli açılıp kapanmasını önleyen küçük güvenlik payıdır.
- **Güç:** USB besleme.

### Çıktılar
- **Zaman damgalı kayıt dosyası (CSV):** Her paket için alış zamanı, etiket kimliği, sıcaklık, sıra numarası, RSSI. RSSI, alınan sinyalin gücüdür (dBm, 1 miliwatt'a göre desibel).
- **Alarm:** Eşik dışına çıkışta yanan LED ve kayıt dosyasına düşen olay satırı (başlangıç ve bitiş zamanıyla).
- **Kayıp paket oranı:** Sıra numaralarındaki boşluklardan hesaplanır.
- **Uçtan uca gecikme:** Etiketin paketi gönderdiği an ile ağ geçidinin aldığı an arasındaki süre (saat ofseti düzeltilmiş).

### Kapsam dışı
- Verici etiket: donanımı, sensörü, pili ve yazılımı ayrı bir projedir. İki proje arasındaki tek ortak nokta reklam paketinin veri formatıdır (Arayüz sözleşmesi).
- Telefon uygulaması.
- Bulut, sunucu veya web paneli.
- BLE bağlantısı (GATT) ile veri indirme; yalnızca reklam paketleri dinlenir.
- Sensör doğruluğu ve kalibrasyonu.
- Pil ile çalışma ve enerji optimizasyonu.
- Şifreleme ve güvenlik.
- PCB ve mekanik kutu tasarımı.
- Çok sayıda etiketin aynı anda yönetilmesi.

## Başarı ölçütleri

| Ölçüt | Nasıl ölçülür (test kurulumu, süre, formül) | Hedef değer | Birim |
|---|---|---|---|
| **(a) Kayıp paket oranı** | **Kurulum:** Etiket (ya da etiketi taklit eden ikinci ESP32) ağ geçidinden ___ m uzakta, sabit aralıkla reklam paketi yayar; ağ geçidi her paketin sıra numarasını kaydeder.<br>**Süre:** ___ dakika, ___ tekrar.<br>**Formül:** N_beklenen = son_sıra − ilk_sıra + 1; N_alınan = alınan farklı sıra numarası sayısı.<br>Kayıp (%) = (N_beklenen − N_alınan) / N_beklenen × 100 | ___ | % |
| **(b) Uçtan uca gecikme** | **Kurulum:** Etiket her pakete kendi saatine göre gönderim zamanını (t_gönderim) yazar; ağ geçidi alış anını (t_alış) kaydeder. İki saat arasındaki ofset (θ), iki cihazın aynı anda gördüğü ortak bir olayla ölçülür (örneğin kabloyla iki cihaza aynı anda gelen GPIO darbesi): θ = t_alış,darbe − t_gönderim,darbe.<br>**Süre:** ___ paket, ___ tekrar.<br>**Formül:** d_i = t_alış,i − t_gönderim,i − θ; d'nin medyanı ve 95. yüzdelik değeri raporlanır. | ___ (medyan)<br>___ (95. yüzdelik) | ms |
| **(c) Eşik alarmı** | **Kurulum:** Etiketi taklit eden ESP32, önceden yazılmış bir sıcaklık senaryosu yayınlar: ___ gerçek sınır aşımı ve sınır çevresinde gidip gelen ___ bölüm.<br>**Alarm kuralı:** Sıcaklık 8 °C'nin üstünde (ya da 2 °C'nin altında) en az ___ s kalırsa alarm açılır; 8 − ___ °C'nin altına (ya da 2 + ___ °C'nin üstüne) inince kapanır.<br>**Formül:** N_yanlış = senaryoda olmayan ama verilen alarm sayısı; N_kaçırılan = senaryoda olup verilmeyen alarm sayısı. | N_yanlış = ___<br>N_kaçırılan = ___ | adet |

**Gerekçeler:**
- **(a)** Soğuk zincirde kaybolan her paket, o andaki sıcaklığın bilinmemesi demektir; bu yüzden alıcının kaç paketi kaçırdığının sayıyla bilinmesi gerekir.
- **(b)** Alarmın ne kadar geç fark edildiğini gösterir; iki cihazın saati farklı olduğundan ofset düzeltilmeden ölçülen gecikme saat farkını da içerir ve yanıltıcı olur.
- **(c)** Sık yanlış alarm kullanıcının alarmları görmezden gelmesine, kaçırılan alarm ise bozulmuş ürünün kullanılmasına yol açar; histerezis ve süre koşulu ikisini birlikte dengelemek için vardır.

## Arayüz sözleşmesi (BLE reklam paketi)

Verici etiket projesiyle birlikte doldurulacaktır. Reklam paketinin "üreticiye özel veri" alanındaki düzen:

| Alan | Bayt konumu | Uzunluk (bayt) | Veri tipi | Birim / çözünürlük | Not |
|---|---|---|---|---|---|
| Etiket kimliği | ___ | ___ | ___ | — | Birden fazla etiketi ayırt etmek için |
| Sıra numarası | ___ | ___ | ___ | — | Ölçüt (a) için gerekli; taşma (sıfıra dönme) davranışı: ___ |
| Sıcaklık | ___ | ___ | ___ | ___ | |
| Zaman damgası (gönderim zamanı) | ___ | ___ | ___ | ___ | Ölçüt (b) için gerekli; etiket saatinin referansı: ___ |
| Pil | ___ | ___ | ___ | ___ | |

- **Bayt sırası:** ___ (little-endian / big-endian). Bayt sırası, çok baytlı bir sayının hangi baytının önce gönderildiğini belirler.
- **Yayın aralığı:** ___
- **Sözleşme sürümü:** ___

## Şu anki durum

- [x] Konu tanımı yazıldı
- [x] Başarı ölçütleri yazıldı (hedef değerler belirlenecek)
- [ ] Arayüz sözleşmesi (verici etiket projesiyle birlikte)

## Yapay zekâ kullanım notu

Bu projede yapay zekâ (Claude) şu işler için kullanılmıştır:
- Konu tanımı, kapsam listesi, başarı ölçütü tablosu ve bu README'nin taslaklarının hazırlanması.
- BLE reklam paketi, RSSI, dBm ve e.i.r.p. gibi kavramların açıklanması.

Taslaklar tarafımdan okunmuş, proje kapsamına göre düzeltilmiş ve onaylanmıştır. Tüm hedef değerler, test ayarları ve arayüz sözleşmesindeki değerler tarafımdan (ve arayüz için verici etiket projesiyle birlikte) belirlenecektir. Yapay zekâ kullanımı her aşamada bu bölüme ve proje günlüğüne eklenecektir.
