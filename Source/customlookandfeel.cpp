#include "customlookandfeel.h"

myplug::CustomLookAndFeel::CustomLookAndFeel()
{
	setColour(juce::ScrollBar::ColourIds::thumbColourId, juce::Colours::mediumpurple);
    setColour(juce::Slider::ColourIds::rotarySliderFillColourId, juce::Colours::darkgrey);
    setColour(juce::Slider::ColourIds::trackColourId, juce::Colours::mediumpurple);
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colour(0.0f, 0.0f, 0.2f, 1.0f));
    setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::mediumpurple);
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colour(0.0f, 0.0f, 0.1f, 1.0f));
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, juce::Colour(0.0f, 0.0f, 0.2f, 1.0f));
    setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colour(0.0f, 0.0f, 0.3f, 1.0f));
}

void myplug::CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto Pi = juce::MathConstants<float>::pi;
    float thickness = 5.0f;
    float arcRad = Pi / 3.0f;

    // background
    g.setColour(findColour(juce::Slider::ColourIds::rotarySliderFillColourId));
    juce::Path bgPath;
    juce::PathStrokeType stroke(thickness, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
    bgPath.addCentredArc(width / 2, height / 2, width / 2 - thickness / 2, height / 2 - thickness / 2, 0.0, rotaryStartAngle, rotaryEndAngle, true);
    g.strokePath(bgPath, stroke);

    // foreground
    g.setColour(findColour(juce::Slider::ColourIds::trackColourId));
    juce::Path fgPath;
    fgPath.addCentredArc(width / 2, height / 2, width / 2 - thickness / 2, height / 2 - thickness / 2, 0.0, rotaryStartAngle, angle, true);
    g.strokePath(fgPath, stroke);

    // draw text
    float fontheight = g.getCurrentFont().getHeight();
    g.setColour(findColour(juce::Slider::ColourIds::trackColourId).brighter());
    g.drawText(slider.getTextFromValue(slider.getValue()), 0, height / 2 - fontheight / 2, width, fontheight, juce::Justification::centred);
}
