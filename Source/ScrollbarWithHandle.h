#pragma once

#include <JuceHeader.h>

namespace myplug
{
	class ScrollbarWithHandle : public juce::ScrollBar
	{
	public:
		ScrollbarWithHandle(bool isVertical)
			: juce::ScrollBar(isVertical)
		{

		}
	};
}