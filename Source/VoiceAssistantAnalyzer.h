// ==============================================================================
// mettavoxad - Source/VoiceAssistantAnalyzer.h
// Analisador deterministico local para o Assistente de Voz (Zero nuvem / Zero IA)
// Mede RMS ativo, Pico, Fator de Crista, Piso de Ruido e Balanco Espectral em 3 bandas.
// ==============================================================================
#pragma once
#include <cmath>
#include <algorithm>
#include <vector>

namespace mettavoxad_assistant
{
    struct VoiceAnalysisResult
    {
        float activeRmsDb     = -24.0f;
        float peakDb          = -12.0f;
        float crestFactorDb   = 12.0f;
        float noiseFloorDb    = -60.0f;
        float lowMudRatioDb   = 0.0f;
        float sibilanceRatioDb= -6.0f;

        float suggestedInputGainDb = 0.0f;
        float suggestedGateDb      = -54.0f;
        float suggestedPesoPct     = 48.0f;
        float suggestedClarezaPct  = 52.0f;
        float suggestedCompPct     = 50.0f;
        float suggestedDeEsserPct  = 42.0f;
    };

    class LocalVoiceAnalyzer
    {
    public:
        void reset (double sr) noexcept
        {
            sampleRate = sr;
            lp280 = lp3800 = 0.0;
            energyLow = energyMid = energySib = 1.0e-7;
            peakAbs = 0.0f;
            frameAccumSq = 0.0;
            frameSampleCount = 0;
            frameRmsHistory.clear();
        }

        void pushSample (float monoSample) noexcept
        {
            const float ax = std::abs (monoSample);
            if (ax > peakAbs) peakAbs = ax;

            const double aLow = std::exp (-2.0 * 3.1415926535 * 280.0 / sampleRate);
            const double aMid = std::exp (-2.0 * 3.1415926535 * 3800.0 / sampleRate);
            lp280  = (1.0 - aLow) * monoSample + aLow * lp280;
            lp3800 = (1.0 - aMid) * monoSample + aMid * lp3800;

            const double bMid = lp3800 - lp280;
            const double bSib = monoSample - lp3800;
            energyLow += lp280 * lp280;
            energyMid += bMid * bMid;
            energySib += bSib * bSib;

            frameAccumSq += monoSample * monoSample;
            if (++frameSampleCount >= 1024)
            {
                const float rms = static_cast<float> (std::sqrt (frameAccumSq / 1024.0));
                const float db  = 20.0f * std::log10 (std::max (rms, 1.0e-5f));
                if (frameRmsHistory.size() < 2048)
                    frameRmsHistory.push_back (db);
                frameAccumSq = 0.0;
                frameSampleCount = 0;
            }
        }

        VoiceAnalysisResult computeSuggestions (int applicationMode, float intensityScale = 1.0f)
        {
            VoiceAnalysisResult r;
            if (frameRmsHistory.empty())
                return r;

            std::sort (frameRmsHistory.begin(), frameRmsHistory.end());
            r.noiseFloorDb  = frameRmsHistory[frameRmsHistory.size() * 12 / 100];
            r.activeRmsDb   = frameRmsHistory[frameRmsHistory.size() * 75 / 100];
            r.peakDb        = 20.0f * std::log10 (std::max (peakAbs, 1.0e-5f));
            r.crestFactorDb = std::max (3.0f, r.peakDb - r.activeRmsDb);
            r.lowMudRatioDb = static_cast<float> (10.0 * std::log10 (energyLow / energyMid));
            r.sibilanceRatioDb = static_cast<float> (10.0 * std::log10 (energySib / energyMid));

            const float targetRms = (applicationMode == 1) ? -15.5f : -17.5f;
            r.suggestedInputGainDb = std::clamp ((targetRms - r.activeRmsDb) * 0.75f, -12.0f, 12.0f);
            r.suggestedGateDb      = std::clamp (r.noiseFloorDb + 6.5f, -68.0f, -36.0f);
            r.suggestedPesoPct     = std::clamp ((applicationMode == 1 ? 64.0f : 48.0f) - (r.lowMudRatioDb > 2.0f ? 8.0f : 0.0f), 25.0f, 80.0f);
            r.suggestedClarezaPct  = std::clamp ((r.lowMudRatioDb > 0.0f ? 64.0f : 52.0f) * intensityScale, 35.0f, 82.0f);
            r.suggestedCompPct     = std::clamp ((r.crestFactorDb > 12.0f ? 62.0f : 48.0f) * intensityScale, 30.0f, 82.0f);
            r.suggestedDeEsserPct  = std::clamp ((r.sibilanceRatioDb > -8.0f ? 58.0f : 42.0f) * intensityScale, 25.0f, 78.0f);
            return r;
        }

    private:
        double sampleRate = 48000.0;
        double lp280 = 0.0, lp3800 = 0.0;
        double energyLow = 1.0e-7, energyMid = 1.0e-7, energySib = 1.0e-7;
        float peakAbs = 0.0f;
        double frameAccumSq = 0.0;
        int frameSampleCount = 0;
        std::vector<float> frameRmsHistory;
    };
}
