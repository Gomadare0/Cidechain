#include "Envelope_PreviewComponent.h"

void myplug::envelope::EnvelopePreviewComponent::paint(juce::Graphics& g)
{
	g.setColour(juce::Colour(0.0f, 0.0f, isMouseOver() ? 0.2f : 0.15f, 1.0f));
	g.fillAll();

	if (envgen_ != nullptr)
	{
		double linethickness = 1.0;

		double zoomRate = getLocalBounds().getWidth() / envgen_->getXRange();
		auto calcScreenXPos = [&](double x) -> double { return x * zoomRate; };
		auto calcScreenYPos = [&](double y) -> double { return y * getLocalBounds().getHeight() * (-1.0) + getLocalBounds().getHeight(); };
		envelope::Point previousPoint;

		auto pointsPtr = envgen_;

		for (int b = 3; b >= 0; --b)
		{
			if (b == 0)
			{
				pointsPtr = envgen_;
				linethickness = 1.5;
				g.setColour(juce::Colours::white);
			}
			else
			{
				pointsPtr = envgenMB_[b-1];
				linethickness = 0.5;
				switch (b)
				{
				case 1:
					g.setColour(juce::Colour(82, 125, 255)); break;
				case 2:
					g.setColour(juce::Colour(168, 111, 206)); break;
				case 3:
					g.setColour(juce::Colour(252, 86, 156)); break;
				default:
					break;
				}
			}
			previousPoint = Point();

			for (int k = 0; k < pointsPtr->getNumPoint(); ++k)
			{
				auto& i = pointsPtr->getPoint(k);

				// Draw interpolated lines
				switch (previousPoint.interpolation)
				{
				case InterpolationType::Linear:
					g.drawLine(
						calcScreenXPos(previousPoint.x),
						calcScreenYPos(previousPoint.y),
						calcScreenXPos(i.x),
						calcScreenYPos(i.y),
						linethickness);
					break;
				case InterpolationType::Sigmoid:
				{
					double plotInterval = 3 / zoomRate;
					double length = i.x - previousPoint.x;
					double prevX = previousPoint.x;
					double nextX = std::clamp(prevX + plotInterval, 0.0, i.x);

					// when previousPoint.x == i.x, getInterpolatedValue(prevX) and getInterpolatedValue(nextX) will return the same value.
					if (previousPoint.x == i.x)
					{
						g.drawLine(
							calcScreenXPos(previousPoint.x),
							calcScreenYPos(previousPoint.y),
							calcScreenXPos(i.x),
							calcScreenYPos(i.y),
							linethickness);
					}
					else
					{
						juce::Path stroke;
						juce::PathStrokeType strokeType(linethickness, juce::PathStrokeType::JointStyle::curved);
						stroke.startNewSubPath(calcScreenXPos(prevX), calcScreenYPos(pointsPtr->getInterpolatedValue(prevX)));

						while (length >= 0)
						{
							stroke.lineTo(calcScreenXPos(nextX), calcScreenYPos(pointsPtr->getInterpolatedValue(nextX)));

							nextX = std::clamp(nextX + plotInterval, 0.0, i.x - 0.0000001);

							length = length - plotInterval;
						}
						g.strokePath(stroke, strokeType);
					}
				}
				break;
				case InterpolationType::Step:
					g.drawLine(
						calcScreenXPos(previousPoint.x),
						calcScreenYPos(previousPoint.y),
						calcScreenXPos(i.x),
						calcScreenYPos(previousPoint.y),
						linethickness);
					g.drawLine(
						calcScreenXPos(i.x),
						calcScreenYPos(previousPoint.y),
						calcScreenXPos(i.x),
						calcScreenYPos(i.y),
						linethickness);
					break;
				default:
					break;
				}

				previousPoint = i;
			}
		}
	}
}

void myplug::envelope::EnvelopePreviewComponent::mouseDown(const juce::MouseEvent& e)
{
	callListeners();
}

void myplug::envelope::EnvelopePreviewComponent::mouseMove(const juce::MouseEvent& e)
{
	repaint();
}

void myplug::envelope::EnvelopePreviewComponent::mouseExit(const juce::MouseEvent& e)
{
	repaint();
}
