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
- **Modüler Mimari**: Compass, Console, Map, Polar view ayrı modüller halinde
- **Connection Management**: GPS cihaz bağlantı dialoğu ve otomatik bağlantı sistemi

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
- **Gelişmiş Harita Görünümü**: Zoom, pan, track görüntüleme ile interactive harita
- **Track Kayıt Sistemi**: GPS rotanızı kaydedin ve GPX formatında export edin
- **Sky Plot**: Uyduların gök kubbesindeki konumunu polar koordinat sisteminde görün
- **Digital Compass**: GPS yön bilgisi ile otomatik pusula ve magnetic declination
- **Raw Data Console**: NMEA komut gönderme ve ham veri izleme
- **Connection Dialog**: GPS cihaz seçimi, baud rate ayarlama ve auto-connect
- **Data Management**: Organize dosya sistemi ile data/ klasöründe veri saklama
- **Veri Kayıt**: GPS verilerinin dosyaya kaydedilmesi
- **Tab-based Interface**: Telemetry, Map, Satellites, Sky Plot, Compass, Raw Data sekmeleri
- **Çoklu Format Desteği**: Ham NMEA ve işlenmiş veri görüntüleme
- **Modüler Mimari**: Her component ayrı modül olarak geliştirilmiş
- **GPS Device Management**: Otomatik port tespit, manuel bağlantı ve cihaz kontrolü

### Yeni Özellikler (v2.0)

#### 🗺️ **Gelişmiş Harita Sistemi**

- **Zoom Controls**: Mouse wheel ile zoom, butonlarla zoom in/out/reset
- **Pan Destegi**: Orta fare tuşu ile harita kaydırma
- **Track Görüntüleme**: GPS rotanızın real-time çizimi
- **Auto-center**: GPS pozisyonunu otomatik takip
- **Grid System**: Referans çizgileri ile konum belirleme

#### 📊 **Track Yönetimi**

- **Recording Controls**: Start/Stop track recording
- **Track Statistics**: Mesafe, süre, maksimum hız bilgileri
- **Visual Track**: Cyan renkte rota çizgisi
- **Direction Arrow**: Hareket yönü göstergesi
- **Fit to Track**: Tüm rotayı ekrana sığdır

#### 💾 **GPX Export**

- **Standard GPX**: Industry standard GPX 1.1 format
- **Track Points**: Timestamp, elevation, coordinates
- **Waypoints**: Future expansion için hazır
- **Metadata**: Track bilgileri ve oluşturma zamanı

#### 🛰️ **Sky Plot (Uydu Diyagramı)**

- **Polar View**: Uyduların gök kubbesindeki gerçek konumları
- **SNR Color Coding**: Sinyal gücüne göre renk kodlaması
- **Interactive**: Uydu seçme ve detay görüntüleme
- **Fix Indicator**: Pozisyon hesaplamasında kullanılan uydular
- **Compass Labels**: Kuzey, Doğu, Güney, Batı işaretleri

#### 🧭 **Compass & Direction (Pusula Sistemi)**

- **Digital Compass**: Dairesel pusula tasarımı polar view benzeri
- **GPS Heading**: Gerçek zamanlı yön göstergesi
- **Magnetic Declination**: Manuel manyetik sapma ayarı
- **Auto-rotate**: GPS course ile otomatik pusula dönüşü
- **Cardinal Directions**: N, NE, E, SE, S, SW, W, NW işaretleri
- **Speed & Course Display**: Anlık hız ve yön bilgileri

#### 📡 **Raw Data Console (Ham Veri Konsolu)**

- **NMEA Monitor**: Son 5 NMEA cümlesinin real-time görüntülenmesi
- **Color Coding**: Mesaj tipine göre renk kodlaması (RMC, GGA, GSV, vb.)
- **Command Interface**: GPS cihazına direkt komut gönderme
- **MTK Support**: MediaTek GPS çipleri için özel komutlar
- **Auto-scroll**: Otomatik kaydırma ile sürekli veri akışı
- **Configuration**: Update rate, sentence types, restart komutları

#### 🔌 **GPS Connection Management (Bağlantı Yönetimi)**

- **Connection Dialog**: Kullanıcı dostu GPS cihaz bağlantı dialoğu
- **Automatic Port Detection**: macOS ve Linux için USB GPS cihaz tespiti
- **Port Selection**: Radio button ile kolay port seçimi
- **Baud Rate Configuration**: 4800-115200 arası 6 farklı baud rate
- **Auto-connect Feature**: Startup'ta otomatik GPS cihaz bağlantısı
- **Connection Status**: Real-time bağlantı durumu ve hata gösterimi
- **Manual Disconnect**: Menu'den temiz bağlantı koparma
- **Error Handling**: Bağlantı hatalarının kullanıcı dostu gösterimi

### 🔧 **v3.1 Güncellemeleri - Connection Management**

#### **GPS Connection Dialog Implementation**

- **Professional UI**: ImGui tabanlı connection dialog
- **Port Discovery**: Real-time USB GPS cihaz tespiti
- **User Control**: Manuel port seçimi ve baud rate ayarlama
- **Smart Interface**: Sadece uygun durumlarda aktif butonlar

#### **Auto-connect System**

- **Menu Integration**: Connection menüsünde toggle control
- **Startup Behavior**: Uygulama açılışında otomatik bağlantı
- **Immediate Action**: Menu'den etkinleştirince anlık bağlantı denemesi
- **First Available**: İlk uygun GPS cihazına otomatik bağlan

#### **Connection Menu Overhaul**

```plaintext
Connection Menu:
├── Connect...     → Opens GPS Connection Dialog ✅
├── Disconnect     → Clean disconnect (already working) ✅  
└── Auto-connect   → Toggle auto-connect behavior ✅
```

#### **Enhanced Error Handling**

- **Connection Failures**: Kullanıcı dostu error messages
- **Port Issues**: "No GPS devices found" uyarıları
- **Status Tracking**: Status bar'da real-time connection info
- **Clean State**: Disconnect'te tüm connection state temizliği

### 🔧 **v3.0 Güncellemeleri - Modüler Refactoring**

#### **Separation of Concerns Implementation**

- **Modüler Mimari**: Her component artık ayrı dosyada ve kendi sorumluluğuna odaklanıyor
- **Maintainability**: Bug fixing ve feature additions artık çok daha kolay

#### **Yeni Modüller**

```plaintext
📁 src/include/
├── 🆕 gps_compass.h/c    # Digital compass logic & calculations
├── 🆕 gps_console.h/c    # Raw NMEA data management & buffering
├── 🔧 gps_serial.h/c     # Enhanced with command sending capability
├── 📊 gps_data.h/c       # GPS data structures & parsing (unchanged)
├── 🗺️ gps_map.h/c        # Map system & tracking (unchanged)
└── 🛰️ gps_polar.h/c     # Satellite sky plot (unchanged)
```

#### **GPS Serial Enhancement**

- **Real Command Sending**: `gps_serial_send_command()` ile gerçek GPS cihaz kontrolü
- **MTK Protocol Support**: MediaTek GPS modülleri için tam komut desteği
- **Error Handling**: Komut gönderme hatalarının yakalanması ve raporlanması
- **Termination Handling**: NMEA komutlarına otomatik `\r\n` ekleme

#### **Compass Module Features**

```c
typedef struct {
    float heading;           // Current heading (0-360°)
    float declination;       // Magnetic declination (-180° to +180°)
    bool auto_rotate;        // Auto-rotate with GPS course
} compass_t;
```

- **True Heading Calculation**: GPS heading + magnetic declination
- **Auto-update**: GPS motion data ile otomatik pusula güncelleme
- **Manual Declination**: Kullanıcı tarafından ayarlanabilir magnetic declination
- **Future-ready**: WMM (World Magnetic Model) implementation'ı için hazır

#### **Console Module Features**

```c
typedef struct {
    char buffer[5][256];     // Circular buffer for NMEA lines
    int current_index;       // Current write position
    bool auto_scroll;        // Auto-scroll behavior
} console_t;
```

- **Circular Buffer**: Efficient memory usage ile son 5 NMEA cümlesini saklar
- **Color-coded Display**: NMEA sentence type'a göre renk kodlaması
- **Command History**: Gönderilen komutların console'da görüntülenmesi
- **Thread-safe Design**: Future multi-threading için hazır yapı

### Kontroller

- **ESC/Q**: Uygulamadan çıkış
- **Mouse**: ImGui arayüzü ile etkileşim
- **Keyboard**: Standart GUI kontrolleri

#### GPS Connection

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

### GPS Komut Desteği

- **MediaTek (MTK) Commands**: PMTK protokolü ile GPS cihaz konfigürasyonu
- **Firmware Query**: Cihaz versiyonu sorgulama
- **Update Rate Control**: NMEA mesaj sıklığı ayarlama
- **Sentence Filtering**: Sadece gerekli NMEA mesajlarını alma
- **Cold/Warm/Hot Restart**: GPS receiver yeniden başlatma

## 9) Kod Mimarisi

### Modüler Yapı

```plaintext
src/include/
├── gps_data.h/c        # GPS data structures & parsing
├── gps_serial.h/c      # Serial communication + command sending  
├── gps_map.h/c         # Map system & tracking
├── gps_polar.h/c       # Satellite sky plot
├── gps_compass.h/c     # Digital compass & direction
├── gps_console.h/c     # Raw NMEA data console
└── minmea.h/c          # NMEA parsing library
```

### Data Directory Structure

```plaintext
data/
├── gps_log_YYYYMMDD_HHMMSS.nmea  # NMEA log files
├── gps_track.gpx                  # GPX track exports
└── gps_log.txt                    # Legacy log file
```

### Component Sorumluluları

- **GPS GUI** (`gps_gui.c`): UI rendering ve event handling
- **GPS Serial** (`gps_serial.c`): Serial communication ve GPS command interface
- **GPS Compass** (`gps_compass.c`): Digital compass logic ve direction calculations
- **GPS Console** (`gps_console.c`): Raw NMEA data management ve command interface
- **GPS Map** (`gps_map.c`): Interactive map rendering ve track management
- **GPS Polar** (`gps_polar.c`): Satellite sky plot ve polar coordinate display

## 10) Planlanan Özellikler

Bu bölümde belirtilen özellikler uygulanırken [NOTE.md](NOTE.md) dosyasındaki tasarım notlarına dikkat edilmiştir. Temel özellikler tamamlanmıştır:

### ✅ Tamamlanan Özellikler

- [x] GPS port otomatik tespit
- [x] Gerçek zamanlı GPS veri görüntüleme (Telemetry panel)
- [x] Sinyal kalitesi ve uydu bilgileri (Satellite panel)
- [x] Modern GUI arayüzü (Dear ImGui tabanlı)
- [x] NMEA veri logging sistemi
- [x] Renk kodlu uydu sinyal gösterimi
- [x] Real-time harita pozisyon görünümü (basit)
- [x] Durum çubuğu ve bağlantı yönetimi
- [x] **Gelişmiş harita görünümü** (zoom, pan, mouse wheel) ✅
- [x] **Track kayıt sistemi** ve geçmiş rota görüntüleme ✅
- [x] **GPX dosya export** işlevselliği ✅  
- [x] **Uydu Sky Plot** - polar koordinat sistemi ✅
- [x] **Tab-based arayüz** (Telemetry/Map/Satellites/Sky Plot) ✅
- [x] **Interactive controls** - zoom, pan, track recording ✅
- [x] **Pusula ve yön göstergesi** - dairesel pusula tasarımı ile GPS heading ✅
- [x] **GPS cihaz konfigürasyonu ve raw veri konsolu** - NMEA komut gönderme ve ham veri izleme ✅
- [x] **GPS Connection Dialog** - Port seçimi, baud rate ve auto-connect ✅

### 🔄 Kısa Vadeli (Geliştirme Aşamasında)

- [x] Gelişmiş harita görünümü (zoom, pan) ✅
- [x] Track çizgisi ve geçmiş rota gösterimi ✅
- [x] GPX export işlevselliği ✅
- [x] Uydu pozisyon diyagramı (polar view) ✅
- [x] Pusula ve yön göstergesici(tasarım polar view ile benzer olmalı dairesel ve pusula şeklinde) ilave TAB compass ✅
- [x] GPS cihaz konfigürasyon seçenekleri ve ham veri izleme konsolu max beş satır görüntüleme ayrıca en altta seri cihaza komut gönderme.ilave TAB raw data ✅
- [x] GPS Connection Dialog ve Auto-connect sistemi ✅
- [ ] Kullanıcı arayüzü iyileştirmeleri ve kontroller

### Orta Vadeli (Sıradaki Geliştirmeler)

- [ ] Gelişmiş waypoint yönetimi ve navigation
- [ ] Route planning özellikleri
- [ ] Harita tile sistemi ve offline maps
- [ ] Track analiz araçları (elevation profil, speed chart)
- [ ] Çoklu GPS cihaz desteği
- [ ] Gelişmiş veri filtreleme ve smoothing

### Uzun Vadeli

- [ ] Web tabanlı uzaktan monitoring
- [ ] Harita tile caching sistemi
- [ ] Plugin mimarisi
- [ ] Multi-language desteği

## 11) Katkı Süreci

1. Fork yapın ve yeni branch oluşturun: `git checkout -b feature/yeni-ozellik`
2. Değişikliklerinizi commit edin: `git commit -am 'Yeni özellik: açıklama'`
3. Branch'i push edin: `git push origin feature/yeni-ozellik`
4. Pull Request oluşturun

### Kod Standartları

- C99/C++11 standartlarına uyum
- Modüler mimari: Her component ayrı dosyada
- Açıklayıcı değişken isimleri
- Fonksiyon başına maksimum 50 satır
- Her commit bir özellik veya düzeltme
- Header files için include guards

## 12) Lisans

Bu proje şu an için lisans belirtilmemiştir. Üretime geçmeden önce uygun bir açık kaynak lisansı (MIT, GPL v3 gibi) eklenmesi planlanmaktadır.

## 13) İletişim ve Destek

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
