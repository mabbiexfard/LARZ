#include "../Source/Voice.h"

#include <cassert>
#include <cmath>

namespace
{
bool near (float actual, float expected)
{
    return std::abs (actual - expected) < 0.001f;
}
}

int main()
{
    const larz::EnvelopeParameters envelope { 10.0f, 400.0f, 0.7f, 500.0f, 600.0f };
    assert (near (larz::envelopeLevel (0.0f, envelope), 0.0f));
    assert (near (larz::envelopeLevel (5.0f, envelope), 0.5f));
    assert (near (larz::envelopeLevel (10.0f, envelope), 1.0f));
    assert (near (larz::envelopeLevel (410.0f, envelope), 0.7f));
    assert (near (larz::envelopeLevel (910.0f, envelope), 0.7f));
    assert (near (larz::envelopeLevel (1210.0f, envelope), 0.35f));
    assert (near (larz::envelopeLevel (1510.0f, envelope), 0.0f));

    const larz::PitchNode nodes[] {
        { 0.0f, 400.0f },
        { 40.0f, 100.0f },
        { 250.0f, 55.0f },
        { 1500.0f, 40.0f }
    };
    assert (near (larz::interpolatePitch (nodes, 4, 0.0f), 400.0f));
    assert (near (larz::interpolatePitch (nodes, 4, 20.0f), 200.0f));
    assert (near (larz::interpolatePitch (nodes, 4, 1500.0f), 40.0f));
    assert (near (larz::interpolatePitch (nodes, 4, 5000.0f), 40.0f));
    assert (larz::frequencyToMidiNote (440.0f) == 69);
    assert (larz::frequencyToMidiNote (261.6256f) == 60);
    assert (larz::frequencyToMidiNote (34.6478f) == 25);

    const larz::EnvelopeParameters instantAttack { 0.0f, 1.0f, 0.5f, 0.0f, 1.0f };
    assert (near (larz::envelopeLevel (0.0f, instantAttack), 1.0f));
    assert (near (larz::envelopeLevel (1.0f, instantAttack), 0.5f));
}
