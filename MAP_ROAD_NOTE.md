# 🗺️ GPC Map Panel Yol Haritası

Bu belge, GPC projesindeki **Map (Harita) penceresi** için mimari, geliştirme adımları ve önerilen teknikleri özetler.

---

## 1. Amaç ve Fonksiyonlar

- Gerçek zamanlı GPS konumunu harita üzerinde gösterme
- Track kaydı ve geçmiş rota görselleştirme
- Waypoint ekleme ve yönetimi
- Harita üzerinde zoom, pan ve fit-to-track özellikleri
- Offline/online tile desteği ile harita görselleştirme

---

## 2. Mimari ve Modüller

- **gps_map.c/h**: Harita işlemleri, tile yükleme, track/waypoint çizimi
- **data/map_tiles/**: Tile cache (offline harita görselleri)
- **data/map_db.sqlite**: Vektör objeler (yol, POI, vs.)
- **gps_data.c/h**: GPS konum ve track verisi

---

## 3. Geliştirme Adımları

### 3.1. Tile Tabanlı Harita

- OSM veya MBTiles formatında tile’ları indir ve cache’le
- Tile loader fonksiyonu ile harita arka planını oluştur
- Zoom/pan desteği ekle

### 3.2. Track ve Waypoint

- GPS track noktalarını harita üzerinde çizgi ile göster
- Waypoint ekleme: Haritada tıklama veya menüden ekleme
- Marker/ikon ile track ve waypoint’leri vurgula

### 3.3. Konum Odaklı Görselleştirme

- Son GPS konumunu harita merkezine al
- Otomatik ve manuel pan/zoom desteği

### 3.4. Offline/Online Destek

- Tile’lar yoksa online olarak indir, sonrasında cache’e kaydet
- Offline modda tile’ları `data/map_tiles/` klasöründen yükle

### 3.5. UI ve Kullanıcı Deneyimi

- Sağ tık ile waypoint ekleme/context menu
- Fit to track, zoom to point butonları
- Track/waypoint detaylarını gösteren info panel

---

## 4. Teknik Notlar

- Tile koordinat dönüşümü: GPS lat/lon → tile x/y
- Görsel tile’lar için PNG/JPG desteği
- Vektör objeler için SQLite sorguları
- OpenGL ile tile ve overlay çizimi

---

## 5. Gelecek Geliştirmeler

- Rota planlama ve analiz araçları
- Harita üzerinde POI arama
- Multi-GPS cihaz desteği
- Web tabanlı harita entegrasyonu

---

**Hazırlayan:** GPC Project
**Güncelleme:** 2025-10-19
