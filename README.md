# GPC — GPS Parser ve GUI Konsol

USB porttan bağlanan GPS cihazlarının verilerinin görüntülenmesi ve takibi için hazırlanmış C tabanlı GUI uygulaması. NMEA 0183 protokolü ile çalışır ve SDL2 + OpenGL tabanlı modern arayüz sunar.

## 1) Amaç ve Kapsam

- USB porttan GPS cihazlarının NMEA 0183 verilerini okumak ve gerçek zamanlı görüntülemek
- Modern GUI arayüzü ile GPS verileri, konum bilgileri ve sinyal durumunu izlemek
- GPS verileri üzerinde gelişmiş analiz ve görselleştirme özellikleri sunmak
- GPX formatında veri dışa aktarımı ve track kaydı özelliklerini sağlamak

## 2) Ana Bileşenler

- **GPS GUI Uygulaması**: SDL2 + OpenGL tabanlı modern arayüz ile gerçek zamanlı GPS veri görüntüleme
- **NMEA Parser**: Güvenilir minmea tabanlı GPS veri ayrıştırma motoru
- **Grafik Arayüz**: Dear ImGui tabanlı kullanıcı dostu kontrol paneli
- **Veri Kayıt Sistemi**: Ham NMEA verileri ve işlenmiş GPS bilgilerinin kaydı

## 3) Gereksinimler

- macOS veya Linux
- C/C++ derleyici: clang, gcc veya g++
- SDL2 kütüphanesi
- OpenGL desteği
- CMake (isteğe bağlı)

## 4) Kurulum ve Derleme

### Bağımlılıkların Kurulumu

**macOS için:**
```bash
# SDL2 kurulumu
brew install sdl2

# Xcode Command Line Tools (gerekiyorsa)
xcode-select --install
```

**Linux (Ubuntu/Debian) için:**
```bash
sudo apt-get update
sudo apt-get install libsdl2-dev libgl1-mesa-dev
```

### Derleme

Make kullanarak (önerilen):

```bash
make
```

Manuel derleme:

```bash
g++ -Wall -Wextra -std=c++11 \
    -I src/cimgui -I src/cimgui/imgui -I src/cimgui/imgui/backends -I src/include \
    src/gps_gui.c src/include/minmea.c \
    src/cimgui/cimgui.cpp src/cimgui/cimgui_impl.cpp \
    src/cimgui/imgui/imgui.cpp src/cimgui/imgui/imgui_draw.cpp \
    src/cimgui/imgui/imgui_tables.cpp src/cimgui/imgui/imgui_widgets.cpp \
    src/cimgui/imgui/imgui_demo.cpp \
    src/cimgui/imgui/backends/imgui_impl_sdl2.cpp \
    src/cimgui/imgui/backends/imgui_impl_opengl3.cpp \
    `sdl2-config --cflags --libs` -framework OpenGL -o gps_gui
```

Çıktı: `gps_gui` ikilisi kök dizinde oluşur.

## 5) Çalıştırma

GUI uygulamasını başlatmak için:

```bash
./gps_gui
```

### Desteklenen Özellikler

- **Gerçek Zamanlı GPS Monitoring**: Konum, hız, yön bilgilerinin canlı takibi
- **Sinyal Kalitesi Görüntüleme**: GPS sinyal gücü ve uydu sayısı monitörü
- **Grafik Arayüz**: Dear ImGui tabanlı modern kontrol paneli
- **Veri Kayıt**: GPS verilerinin dosyaya kaydedilmesi
- **Çoklu Format Desteği**: Ham NMEA ve işlenmiş veri görüntüleme

### Kontroller

- **ESC/Q**: Uygulamadan çıkış
- **Mouse**: ImGui arayüzü ile etkileşim
- **Keyboard**: Standart GUI kontrolleri

## 6) Geliştirme ve Test

### Debug Modda Çalıştırma

```bash
# Debug sembollerle derleme
make clean && make CFLAGS="-g -O0"

# Detaylı çıktı ile çalıştırma  
./gps_gui --verbose
```

### GPS Simülasyonu

Test amaçlı olarak GPS simulatör verileri kullanılabilir:

```bash
# Örnek NMEA verisi ile test
echo '$GPRMC,183730,A,3907.482,N,12102.436,W,000.0,360.0,120598,015.5,E*67' > test_data.nmea
./gps_gui --file test_data.nmea
```

## 7) Sorun Giderme

### macOS'ta Yaygın Sorunlar

- **SDL2 bulunamıyor**: `brew install sdl2` ile SDL2'yi kurun
- **OpenGL hatası**: Xcode Command Line Tools'un kurulu olduğundan emin olun
- **Port erişim sorunu**: GPS cihazının doğru porta bağlı olduğunu kontrol edin (`ls /dev/tty.usb*`)

### Linux'ta Yaygın Sorunlar

- **SDL2 dev kütüphanesi eksik**: `sudo apt-get install libsdl2-dev`
- **OpenGL dev kütüphanesi eksik**: `sudo apt-get install libgl1-mesa-dev`
- **İzin sorunu**: Kullanıcıyı `dialout` grubuna ekleyin: `sudo usermod -a -G dialout $USER`

## 8) NMEA ve GPS Veri Desteği

Desteklenen NMEA 0183 mesaj türleri:
- **RMC**: Önerilen minimum özel GPS veriler
- **GGA**: Global Konumlandırma Sistemi düzeltme verisi
- **GSA**: GPS DOP ve aktif uydu verisi
- **GSV**: GPS Uydu görünür verisi
- **GLL**: Coğrafi konum - Enlem/Boylam
- **VTG**: İz üzerinde yapılan hız ve kurs
- **ZDA**: Zaman ve tarih

## 9) Planlanan Özellikler
Bu bölümde belirtilen özellikler uygulanırken [NOTE.md](NOTE.md) dosyasındaki notlara dikkat ediniz.bu notlar gui uygulamasının geliştirme sürecini yansıtmaktadır.

### Kısa Vadeli
- [ ] GPS port otomatik tespit
- [ ] Gerçek zamanlı harita görünümü
- [ ] Sinyal gücü çubuk grafikleri
- [ ] Basit GPX export

### Orta Vadeli
- [ ] Waypoint işaretleme ve yönetimi
- [ ] Route planning özellikleri
- [ ] Gelişmiş veri filtreleme
- [ ] Çoklu GPS cihaz desteği

### Uzun Vadeli
- [ ] Web tabanlı uzaktan monitoring
- [ ] Harita tile caching sistemi
- [ ] Plugin mimarisi
- [ ] Multi-language desteği

## 10) Katkı Süreci

1. Fork yapın ve yeni branch oluşturun: `git checkout -b feature/yeni-ozellik`
2. Değişikliklerinizi commit edin: `git commit -am 'Yeni özellik: açıklama'`
3. Branch'i push edin: `git push origin feature/yeni-ozellik`
4. Pull Request oluşturun

### Kod Standartları
- C99/C++11 standartlarına uyum
- Açıklayıcı değişken isimleri
- Fonksiyon başına maksimum 50 satır
- Her commit bir özellik veya düzeltme

## 11) Lisans

Bu proje şu an için lisans belirtilmemiştir. Üretime geçmeden önce uygun bir açık kaynak lisansı (MIT, GPL v3 gibi) eklenmesi planlanmaktadır.

## 12) İletişim ve Destek

- Issues için GitHub issue tracker kullanın
- Geliştirme soruları için Pull Request tartışmaları
- GPS protokol sorunları için minmea dokümantasyonunu inceleyin

### Yaygın Mesajlar

- **"Waiting for GPS fix..."**: GPS cihazı bağlı ama sinyal alamıyor, açık alana çıkın
- **"No GPS device found"**: USB GPS cihazını kontrol edin
- **"OpenGL context error"**: GPU sürücüleri ve OpenGL desteğini kontrol edin
- **"SDL2 initialization failed"**: SDL2 kurulumunu ve sistem uyumluluğunu kontrol edin

---

**Not**: Bu proje aktif geliştirme aşamasındadır. Üretim kullanımı için henüz hazır değildir.