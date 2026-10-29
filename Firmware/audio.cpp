#include "audio.h"
#include "state.h"
#include "analyzer.h"
#include <arduinoFFT.h>
#include <math.h>
#include "driver/i2s.h"

// A2DP/PCM5102A tetap menggunakan I2S default milik ESP32-A2DP.
// INMP441 memakai port I2S kedua supaya tidak bentrok dengan output DAC.
static const i2s_port_t MIC_I2S_PORT = I2S_NUM_1;

// Pin INMP441 untuk ESP32-WROOM-32.
// Hindari GPIO 6-11 karena terhubung ke flash internal ESP32.
#define MIC_I2S_BCLK 18
#define MIC_I2S_WS    5
#define MIC_I2S_DIN   4

#define MIC_CHUNK 256                       // sampel per pembacaan DMA

BluetoothA2DPSink a2dp_sink;

float vReal[SAMPLES];
float vImag[SAMPLES];

static ArduinoFFT<float> FFT(vReal, vImag, SAMPLES, (float)AUDIO_SAMPLE_RATE);

// ============================================================
//  JALUR A2DP: ring buffer (producer = task Bluetooth, consumer = loop)
// ============================================================
// Callback menulis sampel mono ke ring buffer. loop() menyalin SAMPLES sampel
// TERBARU ke vReal. Dengan begitu vReal tidak pernah ditimpa saat sedang
// dibaca visualizer, dan data yang dipakai selalu yang paling baru.
#define RING_SIZE 2048                      // harus pangkat 2, > SAMPLES
#define RING_MASK (RING_SIZE - 1)
#define AUDIO_STALE_MS 250                  // tanpa callback selama ini = audio berhenti

static int16_t ring[RING_SIZE];
static volatile uint32_t ringHead = 0;      // total sampel mono yang pernah ditulis
static volatile uint32_t lastAudioMs = 0;
static uint32_t consumedHead = 0;
static bool wasFlowing = false;

static void a2dpConnectionStateChanged(esp_a2d_connection_state_t state, void *obj)
{
    a2dpConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);

    if (!a2dpConnected)
    {
        metaSet(META_TITLE, "No song");
        metaSet(META_ARTIST, "");
    }

    Serial.print("A2DP: ");
    Serial.println(a2dpConnected ? "CONNECTED - visualizer pakai audio Bluetooth"
                                  : "DISCONNECTED - visualizer pakai INMP441");
}

static void audio_data_callback(const uint8_t *data, uint32_t len)
{
    // PCM 16-bit STEREO interleaved: L0 R0 L1 R1 ...
    const int16_t *s = (const int16_t *)data;
    int frames = len / (2 * sizeof(int16_t));

    if (frames <= 0)
        return;

    uint32_t head = ringHead;
    long sum = 0;

    for (int i = 0; i < frames; i++)
    {
        int m = ((int)s[2 * i] + (int)s[2 * i + 1]) >> 1;   // downmix ke mono

        ring[head & RING_MASK] = (int16_t)m;
        head++;
        sum += (m < 0) ? -m : m;
    }

    __sync_synchronize();          // pastikan data tertulis sebelum head dipublikasikan
    ringHead = head;
    lastAudioMs = millis();

    level = sum / frames;
}

static void avrc_metadata_callback(uint8_t id, const uint8_t *text)
{
    Serial.printf("AVRC metadata: ID=0x%02X  %s\n", id, (const char *)text);

    if (id == ESP_AVRC_MD_ATTR_TITLE)
        metaSet(META_TITLE, (const char *)text);
    else if (id == ESP_AVRC_MD_ATTR_ARTIST)
        metaSet(META_ARTIST, (const char *)text);
}

// Salin SAMPLES sampel terbaru ke vReal. false = belum ada data baru.
static bool a2dpFetchWindow()
{
    uint32_t head = ringHead;
    __sync_synchronize();

    if (head < SAMPLES)
        return false;

    if ((uint32_t)(head - consumedHead) < HOP_SAMPLES)
        return false;

    uint32_t start = head - SAMPLES;

    for (int i = 0; i < SAMPLES; i++)
        vReal[i] = (float)ring[(start + i) & RING_MASK];

    __sync_synchronize();

    // Kalau producer sempat "menyalip" bagian yang kita baca, buang jendela ini.
    if ((uint32_t)(ringHead - start) > (uint32_t)RING_SIZE)
        return false;

    consumedHead = head;
    memset(vImag, 0, sizeof(vImag));
    return true;
}

// ============================================================
//  JALUR MIKROFON INMP441
// ============================================================
// AGC sederhana di domain waktu supaya amplitudo mic sebanding dengan PCM A2DP.
#define MIC_GATE_OPEN    20.0f      // envelope (skala 16-bit) di atas ini = ada suara
#define MIC_GATE_CLOSE   12.0f      // di bawah ini = hening (hysteresis)
#define MIC_TARGET_PEAK  3000.0f    // target puncak setelah gain
#define MIC_MAX_GAIN     100.0f
#define MIC_MIN_GAIN     1.0f

static bool micI2SActive = false;
static unsigned long lastMicInitAttempt = 0;

static float micGain = 20.0f;
static float micEnv  = 0.0f;
static float micDc   = 0.0f;
static bool  micGateOpen = false;
static float micWin[SAMPLES];

// Pilihan channel: kalau mic hanya mengeluarkan nol (channel salah), coba channel satunya.
static bool micRightChannel = true;
static bool micChannelLocked = false;
static int  micChannelToggles = 0;
static uint32_t micZeroChunks = 0;

static bool micI2SInit()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = (int)AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = micRightChannel ? I2S_CHANNEL_FMT_ONLY_RIGHT
                                          : I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = MIC_CHUNK,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    // mck_io_num WAJIB di-set NO_CHANGE; kalau dibiarkan 0, IDF 4.4 menyalakan
    // clock keluar di GPIO0 (pin strapping boot).
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = MIC_I2S_BCLK,
        .ws_io_num = MIC_I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_I2S_DIN
    };

    esp_err_t err = i2s_driver_install(MIC_I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK)
    {
        Serial.printf("INMP441: i2s_driver_install gagal (%d)\n", (int)err);
        return false;
    }

    err = i2s_set_pin(MIC_I2S_PORT, &pin_config);
    if (err != ESP_OK)
    {
        Serial.printf("INMP441: i2s_set_pin gagal (%d)\n", (int)err);
        i2s_driver_uninstall(MIC_I2S_PORT);
        return false;
    }

    i2s_zero_dma_buffer(MIC_I2S_PORT);

    // reset state pemrosesan
    micGain = 20.0f;
    micEnv = 0.0f;
    micDc = 0.0f;
    micGateOpen = false;
    micZeroChunks = 0;
    memset(micWin, 0, sizeof(micWin));

    Serial.printf("INMP441 aktif: BCLK=%d WS=%d SD=%d channel=%s\n",
                  MIC_I2S_BCLK, MIC_I2S_WS, MIC_I2S_DIN,
                  micRightChannel ? "RIGHT" : "LEFT");
    return true;
}

static void micI2SStop()
{
    if (micI2SActive)
    {
        i2s_driver_uninstall(MIC_I2S_PORT);
        micI2SActive = false;
        Serial.println("INMP441 standby; audio visualizer kembali ke A2DP");
    }
}

static void micWinPush(const float *chunk, int n)
{
    if (n >= SAMPLES)
    {
        memcpy(micWin, chunk + (n - SAMPLES), SAMPLES * sizeof(float));
    }
    else
    {
        memmove(micWin, micWin + n, (SAMPLES - n) * sizeof(float));
        memcpy(micWin + (SAMPLES - n), chunk, n * sizeof(float));
    }
}

// Baca semua data yang tersedia di DMA (tanpa blocking). true = ada data baru.
static bool micReadAndPush()
{
    static int32_t raw[MIC_CHUNK];
    static float chunk[MIC_CHUNK];

    bool any = false;
    float lastPeak = 0.0f;

    for (int iter = 0; iter < 8; iter++)
    {
        size_t br = 0;
        esp_err_t err = i2s_read(MIC_I2S_PORT, raw, sizeof(raw), &br, 0);

        if (err != ESP_OK || br < sizeof(int32_t))
            break;

        int n = br / sizeof(int32_t);
        bool allZero = true;
        float peak = 0.0f;

        for (int i = 0; i < n; i++)
        {
            if (raw[i] != 0)
                allZero = false;

            // INMP441: 24-bit signed di slot 32-bit. >>8 = 24-bit, /256 = skala 16-bit.
            float x = (float)(raw[i] >> 8) / 256.0f;

            micDc += (x - micDc) * 0.002f;      // high-pass: buang DC offset
            x -= micDc;

            chunk[i] = x;

            float a = fabsf(x);
            if (a > peak) peak = a;
        }

        // Deteksi channel salah: data selalu persis nol.
        if (!micChannelLocked)
        {
            if (allZero)
            {
                micZeroChunks++;

                if (micZeroChunks > (AUDIO_SAMPLE_RATE * 3 / 2) / MIC_CHUNK &&
                    micChannelToggles < 2)
                {
                    micChannelToggles++;
                    micRightChannel = !micRightChannel;
                    Serial.println("INMP441: data nol terus, mencoba channel lain");
                    micI2SStop();
                    lastMicInitAttempt = 0;     // init ulang segera
                    return false;
                }
            }
            else
            {
                micChannelLocked = true;
                micZeroChunks = 0;
            }
        }

        // Envelope follower (puncak naik instan, turun ~0.4 detik)
        micEnv = fmaxf(peak, micEnv * expf(-(float)n / ((float)AUDIO_SAMPLE_RATE * 0.4f)));

        // Noise gate dengan hysteresis
        if (!micGateOpen && micEnv > MIC_GATE_OPEN)
            micGateOpen = true;
        else if (micGateOpen && micEnv < MIC_GATE_CLOSE)
            micGateOpen = false;

        if (micGateOpen)
        {
            // AGC: turun cepat saat keras, naik pelan saat pelan
            float desired = MIC_TARGET_PEAK / fmaxf(micEnv, 1.0f);

            if (desired < micGain)
                micGain += (desired - micGain) * 0.35f;
            else
                micGain += (desired - micGain) * 0.01f;

            if (micGain > MIC_MAX_GAIN) micGain = MIC_MAX_GAIN;
            if (micGain < MIC_MIN_GAIN) micGain = MIC_MIN_GAIN;

            float sumAbs = 0.0f;

            for (int i = 0; i < n; i++)
            {
                float v = chunk[i] * micGain;
                if (v >  32767.0f) v =  32767.0f;
                if (v < -32767.0f) v = -32767.0f;
                chunk[i] = v;
                sumAbs += fabsf(v);
            }

            level = (int)(sumAbs / n);
        }
        else
        {
            // Hening: nol-kan agar bar tidak berkedip karena noise mikrofon
            memset(chunk, 0, n * sizeof(float));
            level = 0;
        }

        micWinPush(chunk, n);
        lastPeak = peak;
        any = true;

        if (br < sizeof(raw))
            break;
    }

    // Debug: cek di Serial Monitor sekali per detik
    static unsigned long lastDbg = 0;
    if (any && millis() - lastDbg > 1000)
    {
        lastDbg = millis();
        Serial.printf("MIC peak=%.1f env=%.1f gain=%.1f gate=%d level=%d\n",
                      lastPeak, micEnv, micGain, (int)micGateOpen, (int)level);
    }

    return any;
}

// ============================================================
//  FFT
// ============================================================
static void runFFT()
{
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();
}

// ============================================================
//  PUBLIC
// ============================================================
bool audioMicActive()
{
    return micI2SActive;
}

void audioInit()
{
    // PCM5102A output pins, tetap dikelola oleh ESP32-A2DP.
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = 19,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    a2dp_sink.set_pin_config(pin_config);
    a2dp_sink.set_stream_reader(audio_data_callback);

    a2dp_sink.set_avrc_metadata_attribute_mask(
        ESP_AVRC_MD_ATTR_TITLE |
        ESP_AVRC_MD_ATTR_ARTIST |
        ESP_AVRC_MD_ATTR_ALBUM
    );
    a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
    a2dp_sink.set_on_connection_state_changed(a2dpConnectionStateChanged);

    a2dp_sink.start("PixelOS");
}

void audioProcess()
{
    uint32_t now = millis();
    bool fresh = false;
    bool flowing = false;

    if (a2dpConnected)
    {
        // Saat Bluetooth A2DP tersambung, mic harus standby agar sumber
        // visualizer hanya audio Bluetooth.
        micI2SStop();

        flowing = (now - lastAudioMs) < AUDIO_STALE_MS;
        fresh = flowing && a2dpFetchWindow();

        // Sinkronkan ikon play/pause dengan kenyataan (mis. di-pause dari HP).
        if (flowing != wasFlowing)
        {
            wasFlowing = flowing;
            isPlaying = flowing;
        }

        if (!flowing)
            level = 0;
    }
    else
    {
        wasFlowing = false;

        // Tidak ada koneksi A2DP: nyalakan INMP441 untuk visualizer suara sekitar.
        if (!micI2SActive)
        {
            if (millis() - lastMicInitAttempt >= 2000)
            {
                lastMicInitAttempt = millis();

                if (micI2SInit())
                    micI2SActive = true;
            }
        }

        if (micI2SActive)
        {
            flowing = true;

            if (micReadAndPush())
            {
                memcpy(vReal, micWin, sizeof(micWin));
                memset(vImag, 0, sizeof(vImag));
                fresh = true;
            }
        }
        else
        {
            level = 0;
        }
    }

    if (fresh)
        runFFT();

    analyzerUpdate(fresh, flowing);
}
