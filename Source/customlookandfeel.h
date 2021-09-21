#pragma once

#include <JuceHeader.h>

namespace myplug
{
	class CustomLookAndFeel : public juce::LookAndFeel_V4
	{
	public:
		CustomLookAndFeel();
		void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
	};
}