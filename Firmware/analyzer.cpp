#include "analyzer.h"
#include "state.h"
#include <math.h>

float spectrum[NUM_BANDS];
float bassEnergy    = 0.0f;
float trebleEnergy  = 0.0f;
float overallLevel  = 0.0f;
float beatPulse     = 0.0f;
bool  beatNow       = false;

// ---- Parameter yang bisa di-tune ----
// Satuan: sqrt(magnitudo FFT). Sinus 16-bit amplitudo A kira-kira memberi
// sqrt(A * SAMPLES/2 * 0.54) pada puncaknya.
static const float NOISE_FLOOR   = 60.0f;    // di bawah ini dianggap hening  (~ -62 dBFS)
static const float REF_MIN       = 220.0f;   // referensi minimum, mencegah sinyal kecil "meledak" (~ -39 dBFS)
static const float AGC_TAU       = 5.0f;     // detik; seberapa lambat referensi turun
static const float SHAPE_KEEP    = 0.35f;    // 0..1; seberapa jauh band lemah boleh dinaikkan relatif ke band terkuat
static const float ATTACK_TAU    = 0.025f;   // detik
static const float RELEASE_TAU   = 0.140f;   // detik
static const float CONTRAST      = 1.3f;     // >1 = puncak lebih menonjol

static float bandRaw[NUM_BANDS];    // nilai instan (sebelum smoothing)
static float bandPeak[NUM_BANDS];   // referensi AGC per band
static float globalPeak = 0.0f;
static float tilt[NUM_BANDS];
static int   edgeBin[NUM_BANDS + 1];
static bool  inited = false;

static float bassAvg = 0.0f;
static uint32_t lastUpdateMs = 0;
static uint32_t lastBeatMs   = 0;
static uint32_t lastActiveMs = 0;

static void analyzerInit()
{
    // Edge band log-spaced 80 Hz .. 10 kHz, dipetakan ke bin FFT.
    const float fLo = 80.0f, fHi = 10000.0f;
    const float binHz = (float)AUDIO_SAMPLE_RATE / (float)SAMPLES;
    const int   maxBin = SAMPLES / 2 - 1;

    int prev = 0;

    for (int k = 0; k <= NUM_BANDS; k++)
    {
        float f = fLo * powf(fHi / fLo, (float)k / (float)NUM_BANDS);
        int bin = (int)(f / binHz + 0.5f);

        if (bin < 1) bin = 1;
        if (bin <= prev) bin = prev + 1;      // tiap band minimal 1 bin
        if (bin > maxBin) bin = maxBin;

        edgeBin[k] = bin;
        prev = bin;
    }

    // Treble secara alami lebih lemah -> dorong sedikit.
    for (int b = 0; b < NUM_BANDS; b++)
        tilt[b] = 1.0f + b * 0.06f;

    inited = true;
}

void analyzerUpdate(bool fresh, bool flowing)
{
    if (!inited)
        analyzerInit();

    uint32_t now = millis();

    float dt = (lastUpdateMs == 0) ? 0.02f : (now - lastUpdateMs) / 1000.0f;
    lastUpdateMs = now;
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.1f)   dt = 0.1f;

    beatNow = false;

    // ---------- 1. Hitung nilai instan tiap band dari FFT ----------
    if (fresh)
    {
        float val[NUM_BANDS];
        float peakNow = 0.0f;

        for (int b = 0; b < NUM_BANDS; b++)
        {
            float m = 0.0f;

            for (int i = edgeBin[b]; i < edgeBin[b + 1]; i++)
                if (vReal[i] > m) m = vReal[i];

            val[b] = sqrtf(m) * tilt[b];

            if (val[b] > peakNow) peakNow = val[b];
        }

        // AGC: referensi naik instan, turun pelan.
        float decay = expf(-dt / AGC_TAU);

        globalPeak = fmaxf(globalPeak * decay, peakNow);

        for (int b = 0; b < NUM_BANDS; b++)
        {
            bandPeak[b] = fmaxf(bandPeak[b] * decay, val[b]);

            float ref = fmaxf(bandPeak[b], globalPeak * SHAPE_KEEP);
            ref = fmaxf(ref, REF_MIN);

            float n = (val[b] - NOISE_FLOOR) / (ref - NOISE_FLOOR);

            if (n < 0.0f) n = 0.0f;
            if (n > 1.0f) n = 1.0f;

            bandRaw[b] = powf(n, CONTRAST);
        }

        // ---------- 2. Beat detector (lonjakan energi bass) ----------
        float bassRaw = fmaxf(bandRaw[0], fmaxf(bandRaw[1], bandRaw[2] * 0.8f));

        if (bassRaw > 0.30f &&
            bassRaw > bassAvg * 1.25f + 0.06f &&
            (now - lastBeatMs) > 150)
        {
            lastBeatMs = now;
            beatNow = true;
            beatPulse = 1.0f;
        }

        bassAvg += (bassRaw - bassAvg) * (1.0f - expf(-dt / 0.5f));
    }
    else if (!flowing)
    {
        // Sumber berhenti: turunkan semuanya ke nol.
        for (int b = 0; b < NUM_BANDS; b++)
            bandRaw[b] = 0.0f;

        bassAvg *= expf(-dt / 0.3f);
    }
    // else: audio masih mengalir tapi belum ada jendela baru -> tahan nilai terakhir.

    // ---------- 3. Smoothing: attack cepat, release pelan ----------
    float aA = 1.0f - expf(-dt / ATTACK_TAU);
    float aR = 1.0f - expf(-dt / RELEASE_TAU);

    for (int b = 0; b < NUM_BANDS; b++)
    {
        float r = bandRaw[b];
        float s = spectrum[b];

        s += (r - s) * ((r > s) ? aA : aR);
        spectrum[b] = s;
    }

    beatPulse *= expf(-dt / 0.14f);

    // ---------- 4. Besaran turunan ----------
    float sum = 0.0f;
    for (int b = 0; b < NUM_BANDS; b++)
        sum += spectrum[b];

    overallLevel = sum / NUM_BANDS;

    // Ambil yang TERKUAT (bukan rata-rata): kick sub-bass murni hanya menyalakan
    // 1-2 band, dan tetap harus bisa membuat bassEnergy mencapai 1.0.
    bassEnergy = fmaxf(fmaxf(spectrum[0], spectrum[1]),
                       fmaxf(spectrum[2] * 0.9f, spectrum[3] * 0.7f));

    trebleEnergy = (spectrum[11] + spectrum[12] + spectrum[13] +
                    spectrum[14] + spectrum[15]) / 5.0f;

    if (overallLevel > 0.04f || beatPulse > 0.3f)
        lastActiveMs = now;
}

bool analyzerIdle()
{
    return (millis() - lastActiveMs) > IDLE_AFTER_MS;
}
