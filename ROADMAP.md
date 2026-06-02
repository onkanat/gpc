# 🗺️ GPC Project Roadmap

Bu belge, **GPC — GPS Parser ve GUI Console** projesinin geliştirme yol haritasını (v3.2 → v3.5+) tanımlar.  
Hedef: yüksek performanslı, kullanıcı dostu ve genişletilebilir bir GPS GUI platformu.

> Durum notu (2026-05-23): Harita tarafında v3.4 başlıklarının önemli bir kısmı MVP seviyesinde tamamlandı (offline tile pipeline, MBTiles entegrasyonu, waypoint sağ tık akışı, fit/zoom kontrolü).

---

## 🚀 v3.2 — Stabilizasyon ve Performans İyileştirmesi (0–2 Hafta)

### 🎯 Hedefler
- Uygulamanın stabil hale getirilmesi
- Render döngüsünün optimize edilmesi
- Kullanıcı konfigürasyonlarının kalıcı hale getirilmesi

### 🔧 Teknik İyileştirmeler
- [~] **Render Optimizations** — `gps_map.c` ve `gps_polar.c` tarafında dirty-flag/revision temeli eklendi; daha agresif redraw kısıtlaması sonraki iterasyonda.
- [x] **Threaded NMEA Parsing** — Serial okuma/parsing arka plan worker thread + güvenli teslim kuyruğu ile UI thread’den ayrıldı.
- [x] **Config System (INI/JSON)** — `gps_config` modülü eklendi; tema/baud/tile tercihleri ve layout ini yolu kalıcı hale getirildi.
- [ ] **Ring Buffer for Logging** — Track kaydı ve NMEA logları bellek dostu yapı ile optimize edilecek.
- [x] **Dynamic FPS Control** — İdling durumunda düşük FPS ile CPU yükü azaltıldı (aktif ~60 FPS / idle ~15 FPS pacing).

### 💡 Kullanıcı Deneyimi
- [x] Tema geçişi (Dark/Light) — View menüsünden değiştirilebilir ve config ile korunur
- [x] Status bar’da dinamik renk kodlaması
- [ ] Compass panelinde hız + yön birleşik gösterim
- [ ] Raw console’da siyah arka plan ve monospace font
- [x] Docking layout save/load desteği (`imgui.ini` + config path kalıcılığı)
- [x] Menüde keyboard shortcut ipuçları
- [x] Connection dialog’da son kullanılan port/baud mini geçmişi

---

## 📊 v3.3 — Analiz ve Görselleştirme Geliştirmesi (2–6 Hafta)

### 🎯 Hedefler
- Kullanıcıya rota ve hız analizi sunmak
- Compass ve Telemetry panellerini profesyonel hale getirmek

### 🧭 Compass & Telemetry
- [ ] 3D Compass tasarımı (gradient ring + gölge efekti)
- [ ] Speed indicator ring — hız değişimlerine göre renk geçişi
- [ ] Magnetic Declination ayarı ve otomatik hesaplama (WMM tablosu)
- [ ] Course over ground yön vektörü (harita ile entegre)

### 📈 Track Analyzer
- [x] Speed vs. Time, Altitude vs. Distance grafikleri (vendor ImPlot ile)
- [x] Track Summary Panel — temel metrik paneli (mesafe, süre, ort. hız, max hız, irtifa aralığı)
- [x] GPX dosyalarından veri yükleme & analiz (MVP import)
- [~] Export to CSV / JSON seçenekleri (CSV export tamam, JSON beklemede)

### 🧩 UI Refinement
- [ ] Unified icon set (Lucide veya FontAwesome)
- [ ] Menu bar hover efektleri ve alt çizgi animasyonu
- [ ] Ayarlar panelinde kategorik düzen (Connection / Display / Logging)

---

## 🌍 v3.4 — Harita ve Navigasyon Sistemi (6–10 Hafta)

### 🎯 Hedefler
- Offline tile harita sistemi
- Waypoint yönetimi ve rota planlama
- Otonom rota analizi

### 🗺️ Harita Sistemi
- [x] **Offline Map Tiles (MVP)** — Disk BMP + MBTiles (SQLite) tile cache/read
- [ ] **Route Planning** — Manuel waypoint ekleme ve yön hesaplama
- [x] **Fit to Track & Zoom to Point** özellikleri (MVP)
- [x] **Mouse Context Menu** — Harita üzerinde sağ tık ile waypoint ekleme
- [~] **Online Tile Fetch Queue** — Hook + gelişmiş telemetri + dedup/retry queue + fail-cache TTL + araç-yetenek cache + arka plan worker thread + dinamik pacing + adaptif polling + viewport-priority pop + aging/fairness + retry-jitter + adaptive-net-cooldown + completion/invalidation + PNG-cache-success + native move fallback + MVP fetch/cache hazır; optional libpng + macOS native decode mevcut, tam platform bağımsız decode hattı olgunlaştırması beklemede
- [~] **POI/Vector Overlay** — bbox count + marker render + hover/click + isim filtresi + temel detay popup hazır, ileri seviye kategori/arama/panel beklemede

### 📡 GPS Veri Geliştirmeleri
- [ ] DOP, HDOP analizi (GSA mesajlarından)
- [ ] Kalman/MAVG tabanlı smoothing filtreleri
- [ ] Smart Logging — yalnızca hareket değiştiğinde kayıt
- [ ] Multi-device desteği (aynı anda birden fazla GPS kaynağı)

### ✅ v3.4 Erken Tamamlananlar (MVP)

- [x] Web Mercator ekran dönüşümleri
- [x] Slippy tile matematiği modülü
- [x] Tile texture cache + negative cache + telemetri sayaçları
- [x] MBTiles `metadata.scheme` (tms/xyz) uyumu + metadata yoksa fallback probing
- [x] Offline-only ve Prefer-MBTiles UI kontrolleri
- [x] Waypoint listesi (Go/Del) + popup tabanlı ekleme

---

## 🧠 v3.5+ — Geleceğe Yönelik Geliştirmeler (Uzun Vadeli)

- [ ] Web tabanlı izleme (WebSocket + REST API)
- [ ] Remote telemetry dashboard (Flask / FastAPI)
- [ ] Plugin mimarisi (harici modül desteği)
- [ ] Multi-language desteği (gettext / JSON i18n)
- [ ] AI tabanlı anomaly detection (GPS jitter, veri kopması)
- [ ] Otonom navigasyon tahmini (AI rota önerisi)

---

## 🧰 Refactoring ve Kod Kalitesi

| Modül | Eylem | Amaç |
|--------|--------|------|
| `gps_serial.c` | Thread-safe queue & async read/write | UI donmalarını önleme |
| `gps_map.c` | VBO tabanlı OpenGL çizim | GPU performansı ↑ |
| `gps_compass.c` | Trigonometrik hesapların normalize edilmesi | CPU yükü ↓ |
| `gps_console.c` | Dynamic deque (STL) geçişi | Daha esnek buffer |
| `gps_gui.c` | Event dispatching sistemi eklenmesi | Modüller arası bağımsızlık |

---

## 📆 Geliştirme Takvimi

| Sürüm | Odak Alanı | Süre (tahmini) |
|--------|-------------|----------------|
| **v3.2** | Performans, config sistemi, stabilizasyon | 2 hafta |
| **v3.3** | Analiz araçları, Compass, Telemetry iyileştirmesi | 1 ay |
| **v3.4** | Offline harita ve navigasyon | 1.5 ay |
| **v3.5+** | Web/AI entegrasyonu | Sürekli geliştirme |

---

## 📎 Kaynaklar

- [NOTE.md](NOTE.md) – UI layout ve tasarım planı  
- [README.md](README.md) – Genel proje açıklaması  
- [imgui.ini](imgui.ini) – Docking ve pencere konumları  
- [gps_gui.c](src/gps_gui.c) – Ana GUI döngüsü ve giriş noktası

---

**Hazırlayan:** Hakan Kılıçaslan  
**Güncelleme:** 2026-06-02  
**Durum:** Aktif geliştirme (v3.2 iyileştirmeleri + v3.3 analytics chart/UX + GPX import + CSV export + v3.4 MVP tamamlandı, ileri adımlar sürüyor)
