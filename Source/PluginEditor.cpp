#include "PluginEditor.h"

#include "BinaryData.h"

#include <algorithm>
#include <cmath>

namespace
{
float readValue (LarzAudioProcessor& processor, const char* parameterID)
{
    return processor.parameters.getRawParameterValue (parameterID)->load (std::memory_order_relaxed);
}

juce::String roundedValue (float value)
{
    return juce::String (juce::roundToInt (value));
}

juce::String pitchNoteName (float frequencyHz)
{
    static constexpr const char* noteNames[] {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    const auto midiNote = larz::frequencyToMidiNote (frequencyHz);
    const auto noteIndex = (midiNote % 12 + 12) % 12;
    const auto octave = static_cast<int> (std::floor (static_cast<float> (midiNote) / 12.0f)) - 1;
    return juce::String (noteNames[noteIndex]) + juce::String (octave);
}
}

void LarzLookAndFeel::drawRotarySlider (juce::Graphics& graphics, int x, int y, int width, int height,
                                        float sliderPosition, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                 static_cast<float> (width), static_cast<float> (height));
    const auto centre = bounds.getCentre();
    const auto radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
    const auto trackRadius = radius - 1.5f;

    graphics.setColour (juce::Colour (0xff050517).withAlpha (0.96f));
    graphics.fillEllipse (centre.x - radius + 4.0f, centre.y - radius + 4.0f,
                          (radius - 4.0f) * 2.0f, (radius - 4.0f) * 2.0f);
    graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.22f));
    graphics.drawEllipse (centre.x - radius + 4.0f, centre.y - radius + 4.0f,
                          (radius - 4.0f) * 2.0f, (radius - 4.0f) * 2.0f, 0.8f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    graphics.setColour (juce::Colours::white.withAlpha (0.10f));
    graphics.strokePath (track, juce::PathStrokeType (2.0f));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, trackRadius, trackRadius, 0.0f,
                            rotaryStartAngle, rotaryStartAngle + sliderPosition * (rotaryEndAngle - rotaryStartAngle), true);
    graphics.setGradientFill (juce::ColourGradient (juce::Colour (0xff07b9d0),
                                                     centre.x - trackRadius, centre.y - trackRadius,
                                                     juce::Colour (0xffb02ba8),
                                                     centre.x + trackRadius, centre.y + trackRadius, false));
    graphics.strokePath (valueArc, juce::PathStrokeType (2.0f));

    const auto angle = rotaryStartAngle + sliderPosition * (rotaryEndAngle - rotaryStartAngle);
    graphics.setColour (juce::Colour (0xffe8f6ff).withAlpha (0.92f));
    graphics.drawLine (centre.x + std::cos (angle) * (radius * 0.30f),
                       centre.y + std::sin (angle) * (radius * 0.30f),
                       centre.x + std::cos (angle) * (radius * 0.69f),
                       centre.y + std::sin (angle) * (radius * 0.69f), 1.5f);
}

void LarzLookAndFeel::drawToggleButton (juce::Graphics& graphics, juce::ToggleButton& button,
                                        bool, bool)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const auto pill = juce::Rectangle<float> (bounds.getX(), bounds.getCentreY() - 8.0f, 34.0f, 16.0f);
    const auto enabled = button.getToggleState();

    graphics.setColour (juce::Colour (0xff070420).withAlpha (0.82f));
    graphics.fillRoundedRectangle (pill, 8.0f);
    graphics.setColour ((enabled ? juce::Colour (0xff07b9d0) : juce::Colour (0xff8fa6c8)).withAlpha (0.55f));
    graphics.drawRoundedRectangle (pill, 8.0f, 1.0f);
    const auto dotX = enabled ? pill.getRight() - 12.0f : pill.getX() + 3.0f;
    graphics.setColour (enabled ? juce::Colour (0xff07b9d0) : juce::Colour (0xff8fa6c8).withAlpha (0.6f));
    graphics.fillEllipse (dotX, pill.getY() + 3.0f, 10.0f, 10.0f);

    graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.86f));
    graphics.setFont (juce::Font (juce::FontOptions (9.0f).withName ("Helvetica Neue").withStyle ("Light")
                                                              .withKerningFactor (0.2f)));
    graphics.drawText (button.getButtonText(), juce::Rectangle<int> (38, 0, button.getWidth() - 38, button.getHeight()),
                       juce::Justification::centredLeft, false);
}

void LarzFineSlider::mouseDown (const juce::MouseEvent& event)
{
    dragStart = event.position;
    dragStartValue = getValue();
    juce::Slider::mouseDown (event);
}

void LarzFineSlider::mouseDrag (const juce::MouseEvent& event)
{
    if (event.mods.isCommandDown() || event.mods.isCtrlDown())
    {
        const auto distance = (event.position.x - dragStart.x) + (dragStart.y - event.position.y);
        setValue (dragStartValue + distance * getRange().getLength() / 1000.0,
                  juce::sendNotificationSync);
        return;
    }

    juce::Slider::mouseDrag (event);
}

juce::Colour LarzAudioProcessorEditor::cyan()
{
    return juce::Colour (0xff07b9d0);
}

juce::Colour LarzAudioProcessorEditor::orange()
{
    return juce::Colour (0xfff4601d);
}

juce::Font LarzAudioProcessorEditor::font (float height, const juce::String& style, float kerning)
{
    return juce::Font (juce::FontOptions (height).withName ("Helvetica Neue")
                                                .withStyle (style)
                                                .withKerningFactor (kerning));
}

LarzAudioProcessorEditor::LarzAudioProcessorEditor (LarzAudioProcessor& audioProcessor)
    : AudioProcessorEditor (&audioProcessor), processor (audioProcessor)
{
    sourceBackground = juce::ImageCache::getFromMemory (BinaryData::Cyberpunk_Paisley_Circuit_Void_png,
                                                        BinaryData::Cyberpunk_Paisley_Circuit_Void_pngSize);
    jassert (sourceBackground.isValid());

    const std::array<const char*, 4> parameterIDs {
        LarzAudioProcessor::gainParam, LarzAudioProcessor::phaseParam,
        LarzAudioProcessor::rootParam, LarzAudioProcessor::viewParam
    };
    const std::array<float, 4> defaults { -6.0f, 0.0f, 36.0f, 2000.0f };
    const std::array<float, 4> minimums { -48.0f, 0.0f, 0.0f, 500.0f };
    const std::array<float, 4> maximums { 12.0f, 360.0f, 127.0f, 8000.0f };
    const std::array<float, 4> steps { 0.1f, 1.0f, 1.0f, 10.0f };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob.setRange (minimums[i], maximums[i], steps[i]);
        knob.setDoubleClickReturnValue (true, defaults[i]);
        knob.setTooltip ({});
        knob.setLookAndFeel (&lookAndFeel);
        knob.onValueChange = [this] { repaint(); };
        addAndMakeVisible (knob);
        knobAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.parameters, parameterIDs[i], knob);
    }

    keyTrackButton.setButtonText ("KEY TRACK");
    keyTrackButton.setTooltip ({});
    keyTrackButton.setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (keyTrackButton);
    keyTrackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.parameters, LarzAudioProcessor::keyTrackParam, keyTrackButton);

    setResizable (true, true);
    setResizeLimits (800, 450, 1672, 941);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (16.0 / 9.0);
    setSize (1280, 720);
}

LarzAudioProcessorEditor::~LarzAudioProcessorEditor()
{
    for (auto& knob : knobs)
        knob.setLookAndFeel (nullptr);
    keyTrackButton.setLookAndFeel (nullptr);
}

void LarzAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    if (scaledBackground.isValid())
        graphics.drawImageAt (scaledBackground, 0, 0);
    else
        graphics.fillAll (juce::Colour (0xff050517));

    drawPanel (graphics, pitchPanel, cyan());
    drawPanel (graphics, ampPanel, orange());

    graphics.setColour (juce::Colour (0xffe8f6ff).withAlpha (0.91f));
    graphics.setFont (font (28.0f * scale, "Light", 0.3f * scale));
    graphics.drawText ("LARZ", juce::Rectangle<int> (0, juce::roundToInt (19.0f * scale), getWidth(),
                                                      juce::roundToInt (34.0f * scale)),
                       juce::Justification::centred, false);
    graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.55f));
    graphics.setFont (font (9.0f * scale, "Light", 0.3f * scale));
    graphics.drawText ("MabbieFX", juce::Rectangle<int> (0, juce::roundToInt (53.0f * scale), getWidth(),
                                                          juce::roundToInt (14.0f * scale)),
                       juce::Justification::centred, false);

    drawPitchPanel (graphics);
    drawAmpPanel (graphics);

    const std::array<juce::String, 4> values {
        juce::String (knobs[0].getValue(), 1) + " dB",
        roundedValue (static_cast<float> (knobs[1].getValue())) + juce::String::fromUTF8 ("\xc2\xb0"),
        roundedValue (static_cast<float> (knobs[2].getValue())),
        roundedValue (static_cast<float> (knobs[3].getValue())) + " ms"
    };
    const std::array<const char*, 4> labels { "GAIN", "START PHASE", "ROOT NOTE", "VIEW LENGTH" };
    const std::array<float, 4> centres { 0.25f, 0.40f, 0.55f, 0.70f };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto centreX = juce::roundToInt (centres[i] * static_cast<float> (getWidth()));
        graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.66f));
        graphics.setFont (font (9.0f * scale, "Light", 0.55f * scale));
        graphics.drawText (labels[i], juce::Rectangle<int> (centreX - juce::roundToInt (62.0f * scale),
                                                              juce::roundToInt (0.887f * getHeight()),
                                                              juce::roundToInt (124.0f * scale),
                                                              juce::roundToInt (13.0f * scale)),
                           juce::Justification::centred, false);
        graphics.setColour (juce::Colour (0xffe8f6ff).withAlpha (0.86f));
        graphics.setFont (font (10.0f * scale, "Light", 0.15f * scale));
        graphics.drawText (values[i], juce::Rectangle<int> (centreX - juce::roundToInt (62.0f * scale),
                                                              juce::roundToInt (0.915f * getHeight()),
                                                              juce::roundToInt (124.0f * scale),
                                                              juce::roundToInt (14.0f * scale)),
                           juce::Justification::centred, false);
    }
}

void LarzAudioProcessorEditor::resized()
{
    const auto width = static_cast<float> (getWidth());
    const auto height = static_cast<float> (getHeight());
    scale = std::min (width / 1280.0f, height / 720.0f);

    pitchPanel = { width * 0.17f, height * 0.12f, width * 0.66f, height * 0.33f };
    ampPanel = { width * 0.17f, height * 0.48f, width * 0.66f, height * 0.28f };
    pitchPlot = pitchPanel.withTrimmedLeft (44.0f * scale).withTrimmedRight (18.0f * scale)
                           .withTrimmedTop (33.0f * scale).withTrimmedBottom (22.0f * scale);
    ampPlot = ampPanel.withTrimmedLeft (44.0f * scale).withTrimmedRight (18.0f * scale)
                       .withTrimmedTop (33.0f * scale).withTrimmedBottom (22.0f * scale);

    const std::array<float, 4> centres { 0.25f, 0.40f, 0.55f, 0.70f };
    const auto knobSize = juce::roundToInt (60.0f * scale);
    const auto knobY = juce::roundToInt (0.792f * height);
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].setBounds (juce::roundToInt (centres[i] * width) - knobSize / 2,
                            knobY, knobSize, knobSize);

    keyTrackButton.setBounds (juce::roundToInt (0.73f * width), juce::roundToInt (0.835f * height),
                              juce::roundToInt (0.08f * width), juce::roundToInt (42.0f * scale));

    if (sourceBackground.isValid() && getWidth() > 0 && getHeight() > 0)
    {
        scaledBackground = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics imageGraphics (scaledBackground);
        imageGraphics.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        imageGraphics.drawImage (sourceBackground, 0, 0, getWidth(), getHeight(),
                                 0, 0, sourceBackground.getWidth(), sourceBackground.getHeight());
    }
}

void LarzAudioProcessorEditor::mouseMove (const juce::MouseEvent& event)
{
    const auto pitch = pitchPlot.contains (event.position) ? pitchNodeAt (event.position) : -1;
    const auto amp = ampPlot.contains (event.position) ? ampNodeAt (event.position) : -1;
    if (pitch != hoveredPitchNode || amp != hoveredAmpNode)
    {
        hoveredPitchNode = pitch;
        hoveredAmpNode = amp;
        repaint();
    }
}

void LarzAudioProcessorEditor::mouseExit (const juce::MouseEvent&)
{
    hoveredPitchNode = -1;
    hoveredAmpNode = -1;
    repaint();
}

void LarzAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    dragPosition = event.position;

    if (pitchPlot.contains (event.position))
    {
        selectedPitchNode = pitchNodeAt (event.position);
        if (selectedPitchNode >= 0)
            draggingPitchNode = selectedPitchNode;
        repaint();
        return;
    }

    if (ampPlot.contains (event.position))
    {
        draggingAmpNode = ampNodeAt (event.position);
        if (draggingAmpNode >= 0)
            beginAmpGestures (draggingAmpNode);
        repaint();
    }
}

void LarzAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    dragPosition = event.position;
    if (draggingPitchNode >= 0)
        updatePitchNode (event.position);
    else if (draggingAmpNode >= 0)
        updateAmpNode (event.position);
}

void LarzAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    draggingPitchNode = -1;
    draggingAmpNode = -1;
    endAmpGestures();
}

void LarzAudioProcessorEditor::mouseDoubleClick (const juce::MouseEvent& event)
{
    if (! pitchPlot.contains (event.position))
        return;

    const auto index = pitchNodeAt (event.position);
    if (index > 0)
        removePitchNode (index);
    else if (index < 0)
        addPitchNode (event.position);
}

juce::Point<float> LarzAudioProcessorEditor::pitchNodePoint (const larz::PitchNode& node, float viewMs) const
{
    const auto frequency = std::clamp (node.frequencyHz, 20.0f, 2000.0f);
    const auto x = pitchPlot.getX() + node.timeMs / viewMs * pitchPlot.getWidth();
    const auto y = pitchPlot.getY() + std::log (2000.0f / frequency) / std::log (100.0f) * pitchPlot.getHeight();
    return { x, y };
}

juce::Point<float> LarzAudioProcessorEditor::ampNodePoint (int index) const
{
    const auto attack = readValue (processor, LarzAudioProcessor::attackParam);
    const auto decay = readValue (processor, LarzAudioProcessor::decayParam);
    const auto sustain = readValue (processor, LarzAudioProcessor::sustainParam);
    const auto hold = readValue (processor, LarzAudioProcessor::holdParam);
    const auto release = readValue (processor, LarzAudioProcessor::releaseParam);
    const auto viewMs = static_cast<float> (knobs[3].getValue());

    const std::array<float, 4> times {
        attack, attack + decay, attack + decay + hold, attack + decay + hold + release
    };
    const std::array<float, 4> levels { 1.0f, sustain, sustain, 0.0f };
    const auto x = ampPlot.getX() + std::min (times[static_cast<size_t> (index)], viewMs) / viewMs * ampPlot.getWidth();
    const auto y = ampPlot.getBottom() - levels[static_cast<size_t> (index)] * ampPlot.getHeight();
    return { x, y };
}

int LarzAudioProcessorEditor::pitchNodeAt (juce::Point<float> point) const
{
    LarzAudioProcessor::PitchNodes nodes;
    int count = 0;
    processor.getPitchNodes (nodes, count);
    const auto viewMs = static_cast<float> (knobs[3].getValue());
    auto best = -1;
    auto bestDistance = 14.0f * scale;

    for (auto i = 0; i < count; ++i)
    {
        const auto& node = nodes[static_cast<size_t> (i)];
        if (node.timeMs > viewMs)
            continue;
        const auto distance = point.getDistanceFrom (pitchNodePoint (node, viewMs));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }

    return best;
}

int LarzAudioProcessorEditor::ampNodeAt (juce::Point<float> point) const
{
    auto best = -1;
    auto bestDistance = 14.0f * scale;
    for (auto i = 0; i < 4; ++i)
    {
        const auto distance = point.getDistanceFrom (ampNodePoint (i));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

float LarzAudioProcessorEditor::frequencyFromY (float y) const
{
    const auto position = std::clamp ((pitchPlot.getBottom() - y) / pitchPlot.getHeight(), 0.0f, 1.0f);
    return 20.0f * std::pow (100.0f, position);
}

float LarzAudioProcessorEditor::timeFromX (float x) const
{
    const auto viewMs = static_cast<float> (knobs[3].getValue());
    return std::clamp ((x - pitchPlot.getX()) / pitchPlot.getWidth() * viewMs, 0.0f, viewMs);
}

void LarzAudioProcessorEditor::addPitchNode (juce::Point<float> point)
{
    LarzAudioProcessor::PitchNodes nodes;
    int count = 0;
    processor.getPitchNodes (nodes, count);
    if (count >= static_cast<int> (nodes.size()))
        return;

    const auto time = timeFromX (point.x);
    auto index = 1;
    while (index < count && nodes[static_cast<size_t> (index)].timeMs < time)
        ++index;

    const auto lower = nodes[static_cast<size_t> (index - 1)].timeMs + 1.0f;
    const auto upper = index < count ? nodes[static_cast<size_t> (index)].timeMs - 1.0f
                                     : static_cast<float> (knobs[3].getValue());
    if (lower > upper)
        return;

    std::move_backward (nodes.begin() + index, nodes.begin() + count, nodes.begin() + count + 1);
    nodes[static_cast<size_t> (index)] = { std::clamp (time, lower, upper), frequencyFromY (point.y) };
    processor.setPitchNodes (nodes, count + 1);
    selectedPitchNode = index;
    repaint();
}

void LarzAudioProcessorEditor::removePitchNode (int index)
{
    LarzAudioProcessor::PitchNodes nodes;
    int count = 0;
    processor.getPitchNodes (nodes, count);
    if (index <= 0 || index >= count || count <= 2)
        return;

    std::move (nodes.begin() + index + 1, nodes.begin() + count, nodes.begin() + index);
    processor.setPitchNodes (nodes, count - 1);
    selectedPitchNode = -1;
    repaint();
}

void LarzAudioProcessorEditor::updatePitchNode (juce::Point<float> point)
{
    LarzAudioProcessor::PitchNodes nodes;
    int count = 0;
    processor.getPitchNodes (nodes, count);
    if (draggingPitchNode < 0 || draggingPitchNode >= count)
        return;

    const auto index = static_cast<size_t> (draggingPitchNode);
    if (index > 0)
    {
        const auto low = nodes[index - 1].timeMs + 1.0f;
        const auto viewMs = static_cast<float> (knobs[3].getValue());
        const auto nextTime = index + 1 < static_cast<size_t> (count) ? nodes[index + 1].timeMs - 1.0f : viewMs;
        const auto high = std::max (low, std::min (viewMs, nextTime));
        nodes[index].timeMs = std::clamp (timeFromX (point.x), low, high);
    }
    nodes[index].frequencyHz = frequencyFromY (point.y);
    processor.setPitchNodes (nodes, count);
    repaint();
}

void LarzAudioProcessorEditor::beginAmpGestures (int index)
{
    const std::array<const char*, 2> ids = [index]
    {
        switch (index)
        {
            case 0: return std::array<const char*, 2> { LarzAudioProcessor::attackParam, nullptr };
            case 1: return std::array<const char*, 2> { LarzAudioProcessor::decayParam, LarzAudioProcessor::sustainParam };
            case 2: return std::array<const char*, 2> { LarzAudioProcessor::holdParam, LarzAudioProcessor::sustainParam };
            default: return std::array<const char*, 2> { LarzAudioProcessor::releaseParam, nullptr };
        }
    }();

    activeGestureCount = 0;
    for (const auto* id : ids)
    {
        if (id == nullptr)
            continue;
        if (auto* parameter = processor.parameters.getParameter (id))
        {
            parameter->beginChangeGesture();
            activeGestures[static_cast<size_t> (activeGestureCount++)] = parameter;
        }
    }
}

void LarzAudioProcessorEditor::endAmpGestures()
{
    for (auto i = 0; i < activeGestureCount; ++i)
        activeGestures[static_cast<size_t> (i)]->endChangeGesture();
    activeGestureCount = 0;
}

void LarzAudioProcessorEditor::setParameterValue (const char* parameterID, float value)
{
    if (auto* parameter = processor.parameters.getParameter (parameterID))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void LarzAudioProcessorEditor::updateAmpNode (juce::Point<float> point)
{
    const auto attack = readValue (processor, LarzAudioProcessor::attackParam);
    const auto decay = readValue (processor, LarzAudioProcessor::decayParam);
    const auto hold = readValue (processor, LarzAudioProcessor::holdParam);
    const auto ms = std::clamp ((point.x - ampPlot.getX()) / ampPlot.getWidth()
                                    * static_cast<float> (knobs[3].getValue()),
                                0.0f, static_cast<float> (knobs[3].getValue()));
    const auto level = std::clamp ((ampPlot.getBottom() - point.y) / ampPlot.getHeight(), 0.0f, 1.0f);

    switch (draggingAmpNode)
    {
        case 0:
            setParameterValue (LarzAudioProcessor::attackParam, std::clamp (ms, 0.0f, 500.0f));
            break;
        case 1:
            setParameterValue (LarzAudioProcessor::decayParam, std::clamp (ms - attack, 1.0f, 4000.0f));
            setParameterValue (LarzAudioProcessor::sustainParam, level);
            break;
        case 2:
            setParameterValue (LarzAudioProcessor::holdParam, std::clamp (ms - attack - decay, 0.0f, 4000.0f));
            setParameterValue (LarzAudioProcessor::sustainParam, level);
            break;
        case 3:
            setParameterValue (LarzAudioProcessor::releaseParam,
                               std::clamp (ms - attack - decay - hold, 1.0f, 4000.0f));
            break;
        default:
            break;
    }
    repaint();
}

void LarzAudioProcessorEditor::drawPanel (juce::Graphics& graphics, juce::Rectangle<float> panel,
                                          juce::Colour accent) const
{
    graphics.setColour (juce::Colour (0xff070420).withAlpha (0.55f));
    graphics.fillRoundedRectangle (panel, 6.0f * scale);
    graphics.setColour (accent.withAlpha (0.26f));
    graphics.drawRoundedRectangle (panel.reduced (0.5f * scale), 6.0f * scale, 1.0f * scale);

    const auto bracket = 5.0f * scale;
    const auto left = panel.getX() + 1.0f * scale;
    const auto right = panel.getRight() - 1.0f * scale;
    const auto top = panel.getY() + 1.0f * scale;
    const auto bottom = panel.getBottom() - 1.0f * scale;
    graphics.setColour (accent.withAlpha (0.52f));
    graphics.drawLine (left, top + bracket, left, top);
    graphics.drawLine (left, top, left + bracket, top);
    graphics.drawLine (right - bracket, top, right, top);
    graphics.drawLine (right, top, right, top + bracket);
    graphics.drawLine (left, bottom - bracket, left, bottom);
    graphics.drawLine (left, bottom, left + bracket, bottom);
    graphics.drawLine (right - bracket, bottom, right, bottom);
    graphics.drawLine (right, bottom - bracket, right, bottom);
}

void LarzAudioProcessorEditor::drawGrid (juce::Graphics& graphics, juce::Rectangle<float> plot,
                                         juce::Colour, bool pitchGrid)
{
    const auto viewMs = static_cast<float> (knobs[3].getValue());
    const auto step = viewMs <= 1000.0f ? 100.0f : viewMs <= 2500.0f ? 250.0f : viewMs <= 5000.0f ? 500.0f : 1000.0f;
    graphics.setColour (juce::Colours::white.withAlpha (0.055f));
    for (auto time = 0.0f; time <= viewMs; time += step)
    {
        const auto x = plot.getX() + time / viewMs * plot.getWidth();
        graphics.drawLine (x, plot.getY(), x, plot.getBottom(), 0.7f * scale);
        graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.72f));
        graphics.setFont (font (8.0f * scale));
        graphics.drawText (roundedValue (time), juce::Rectangle<int> (juce::roundToInt (x - 20.0f * scale),
                                                                        juce::roundToInt (plot.getBottom() + 3.0f * scale),
                                                                        juce::roundToInt (40.0f * scale),
                                                                        juce::roundToInt (10.0f * scale)),
                           juce::Justification::centred, false);
        graphics.setColour (juce::Colours::white.withAlpha (0.055f));
    }

    if (pitchGrid)
    {
        const std::array<float, 8> frequencies { 20.0f, 40.0f, 80.0f, 160.0f, 320.0f, 640.0f, 1280.0f, 2000.0f };
        for (const auto frequency : frequencies)
        {
            const auto y = plot.getY() + std::log (2000.0f / frequency) / std::log (100.0f) * plot.getHeight();
            graphics.drawLine (plot.getX(), y, plot.getRight(), y, 0.7f * scale);
            graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.72f));
            graphics.setFont (font (8.0f * scale));
            graphics.drawText (roundedValue (frequency),
                               juce::Rectangle<int> (juce::roundToInt (pitchPanel.getX() + 4.0f * scale),
                                                     juce::roundToInt (y - 5.0f * scale),
                                                     juce::roundToInt (36.0f * scale),
                                                     juce::roundToInt (10.0f * scale)),
                               juce::Justification::centredRight, false);
            graphics.setColour (juce::Colours::white.withAlpha (0.055f));
        }
    }
    else
    {
        for (auto level = 0.0f; level <= 1.0f; level += 0.25f)
        {
            const auto y = plot.getBottom() - level * plot.getHeight();
            graphics.drawLine (plot.getX(), y, plot.getRight(), y, 0.7f * scale);
        }
    }
}

void LarzAudioProcessorEditor::drawPitchPanel (juce::Graphics& graphics)
{
    drawGrid (graphics, pitchPlot, cyan(), true);
    graphics.setColour (cyan().withAlpha (0.95f));
    graphics.setFont (font (9.0f * scale, "Light", 0.35f * scale));
    graphics.drawText ("PITCH  |  NOTE = Hz", juce::Rectangle<int> (juce::roundToInt (pitchPanel.getX() + 15.0f * scale),
                                                              juce::roundToInt (pitchPanel.getY() + 8.0f * scale),
                                                              juce::roundToInt (190.0f * scale),
                                                              juce::roundToInt (15.0f * scale)),
                       juce::Justification::centredLeft, false);

    LarzAudioProcessor::PitchNodes nodes;
    int count = 0;
    processor.getPitchNodes (nodes, count);
    const auto viewMs = static_cast<float> (knobs[3].getValue());
    juce::String readout = "VIEW  " + roundedValue (viewMs) + " ms";
    const auto focusIndex = selectedPitchNode >= 0 ? selectedPitchNode : hoveredPitchNode;
    if (focusIndex >= 0 && focusIndex < count)
    {
        const auto& node = nodes[static_cast<size_t> (focusIndex)];
        readout += "    " + pitchNoteName (node.frequencyHz) + "  ·  "
                 + roundedValue (node.frequencyHz) + " Hz  ·  " + roundedValue (node.timeMs) + " ms";
    }
    graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.68f));
    graphics.setFont (font (9.0f * scale));
    graphics.drawText (readout, juce::Rectangle<int> (juce::roundToInt (pitchPlot.getX()),
                                                       juce::roundToInt (pitchPanel.getY() + 8.0f * scale),
                                                       juce::roundToInt (pitchPlot.getWidth()),
                                                       juce::roundToInt (15.0f * scale)),
                       juce::Justification::centredRight, false);

    {
        juce::Graphics::ScopedSaveState state (graphics);
        graphics.reduceClipRegion (pitchPlot.toNearestInt());
        juce::Path curve;
        juce::Path fill;
        constexpr int samples = 160;
        for (auto i = 0; i <= samples; ++i)
        {
            const auto time = viewMs * static_cast<float> (i) / static_cast<float> (samples);
            const auto frequency = larz::interpolatePitch (nodes.data(), count, time);
            const auto x = pitchPlot.getX() + time / viewMs * pitchPlot.getWidth();
            const auto y = pitchPlot.getY() + std::log (2000.0f / frequency) / std::log (100.0f) * pitchPlot.getHeight();
            if (i == 0)
            {
                curve.startNewSubPath (x, y);
                fill.startNewSubPath (x, pitchPlot.getBottom());
                fill.lineTo (x, y);
            }
            else
            {
                curve.lineTo (x, y);
                fill.lineTo (x, y);
            }
        }
        fill.lineTo (pitchPlot.getRight(), pitchPlot.getBottom());
        fill.closeSubPath();
        graphics.setGradientFill (juce::ColourGradient (cyan().withAlpha (0.14f), pitchPlot.getX(), pitchPlot.getY(),
                                                         cyan().withAlpha (0.0f), pitchPlot.getX(), pitchPlot.getBottom(), false));
        graphics.fillPath (fill);
        graphics.setColour (cyan().withAlpha (0.13f));
        graphics.strokePath (curve, juce::PathStrokeType (8.0f * scale, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        graphics.setColour (cyan().withAlpha (0.96f));
        graphics.strokePath (curve, juce::PathStrokeType (2.0f * scale, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

        for (auto i = 0; i < count; ++i)
        {
            const auto& node = nodes[static_cast<size_t> (i)];
            if (node.timeMs > viewMs)
                continue;
            const auto point = pitchNodePoint (node, viewMs);
            const auto highlighted = i == hoveredPitchNode || i == selectedPitchNode;
            drawNode (graphics, point, cyan(), highlighted);
            constexpr float labelWidth = 78.0f;
            const auto labelX = juce::jlimit (pitchPlot.getX(),
                                              pitchPlot.getRight() - labelWidth * scale,
                                              point.x > pitchPlot.getRight() - (labelWidth + 12.0f) * scale
                                                  ? point.x - (labelWidth + 8.0f) * scale
                                                  : point.x + 9.0f * scale);
            const auto labelY = juce::jlimit (pitchPlot.getY(),
                                              pitchPlot.getBottom() - 14.0f * scale,
                                              point.y < pitchPlot.getY() + 22.0f * scale
                                                  ? point.y + 8.0f * scale
                                                  : point.y - 18.0f * scale);
            const auto labelBounds = juce::Rectangle<float> (labelX, labelY,
                                                              labelWidth * scale, 14.0f * scale);
            graphics.setColour (juce::Colour (0xff050218).withAlpha (0.88f));
            graphics.fillRoundedRectangle (labelBounds, 2.0f * scale);
            graphics.setColour (juce::Colour (0xffe8f6ff).withAlpha (highlighted ? 1.0f : 0.96f));
            graphics.setFont (font (8.0f * scale, "Light"));
            graphics.drawText (pitchNoteName (node.frequencyHz) + " = "
                                   + roundedValue (node.frequencyHz) + " Hz",
                               labelBounds.toNearestInt(),
                               juce::Justification::centredLeft, false);
        }
    }

    if (draggingPitchNode >= 0 && draggingPitchNode < count)
    {
        const auto& node = nodes[static_cast<size_t> (draggingPitchNode)];
        drawDragBadge (graphics, dragPosition,
                       pitchNoteName (node.frequencyHz) + "  ·  " + roundedValue (node.frequencyHz)
                           + " Hz  ·  " + roundedValue (node.timeMs) + " ms", cyan());
    }
}

void LarzAudioProcessorEditor::drawAmpPanel (juce::Graphics& graphics)
{
    drawGrid (graphics, ampPlot, orange(), false);
    graphics.setColour (orange().withAlpha (0.95f));
    graphics.setFont (font (10.0f * scale, "Light", 0.8f * scale));
    graphics.drawText ("AMP", juce::Rectangle<int> (juce::roundToInt (ampPanel.getX() + 15.0f * scale),
                                                     juce::roundToInt (ampPanel.getY() + 8.0f * scale),
                                                     juce::roundToInt (80.0f * scale),
                                                     juce::roundToInt (15.0f * scale)),
                       juce::Justification::centredLeft, false);

    const auto attack = readValue (processor, LarzAudioProcessor::attackParam);
    const auto decay = readValue (processor, LarzAudioProcessor::decayParam);
    const auto sustain = readValue (processor, LarzAudioProcessor::sustainParam);
    const auto hold = readValue (processor, LarzAudioProcessor::holdParam);
    const auto release = readValue (processor, LarzAudioProcessor::releaseParam);
    const auto totalMs = attack + decay + hold + release;
    auto readout = "TOTAL  " + roundedValue (totalMs) + " ms";
    if (hoveredAmpNode >= 0)
    {
        const std::array<const char*, 4> names { "ATTACK", "SUSTAIN", "HOLD", "RELEASE" };
        readout += "    " + juce::String (names[static_cast<size_t> (hoveredAmpNode)]);
        if (hoveredAmpNode == 1 || hoveredAmpNode == 2)
            readout += "  " + juce::String (sustain, 2);
    }
    graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.68f));
    graphics.setFont (font (9.0f * scale));
    graphics.drawText (readout, juce::Rectangle<int> (juce::roundToInt (ampPlot.getX()),
                                                       juce::roundToInt (ampPanel.getY() + 8.0f * scale),
                                                       juce::roundToInt (ampPlot.getWidth()),
                                                       juce::roundToInt (15.0f * scale)),
                       juce::Justification::centredRight, false);

    const auto viewMs = static_cast<float> (knobs[3].getValue());
    juce::Path curve;
    juce::Path fill;
    const auto first = juce::Point<float> (ampPlot.getX(), ampPlot.getBottom());
    curve.startNewSubPath (first);
    fill.startNewSubPath (first);

    const std::array<juce::Point<float>, 4> points {
        ampNodePoint (0), ampNodePoint (1), ampNodePoint (2), ampNodePoint (3)
    };
    for (const auto point : points)
    {
        curve.lineTo (point);
        fill.lineTo (point);
    }
    fill.lineTo (ampPlot.getRight(), ampPlot.getBottom());
    fill.closeSubPath();

    {
        juce::Graphics::ScopedSaveState state (graphics);
        graphics.reduceClipRegion (ampPlot.toNearestInt());
        graphics.setGradientFill (juce::ColourGradient (orange().withAlpha (0.15f), ampPlot.getX(), ampPlot.getY(),
                                                         orange().withAlpha (0.0f), ampPlot.getX(), ampPlot.getBottom(), false));
        graphics.fillPath (fill);
        graphics.setColour (orange().withAlpha (0.13f));
        graphics.strokePath (curve, juce::PathStrokeType (8.0f * scale, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
        graphics.setColour (orange().withAlpha (0.98f));
        graphics.strokePath (curve, juce::PathStrokeType (2.0f * scale, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

        if (totalMs <= viewMs)
        {
            const auto markerX = ampPlot.getX() + totalMs / viewMs * ampPlot.getWidth();
            graphics.setColour (orange().withAlpha (0.68f));
            for (auto y = ampPlot.getY(); y < ampPlot.getBottom(); y += 8.0f * scale)
                graphics.drawLine (markerX, y, markerX, std::min (y + 4.0f * scale, ampPlot.getBottom()), 1.0f * scale);
        }

        const std::array<const char*, 4> labels { "A", "D/S", "H/S", "R" };
        for (auto i = 0; i < 4; ++i)
        {
            const auto point = points[static_cast<size_t> (i)];
            drawNode (graphics, point, orange(), i == hoveredAmpNode || i == draggingAmpNode);
            graphics.setColour (juce::Colour (0xff8fa6c8).withAlpha (0.75f));
            graphics.setFont (font (8.0f * scale));
            graphics.drawText (labels[static_cast<size_t> (i)],
                               juce::Rectangle<int> (juce::roundToInt (point.x - 18.0f * scale),
                                                     juce::roundToInt (point.y - 17.0f * scale),
                                                     juce::roundToInt (36.0f * scale),
                                                     juce::roundToInt (10.0f * scale)),
                               juce::Justification::centred, false);
        }
    }

    if (draggingAmpNode >= 0)
    {
        const auto index = static_cast<size_t> (draggingAmpNode);
        const auto value = (draggingAmpNode == 1 || draggingAmpNode == 2)
                         ? juce::String (sustain, 2)
                         : roundedValue (std::array<float, 4> { attack, attack + decay,
                                                                attack + decay + hold, totalMs }[index]) + " ms";
        drawDragBadge (graphics, dragPosition, value, orange());
    }
}

void LarzAudioProcessorEditor::drawNode (juce::Graphics& graphics, juce::Point<float> point,
                                         juce::Colour accent, bool highlighted) const
{
    const auto haloRadius = (highlighted ? 12.0f : 9.0f) * scale;
    graphics.setColour (accent.withAlpha (highlighted ? 0.42f : 0.25f));
    graphics.fillEllipse (point.x - haloRadius, point.y - haloRadius, haloRadius * 2.0f, haloRadius * 2.0f);
    graphics.setColour (juce::Colour (0xfff4fbff).withAlpha (0.98f));
    const auto coreRadius = (highlighted ? 4.5f : 3.7f) * scale;
    graphics.fillEllipse (point.x - coreRadius, point.y - coreRadius, coreRadius * 2.0f, coreRadius * 2.0f);
}

void LarzAudioProcessorEditor::drawDragBadge (juce::Graphics& graphics, juce::Point<float> point,
                                              const juce::String& value, juce::Colour accent) const
{
    const auto badgeWidth = 132.0f * scale;
    const auto badgeHeight = 21.0f * scale;
    const auto x = std::clamp (point.x + 13.0f * scale, 4.0f * scale,
                               static_cast<float> (getWidth()) - badgeWidth - 4.0f * scale);
    const auto y = std::clamp (point.y - badgeHeight - 10.0f * scale, 4.0f * scale,
                               static_cast<float> (getHeight()) - badgeHeight - 4.0f * scale);
    const juce::Rectangle<float> badge (x, y, badgeWidth, badgeHeight);
    graphics.setColour (juce::Colour (0xff050517).withAlpha (0.92f));
    graphics.fillRoundedRectangle (badge, 4.0f * scale);
    graphics.setColour (accent.withAlpha (0.62f));
    graphics.drawRoundedRectangle (badge, 4.0f * scale, 0.8f * scale);
    graphics.setColour (juce::Colour (0xffe8f6ff).withAlpha (0.94f));
    graphics.setFont (font (9.0f * scale));
    graphics.drawText (value, badge.toNearestInt().reduced (5, 0), juce::Justification::centredLeft, false);
}
