# GPC - VS Code Copilot Instructions

Bu dosya, VS Code içindeki ajan/yardımcı akışları için proje bağlamını kısa ve güncel tutar.

## Proje Özeti
- Proje: GPC (GPS Console)
- Stack: C/C++, SDL2, OpenGL, cimgui/ImGui
- Ana giriş: `src/gps_gui.c`
- Modüller: `src/include/*.h|*.c`

## Build & Run
- Build: `make clean && make`
- Run: `./gps_gui`

## Harita Altyapısı (Güncel)
- Tile math: `gps_tiles.h/c`
- Tile loader (BMP): `gps_tile_loader.h/c`
- MBTiles reader: `gps_mbtiles.h/c`
- POI scaffold: `gps_poi.h/c`
- Tile source controls: `Offline Only`, `Prefer MBTiles`

## Kodlama Notları
- `gps_gui.c` orchestration katmanı olarak tutulmalı; iş mantığı mümkünse modüllere taşınmalı.
- Yeni eklemelerde mevcut C-style/snake_case korunmalı.
- Kullanıcıya dönük değişikliklerde `README.md` ve teknik kararlar için `NOTE.md` güncellenmeli.

## Test Notları
- Hızlı doğrulama: build + açılış test
- Donanım testleri: GPS bağlantı, NMEA akışı, Satellite/Sky Plot/Map panel kontrolleri
