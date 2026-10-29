# PixelOS v2 — Ringkasan Perubahan

## Bug yang diperbaiki
| # | Masalah lama | Perbaikan |
|---|---|---|
| 1 | Bass hampir tak terlihat: stereo A2DP dibaca sbg mono (bin ~689 Hz), `vReal[0]` (100-200 Hz) tak pernah digambar | Downmix L+R -> mono, FFT 512 float, 16 band log 80 Hz-10 kHz mulai dari bass |
| 2 | `vReal` ditimpa PCM mentah oleh task BT saat sedang digambar (bar berkedip acak) | Ring buffer; FFT disalin dari sampel terbaru; visualizer hanya baca `spectrum[]` |
| 3 | `display.display()` dobel + I2C ~25 ms memblok loop (~20 FPS + hitch tiap 100 ms) | OLED di task FreeRTOS sendiri; loop dibatasi 50 FPS (`FRAME_MS`) |
| 4 | `FastLED.clear()` tiap frame mematikan Twinkle (cuma 1 titik) & trail Line/Rain | Tiap mode simpan state sendiri; Twinkle/Rain/Line ditulis ulang |
| 5 | Skala tetap 45000: musik normal mentok penuh, lagu pelan kecil | AGC adaptif per band + noise floor + kontras |
| 6 | Bar membeku saat lagu di-pause (A2DP masih tersambung) | Deteksi audio berhenti (250 ms) -> bar meluruh; ikon play/pause ikut sinkron |
| 7 | `String` metadata ditulis task BT & dibaca task OLED (risiko crash) + fragmentasi heap | `char[]` + mutex; OLED tanpa `String`; karakter non-ASCII jadi `?` |
| 8 | Tanpa batas arus LED (risiko brownout); brightness awal 80 vs 50 | `setMaxPowerInVoltsAndMilliamps` (`LED_MAX_MILLIAMPS`); brightness konsisten |

## Perbaikan kecil
- Tombol di-debounce (25 ms); brightness naik/turun berulang saat ditahan.
- Huruf kecil di `T:teks` sekarang tampil (font sebelumnya menjadikannya spasi).
- `mck_io_num = I2S_PIN_NO_CHANGE` (cegah clock keluar di GPIO0).
- Pilihan channel mic otomatis (RIGHT/LEFT) kalau data selalu nol.
- BLE: `WRITE_NR` diterima; buffer perintah 128 byte.
- Matrix OFF tidak lagi `show()` tiap frame.
- Tombol lagu diabaikan saat A2DP tidak tersambung.

## Fitur baru
- `analyzer.cpp`: band log, AGC, smoothing attack/release, `bassEnergy`, `trebleEnergy`, `beatPulse` (deteksi kick).
- Semua mode memakai analyzer; Circle/Plasma/Lava/Rain/Twinkle/Pulse bereaksi ke beat.
- Animasi ambient halus saat hening (>2,5 dtk), memudar otomatis saat musik mulai.
- Gerakan berbasis waktu (kecepatan sama berapapun FPS).
- Setting tersimpan di NVS (brightness, mode, warna per-mode, teks): `settings.cpp`.
- Indikator "MIC" di OLED saat sumber visualizer = mikrofon.

## Yang perlu kamu sesuaikan
- `config.h` -> `LED_MAX_MILLIAMPS` (default 2500) sesuai power supply 5V. Frame berat akan diredupkan otomatis.
- `config.h` -> `SAMPLES` (512). Kalau terasa berat, coba 256 (bass kurang detail).
- `analyzer.cpp` -> konstanta di bagian atas (`NOISE_FLOOR`, `REF_MIN`, `ATTACK_TAU`, `RELEASE_TAU`, `CONTRAST`) untuk selera "kerasnya" visual.

## Verifikasi
Kode diuji di host dengan stub + AddressSanitizer/UBSan (analyzer, ring buffer, mic, tombol, NVS, 13 mode x 7 warna).
Belum diuji di hardware ESP32 asli.
