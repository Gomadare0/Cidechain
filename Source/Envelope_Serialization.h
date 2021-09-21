#pragma once

#include <JuceHeader.h>
#include "Envelope_Data.h"

namespace myplug::envelope
{
	juce::ValueTree serializeEnvGen(EnvelopeGenerator& envgen)
	{
		using namespace juce;
		ValueTree tree("EnvelopeGenerator");
		
		for (size_t i = 0; i < envgen.getNumPoint(); ++i)
		{
			ValueTree child("Point");
			const auto& point = envgen.getPoint(i);
			child.setProperty("XPos", point.x, nullptr);
			child.setProperty("YPos", point.y, nullptr);
			child.setProperty("Loop", static_cast<int>(point.loop), nullptr);
			child.setProperty("Interpolation", static_cast<int>(point.interpolation), nullptr);
			child.setProperty("Param1", point.param1, nullptr);

			tree.addChild(child, i, nullptr);
		}

		return tree;
	}

	void deserializeEnvGen(EnvelopeGenerator& envgen, const juce::ValueTree& tree)
	{
		using namespace juce;

		envgen.clearPoints();
		for (size_t i = 0; i < tree.getNumChildren(); ++i)
		{
			Point point;
			auto child = tree.getChild(i);
			point.x = child.getPropertyAsValue("XPos", nullptr).getValue();
			point.y = child.getPropertyAsValue("YPos", nullptr).getValue();
			point.loop = static_cast<LoopState>(int(child.getPropertyAsValue("Loop", nullptr).getValue()));
			point.interpolation = static_cast<InterpolationType>(int(child.getPropertyAsValue("Interpolation", nullptr).getValue()));
			point.param1 = child.getPropertyAsValue("Param1", nullptr).getValue();

			envgen.addPoint(point.x, point.y, point.interpolation, point.param1, point.loop);
		}
	}
}