# GPC — GPS Console

---

## Hakkında

**GPC** (GPS Console), NMEA 0183 protokolü üzerinden GPS verilerini okuyabilen ve görselleştirebilen açık kaynak bir uygulamasıdır.

### Özellikler

- ✅ **Gerçek Zamanlı GPS Verisi**: NMEA 0183 protokolü desteği
- ✅ **Modern GUI**: SDL2 + OpenGL + Dear ImGui tabanlı arayüz
- ✅ **Uydu Görselleştirme**: GPS uydularının konum ve durumları
- ✅ **Telemetri Paneli**: Hız, yükseklik, koordinat bilgileri
- ✅ **Ham Veri Konsolu**: NMEA mesajları ve komut gönderimi
- ✅ **GPS Logging**: Veri kaydetme ve GPX dışa aktarma
- ✅ **Harita Görselleştirme**: Zoom/pan/track + tile cache + MBTiles desteği
- ✅ **Pusula ve Sky Plot**: Gelişmiş navigasyon panelleri
- ✅ **POI Overlay (MVP)**: Marker, filtre ve temel etkileşim
- 🔄 **Online Tile Queue**: Worker thread + retry/backoff + completion/invalidation
- ✅ **Config Kalıcılığı**: Tema/baud/tile tercihleri ve layout ayarları saklanır
- ✅ **Threaded NMEA Parsing**: Serial okuma/parsing UI thread'den ayrıldı
- ✅ **Track Analytics (Summary + Charts)**: Mesafe, süre, ortalama hız ve ImPlot tabanlı grafik paneli

### Teknik Bilgiler

- **Geliştirme Dili**: C/C++
- **GUI Framework**: Dear ImGui
- **Graphics API**: OpenGL 3.3+
- **Platform**: SDL2 (çoklu platform desteği)
- **GPS Protokolü**: NMEA 0183
- **Harita Veri Katmanı**: Disk tile + MBTiles (SQLite)
- **Plot Katmanı**: Vendor ImPlot (`src/third_party/implot`, commit `1351ab2`)
- **Lisans**: MIT + proje içi katkı lisans metni

### Geliştirici

- **Proje**: GPC GPS Console
- **Repository**: [github.com/onkanat/gpc](https://github.com/onkanat/gpc)
- **Durum**: Aktif geliştirme (Alpha)

---

## Açık Kaynak Lisans

```text
THE "USE IT, FIX IT, ADD TO IT" LICENSE

Bu yazılım özgürce kullanılabilir, değiştirilebilir ve 
dağıtılabilir. Herhangi bir hata bulursanız düzeltin,
yeni özellik eklemek istiyorsanız ekleyin ve paylaşın.

Geliştirici hiçbir garanti vermez, ancak katkılarınızı
memnuniyetle karşılar.
```

### Teşekkürler

- **Dear ImGui**: Omar Cornut ve katkıda bulunanlar
- **SDL2**: Sam Lantinga ve SDL Development Team  
- **cimgui**: Sonoro1234 ve katkıda bulunanlar
- **ImPlot**: Evan Pezent, Breno Cunha Queiroz ve katkıda bulunanlar
- **minmea**: Kosma Moczek - NMEA parser library

---
