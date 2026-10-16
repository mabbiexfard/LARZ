#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace larz
{
struct PitchNode
{
    float timeMs = 0.0f;
    float frequencyHz = 20.0f;
};

struct EnvelopeParameters
{
    float attackMs = 0.0f;
    float decayMs = 1.0f;
    float sustain = 0.0f;
    float holdMs = 0.0f;
    float releaseMs = 1.0f;
};

inline float envelopeLevel (float timeMs, const EnvelopeParameters& envelope) noexcept
{
    const auto attack = std::max (0.0f, envelope.attackMs);
    const auto decay = std::max (1.0f, envelope.decayMs);
    const auto sustain = std::clamp (envelope.sustain, 0.0f, 1.0f);
    const auto hold = std::max (0.0f, envelope.holdMs);
    const auto release = std::max (1.0f, envelope.releaseMs);
    const auto decayEnd = attack + decay;
    const auto holdEnd = decayEnd + hold;
    const auto releaseEnd = holdEnd + release;

    if (timeMs < attack)
        return timeMs / attack;

    if (timeMs < decayEnd)
        return 1.0f - (1.0f - sustain) * ((timeMs - attack) / decay);

    if (timeMs < holdEnd)
        return sustain;

    if (timeMs < releaseEnd)
        return sustain * (1.0f - (timeMs - holdEnd) / release);

    return 0.0f;
}

inline float interpolatePitch (const PitchNode* nodes, int count, float timeMs) noexcept
{
    if (count < 2)
        return 0.0f;

    for (auto i = 0; i < count - 1; ++i)
    {
        if (timeMs <= nodes[i + 1].timeMs)
        {
            const auto duration = nodes[i + 1].timeMs - nodes[i].timeMs;
            const auto proportion = std::clamp ((timeMs - nodes[i].timeMs) / duration, 0.0f, 1.0f);
            const auto ratio = nodes[i + 1].frequencyHz / nodes[i].frequencyHz;
            return nodes[i].frequencyHz * std::pow (ratio, proportion);
        }
    }

    return nodes[count - 1].frequencyHz;
}

inline int frequencyToMidiNote (float frequencyHz) noexcept
{
    if (! std::isfinite (frequencyHz) || frequencyHz <= 0.0f)
        return 0;

    return static_cast<int> (std::lround (69.0f + 12.0f * std::log2 (frequencyHz / 440.0f)));
}

struct Voice
{
    bool active = false;
    double phaseRadians = 0.0;
    float elapsedMs = 0.0f;
    float frequencyMultiplier = 1.0f;
    float velocity = 1.0f;

    void start (double phase, float pitchMultiplier, float noteVelocity) noexcept
    {
        active = true;
        phaseRadians = phase;
        elapsedMs = 0.0f;
        frequencyMultiplier = pitchMultiplier;
        velocity = noteVelocity;
    }

    float render (double sampleRate,
                  const PitchNode* nodes,
                  int nodeCount,
                  const EnvelopeParameters& envelope) noexcept
    {
        if (! active || sampleRate <= 0.0)
            return 0.0f;

        const auto totalMs = std::max (0.0f, envelope.attackMs)
                           + std::max (1.0f, envelope.decayMs)
                           + std::max (0.0f, envelope.holdMs)
                           + std::max (1.0f, envelope.releaseMs);

        if (elapsedMs >= totalMs)
        {
            active = false;
            return 0.0f;
        }

        const auto frequency = interpolatePitch (nodes, nodeCount, elapsedMs) * frequencyMultiplier;
        phaseRadians += 2.0 * 3.14159265358979323846 * frequency / sampleRate;
        phaseRadians = std::fmod (phaseRadians, 2.0 * 3.14159265358979323846);
        const auto sample = std::sin (phaseRadians) * envelopeLevel (elapsedMs, envelope) * velocity;
        elapsedMs += static_cast<float> (1000.0 / sampleRate);
        return static_cast<float> (sample);
    }
};
}
