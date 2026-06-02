# GPC Yardım Dosyası

Bu belge, **GPC — GPS Console** için güncel kullanıcı rehberidir.

---

## 1) Hızlı Başlangıç

1. Uygulamayı başlatın.
2. **Menu → Connection → Connect...** ile GPS portunu seçin.
3. Gerekirse baud rate ayarlayın (varsayılan 9600).
4. Bağlandıktan sonra Telemetry/Map/Satellites panellerini izleyin.
5. Track kaydı için **Menu → Tools → Start Recording** kullanın.

> İpucu: Connection dialog içinde son kullanılan **port/baud** mini geçmişi bulunur.

---

## 2) Panel Özeti

- **Telemetry**: Konum, hız, fix kalitesi, istatistik ve log durumu.
- **Map**: Zoom/pan, track çizimi, waypoint, POI overlay, tile kaynak yönetimi.
- **Satellites**: Uydu listesi, sinyal ve kullanım bilgileri.
- **Sky Plot**: Uyduların polar görünümü.
- **Compass**: Yön ve course bazlı pusula görünümü.
- **Raw Data Console**: Ham NMEA akışı ve komut gönderimi.
- **Analytics**: Track özeti + Speed vs Time ve Altitude vs Distance grafikleri.
	- Hover ile cursor readout: anlık eksen değerleri gösterilir.
	- Zoom/pan için fare etkileşimi + plot reset butonları bulunur.

---

## 3) Menü Özeti

### Connection

- **Connect...**: Port/baud seçimi ile bağlantı.
- **Disconnect**: Bağlantıyı kapatır.
- **Auto-connect**: Uygun cihazı otomatik bağlamayı aç/kapat.

### Tools

- **Start/Stop Recording**: Track kaydı kontrolü.
- **Clear Track**: Kaydedilen izi temizler.
- **Export GPX...**: Track verisini GPX olarak dışa aktarır.
- **Import GPX...**: GPX dosyasını belleğe yükler ve analiz panelinde kullanır.
- **Export CSV (Analysis)...**: Track ve analiz verisini CSV olarak dışa aktarır.
- **Start/Stop Logging**: Ham GPS log kaydı.

### Help

- **Help**: Bu yardım dokümanını gösterir.
- **About**: Proje ve teknik bilgiler.
- **Keyboard Shortcuts (Tips)**: Kısayol/ipucu menüsü.

### View

- **Dark Theme / Light Theme**: Tema geçişi ve kalıcılığı.

---

## 4) Kısayollar / Shortcuts (TR + EN)

- **Cmd/Ctrl+K**: Bağlantı penceresi ipucu (Connection dialog tip)
- **F1**: Yardım penceresi (Open help)
- **Mouse Wheel**: Harita zoom in/out
- **Middle Mouse + Drag**: Harita kaydırma (Pan)
- **Right Click (Map)**: Waypoint/harita etkileşimi
- **Enter (Console)**: Komut gönder (Send command)

---

## 5) Harita ve Tile Davranışı

- Offline tile zinciri: **Disk BMP → MBTiles → Online fetch kuyruğu**
- **Offline Only** açıkken online deneme yapılmaz.
- **Prefer MBTiles** açıkken MBTiles öncelikli okunur.
- Online kuyruk arka plan worker thread ile çalışır (UI bloklama azaltılır).
- Kuyruk işleyici, viewport merkezine yakın tile isteklerini önce işler.
- Aging/fairness ile uzun süre bekleyen uzak tile istekleri de zamanla öne çekilir.
- Retry backoff jitter ile başarısız tile denemeleri zamana yayılır, ani yük azalır.
- Ardışık download hatalarında worker kısa adaptif cooldown uygular.
- Worker pacing, kuyruk yükü ve fail/cooldown durumuna göre dinamik ayarlanır.
- Başarılı fetch sonrası completion kuyruğu ile cache invalidation yapılır;
	tile daha hızlı yeniden yüklenir.
- Uygulama ana döngüsü idle durumda düşük FPS'e geçer (CPU yükünü azaltır), aktif kullanımda yüksek FPS'e döner.
- Seri okuma/parsing arka planda işlendiği için yoğun veri akışında UI daha akıcı kalır.
- Analytics paneli, kayıt geldikçe ImPlot ile grafikleri otomatik günceller.

### PNG/BMP Decode Notu

- PNG tile indirildiğinde BMP dönüşümü mümkünse BMP saklanır.
- Dönüşüm mümkün değilse PNG cache saklama da başarı kabul edilir.

---

## 6) POI (Points of Interest)

- **Show POI**: Marker görünürlüğü.
- **POI Filter**: İsme göre case-insensitive filtre.
- Marker etkileşimi:
	- Hover: hızlı bilgi
	- Sol tık: haritayı merkeze al
	- Sağ tık: detay popup

---

## 7) Sorun Giderme

### Bağlantı

- "No GPS device found": Portları yeniden tarayın, cihaz/USB kabloyu kontrol edin.
- Bağlı ama veri yok: Doğru baud rate seçimini doğrulayın.

### Harita / Tile

- Tile geç geliyorsa birkaç frame bekleyin; online queue arka planda işler.
- Sürekli boş tile görünüyorsa internet erişimi ve `curl` araç varlığını kontrol edin.
- Analytics grafikleri boş görünüyorsa en az iki geçerli track noktası kaydedildiğini doğrulayın.

### Derleme

- C dosyalarının C++ olarak derlenmesine dair deprecation uyarıları şu an bilinen durumdur.

---

## 8) Dosya Konumları

- GPS logları: `data/gps_log_YYYYMMDD_HHMMSS.nmea`
- GPX çıktısı: `data/gps_track.gpx`
- GPX import varsayılanı: `data/gps_track.gpx`
- CSV analiz çıktısı: `data/gps_track.csv`
- Offline tile klasörü: `data/map_tiles/`
- MBTiles dosyası: `data/map_tiles.mbtiles`
- Kullanıcı tercihleri: `data/gpc_config.ini`

