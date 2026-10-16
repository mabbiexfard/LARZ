#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LarzLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosition, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class LarzFineSlider final : public juce::Slider
{
public:
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    juce::Point<float> dragStart;
    double dragStartValue = 0.0;
};

class LarzAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LarzAudioProcessorEditor (LarzAudioProcessor&);
    ~LarzAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Control
    {
        gain,
        phase,
        root,
        view
    };

    static juce::Colour cyan();
    static juce::Colour orange();
    static juce::Font font (float height, const juce::String& style = "Light", float kerning = 0.0f);

    juce::Point<float> pitchNodePoint (const larz::PitchNode&, float viewMs) const;
    juce::Point<float> ampNodePoint (int index) const;
    int pitchNodeAt (juce::Point<float>) const;
    int ampNodeAt (juce::Point<float>) const;
    float frequencyFromY (float y) const;
    float timeFromX (float x) const;
    void addPitchNode (juce::Point<float>);
    void removePitchNode (int index);
    void updatePitchNode (juce::Point<float>);
    void updateAmpNode (juce::Point<float>);
    void beginAmpGestures (int index);
    void endAmpGestures();
    void setParameterValue (const char* parameterID, float value);
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, juce::Colour) const;
    void drawPitchPanel (juce::Graphics&);
    void drawAmpPanel (juce::Graphics&);
    void drawGrid (juce::Graphics&, juce::Rectangle<float>, juce::Colour, bool pitchGrid);
    void drawNode (juce::Graphics&, juce::Point<float>, juce::Colour, bool highlighted) const;
    void drawDragBadge (juce::Graphics&, juce::Point<float>, const juce::String&, juce::Colour) const;

    LarzAudioProcessor& processor;
    LarzLookAndFeel lookAndFeel;
    juce::Image sourceBackground;
    juce::Image scaledBackground;
    std::array<LarzFineSlider, 4> knobs;
    juce::ToggleButton keyTrackButton;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> knobAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> keyTrackAttachment;
    std::array<juce::RangedAudioParameter*, 2> activeGestures {};
    int activeGestureCount = 0;

    juce::Rectangle<float> pitchPanel;
    juce::Rectangle<float> ampPanel;
    juce::Rectangle<float> pitchPlot;
    juce::Rectangle<float> ampPlot;
    juce::Point<float> dragPosition;
    int selectedPitchNode = -1;
    int hoveredPitchNode = -1;
    int hoveredAmpNode = -1;
    int draggingPitchNode = -1;
    int draggingAmpNode = -1;
    float scale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LarzAudioProcessorEditor)
};
