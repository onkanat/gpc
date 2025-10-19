# 🗺️ GPC Project Roadmap

Bu belge, **GPC — GPS Parser ve GUI Console** projesinin geliştirme yol haritasını (v3.2 → v3.4) tanımlar.  
Hedef: yüksek performanslı, kullanıcı dostu ve genişletilebilir bir GPS GUI platformu.

---

## 🚀 v3.2 — Stabilizasyon ve Performans İyileştirmesi (0–2 Hafta)

### 🎯 Hedefler
- Uygulamanın stabil hale getirilmesi
- Render döngüsünün optimize edilmesi
- Kullanıcı konfigürasyonlarının kalıcı hale getirilmesi

### 🔧 Teknik İyileştirmeler
- [ ] **Render Optimizations** — Harita ve SkyPlot çizimleri “dirty flag” mantığıyla güncellensin.
- [ ] **Threaded NMEA Parsing** — Parsing işlemi arka planda yapılacak.
- [ ] **Config System (INI/JSON)** — Kullanıcı tercihleri (tema, baud rate, layout) `gps_config` modülüne taşınacak.
- [ ] **Ring Buffer for Logging** — Track kaydı ve NMEA logları bellek dostu yapı ile optimize edilecek.
- [ ] **Dynamic FPS Control** — İdling durumunda düşük FPS ile CPU yükü azaltılacak.

### 💡 Kullanıcı Deneyimi
- [ ] Tema geçişi (Dark/Light) — ImGui style profile sistemi
- [ ] Status bar’da dinamik renk kodlaması
- [ ] Compass panelinde hız + yön birleşik gösterim
- [ ] Raw console’da siyah arka plan ve monospace font
- [ ] Docking layout save/load desteği

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
- [ ] Speed vs. Time, Altitude vs. Distance grafikleri (ImPlot ile)
- [ ] Track Summary Panel — toplam mesafe, ortalama hız, süre
- [ ] GPX dosyalarından veri yükleme & analiz
- [ ] Export to CSV / JSON seçenekleri

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
- [ ] **Offline Map Tiles** — MBTiles veya OSM tile cache sistemi
- [ ] **Route Planning** — Manuel waypoint ekleme ve yön hesaplama
- [ ] **Fit to Track & Zoom to Point** özellikleri
- [ ] **Mouse Context Menu** — Harita üzerinde sağ tıklama eylemleri

### 📡 GPS Veri Geliştirmeleri
- [ ] DOP, HDOP analizi (GSA mesajlarından)
- [ ] Kalman/MAVG tabanlı smoothing filtreleri
- [ ] Smart Logging — yalnızca hareket değiştiğinde kayıt
- [ ] Multi-device desteği (aynı anda birden fazla GPS kaynağı)

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
**Güncelleme:** 2025-10-19  
**Durum:** Aktif geliştirme (v3.2 hazırlık aşaması)
