# GPC Yardım Dosyası

Bu belge, **GPC — GPS Parser ve GUI Console** uygulamasının kullanıcı rehberidir. Aşağıda, temel özellikler, arayüz bileşenleri ve kontroller hakkında bilgiler bulabilirsiniz.

---

## Temel Özellikler

- **Gerçek Zamanlı GPS Verisi**: NMEA 0183 protokolü üzerinden GPS verilerini okuyabilir.
- **GUI Arayüzü**: SDL2 + OpenGL + Dear ImGui tabanlı kullanıcı dostu arayüz.
- **Harita Görselleştirme**: OpenStreetMap tabanlı harita görüntüleme ve izleme. **Yakında eklenecek**
- **Uydu Görselleştirme**: GPS uydularının konumlarını ve durumlarını gösterme.
- **Telemetry Paneli**: Hız, yükseklik, koordinatlar ve diğer GPS verilerini gösterme.
- **NMEA Konsolu**: Gelen ham NMEA verilerini görüntüleme ve GPS cihazına komut gönderme.
- **Logging**: GPS verilerini GPX formatında kaydetme.
- **Tema Desteği**: Koyu ve açık tema seçenekleri. **Yakında eklenecek**
- **Yapılandırma Dosyası**: Kullanıcı tercihlerini INI/JSON dosyasında saklama. **Yakında eklenecek**

---

## Arayüz Bileşenleri

### Menü Çubuğu **(Header Bar)**

Uygulamanın üst kısmında yer alır. **Yakında eklenecek**

- **Logo / Başlık**: Sol üst köşede "GPC – GPS Console" başlığı.
- **Sekmeler**: Telemetry, Satellites, Map, Sky Plot, Compass, Connection, Tools, Settings sekmeleri.
- **Simgeler**: Tema değiştirme, ayarlar ve bağlantı durumu simgeleri.

### Kontroller

- **ESC/Q**: Uygulamadan çıkış
- **Mouse**: ImGui arayüzü ile etkileşim
- **Keyboard**: Standart GUI kontrolleri

#### GPS Connection (Connection Tab)

- **Menu → Connection → Connect...**: GPS Connection Dialog açma
- **Menu → Connection → Disconnect**: GPS bağlantısını koparma  
- **Menu → Connection → Auto-connect**: Otomatik bağlantıyı toggle etme

#### Map Controls

- **Mouse Wheel**: Zoom in/out
- **Middle Mouse + Drag**: Map panning
- **Left Click**: Map interaction (future features)

#### Raw Data Console

- **Auto-scroll Checkbox**: Yeni NMEA verisi geldiğinde otomatik scroll
- **Clear Console Button**: Console buffer'ını temizleme
- **Command Input**: GPS cihazına NMEA komut gönderme
- **Predefined Buttons**: Firmware query, GPS restart vb.

## Compass Panel

- **3D Compass**: Yönü gösteren 3D grafik. **Çok sonra eklenecek**
- **Heading Ring**: Mevcut yönü gösterir.
- **Speed Indicator**: Hız göstergesi.
- **Magnetic Declination**: Manyetik sapma ayarı.
- **Course Over Ground**: Harita ile entegre yön vektörü. **Çok sonra eklenecek**
- **Altitude Indicator**: Yükseklik göstergesi. **Yakında eklenecek**
