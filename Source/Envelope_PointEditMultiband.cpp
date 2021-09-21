
#include "Envelope_PointEditMultiband.h"
#include "Envelope_VoiceController.h"

//
// PointEditMB Component
//

double myplug::envelope::PointEditMB::calcScreenXPos(double x)
{
	return x * zoomRate_ - offsetX_;
}

double myplug::envelope::PointEditMB::calcEnvGenXPos(double x)
{
	return (x + offsetX_) / zoomRate_;
}

double myplug::envelope::PointEditMB::calcScreenYPos(double y)
{
	return y * getLocalBounds().getHeight() * (-1.0) + getLocalBounds().getHeight();
}

double myplug::envelope::PointEditMB::calcEnvGenYPos(double y)
{
	return (-y + getLocalBounds().getHeight()) / getLocalBounds().getHeight();
}

void myplug::envelope::PointEditMB::setZoomRateToWholeRange()
{
	if (envManager_ == nullptr) return;
	if (envManager_->getEnvGeneratorPtr() == nullptr) return;

	offsetX_ = 0.0;
	zoomRate_ = getLocalBounds().getWidth() / envManager_->getEnvGeneratorPtr()->getXRange();
	baseZoomRate_ = zoomRate_;
}

double myplug::envelope::PointEditMB::getZoomedWidth()
{
	if (envManager_ == nullptr) return 0.0;
	if (envManager_->getEnvGeneratorPtr() == nullptr) return 0.0;

	return zoomRate_ * envManager_->getEnvGeneratorPtr()->getLastPoint().x;
}


void myplug::envelope::PointEditMB::updateHandleIntersectionInfo(const juce::MouseEvent& e)
{
	mouseIntersectedBranchIndex_ = -1;
	mouseIntersectedHandleIndex_ = -1;

	// Handle intersection test
	auto envGen = envManager_->getEnvGeneratorPtr();
	if (!envGen)
	{
		return;
	}
	if (isMultiband_)
	{
		envGen = envManager_->getSpecificEnvGeneratorPtrFromName(editingBand_);
	}

	const auto& localBounds = getLocalBounds();

	for (int k = 0; k < envGen->getNumPoint(); ++k)
	{
		auto& i = envGen->getPoint(k);
		juce::Path circle;
		circle.addEllipse(
			calcScreenXPos(i.x) - circleRadius_,
			calcScreenYPos(i.y) - circleRadius_,
			circleRadius_ * 2.0,
			circleRadius_ * 2.0);
		if (circle.contains(e.x, e.y))
		{
			mouseIntersectedHandleIndex_ = k;
			break;
		}
	}

	// Branch Intersection Test
	const double heighttolerance = 20.0;
	auto yPos = calcScreenYPos(envGen->getCachedInterpolatitonValue(calcEnvGenXPos(e.position.x)));
	if (abs(yPos - e.position.y) <= heighttolerance && mouseIntersectedHandleIndex_ < 0)
	{
		mouseIntersectedBranchIndex_ = envGen->getLeftPointIndex(calcEnvGenXPos(e.position.x));
	}

	// Bounds Intersection Test
	if (getLocalBounds().contains(e.position.toInt()))
	{
		isMouseOnBounds_ = true;
	}
	else
	{
		isMouseOnBounds_ = false;
	}
}

void myplug::envelope::PointEditMB::updateGridIntersectionInfo(const juce::MouseEvent& e)
{
	mouseIntersectedHGridIndex_ = -1;
	mouseIntersectedVGridIndex_ = -1;

	const auto& localBounds = getLocalBounds();

	// Grid Intersection Test
	if (snapToGrid_)
	{
		// Horizontally
		if (grid_.isHorizontalGridVisible || grid_.isHorizontalSubGridVisible)
		{
			double xInterval = grid_.horizontalGridInterval;
			double maxWidth = std::max<double>(getZoomedWidth(), localBounds.getWidth());
			if (grid_.isHorizontalSubGridVisible)
			{
				xInterval /= grid_.horizontalSubGridSubdiv;
			}
			for (int i = 0; i <= maxWidth / xInterval / zoomRate_; ++i)
			{
				if (abs(e.x - calcScreenXPos(xInterval * i)) <= snapDistance_)
				{
					mouseIntersectedHGridIndex_ = i;
				}
			}
		}

		// Vertically
		if (grid_.isVerticalGridVisible || grid_.isVerticalSubGridVisible)
		{
			double yInterval = 1.0 * localBounds.getHeight() / grid_.verticalGridSubdiv;
			if (grid_.isVerticalSubGridVisible)
			{
				yInterval /= grid_.verticalSubGridSubdiv;
			}
			for (int i = 0; i <= localBounds.getHeight() / yInterval; ++i)
			{
				if (abs(e.y - (yInterval * i)) <= snapDistance_)
				{
					mouseIntersectedVGridIndex_ = i;
				}
			}
		}
	}
}

myplug::envelope::PointEditMB::PointEditMB(EnvelopeManager* envManager) : envManager_(envManager)
{
}

void myplug::envelope::PointEditMB::paint(juce::Graphics& g)
{
	g.setColour(juce::Colour(0.0f, 0.0f, 0.15f, 1.0f));
	g.fillAll();

	// Debugging
	/*
	g.setColour(juce::Colours::white);
	juce::String str;
	str << "HGrid:" << mouseIntersectedHGridIndex_ << "\nVGrid:" << mouseIntersectedVGridIndex_;
	g.drawText(str, 0.0, 0.0, 300.0, 10.0, juce::Justification::topLeft);
	*/

	if (envManager_ == nullptr)
	{
		g.setColour(juce::Colours::white);
		juce::String text_nothing{ "There's nothing to draw." };
		juce::Rectangle<int> textArea{ g.getCurrentFont().getStringWidth(text_nothing), 10 };
		textArea.setPosition(getLocalBounds().getCentreX() - textArea.getWidth() / 2, getLocalBounds().getCentreY() - textArea.getHeight() / 2);
		g.drawText(text_nothing, textArea, juce::Justification::centredLeft);

	}
	else
	{
		auto envgen = envManager_->getEnvGeneratorPtr();
		const auto& localBounds = getLocalBounds();
		double previousXPosition = 0.0;
		envelope::Point initPoint;
		envelope::Point& previousPoint = initPoint;
		int index = 0;

		// Draw Grids
		if (grid_.isHorizontalGridVisible)
		{
			g.setColour(juce::Colour(0.0f, 0.0f, 0.3f, 1.0f));
			double maxWidth = std::max<double>(getZoomedWidth(), localBounds.getWidth());
			for (int w = 0; w < maxWidth / grid_.horizontalGridInterval / zoomRate_; ++w)
			{
				double xPos = w * grid_.horizontalGridInterval;
				g.drawLine(calcScreenXPos(xPos), 0.0, calcScreenXPos(xPos), localBounds.getHeight(), 1.0f);
			}
		}
		if (grid_.isHorizontalSubGridVisible)
		{
			g.setColour(juce::Colour(0.0f, 0.0f, 0.20f, 1.0f));
			double interval = grid_.horizontalGridInterval / grid_.horizontalSubGridSubdiv;
			double maxWidth = std::max<double>(getZoomedWidth(), localBounds.getWidth());
			for (int w = 0; w < maxWidth / interval / zoomRate_; ++w)
			{
				double xPos = w * interval;
				g.drawLine(calcScreenXPos(xPos), 0.0, calcScreenXPos(xPos), localBounds.getHeight(), 1.0f);
			}
		}
		if (grid_.isVerticalGridVisible)
		{
			g.setColour(juce::Colour(0.0f, 0.0f, 0.3f, 1.0f));
			double interval = 1.0 * localBounds.getHeight() / grid_.verticalGridSubdiv;
			for (int h = 0; h < grid_.verticalGridSubdiv - 1; ++h)
			{
				double yPos = h * interval + interval;
				g.drawLine(0.0, yPos, localBounds.getWidth(), yPos, 1.0f);
			}
		}
		if (grid_.isVerticalSubGridVisible)
		{
			g.setColour(juce::Colour(0.0f, 0.0f, 0.20f, 1.0f));
			double interval = 1.0 * localBounds.getHeight() / grid_.verticalSubGridSubdiv / grid_.verticalGridSubdiv;
			for (int h = 0; h < (grid_.verticalGridSubdiv * grid_.verticalSubGridSubdiv) - 1; ++h)
			{
				double yPos = h * interval + interval;
				g.drawLine(0.0, yPos, localBounds.getWidth(), yPos, 1.0f);
			}
		}

		// Draw Grid Units		
		{
			const int border = 5;
			const int textHeight = 10;
			if (grid_.isHorizontalGridVisible)
			{
				g.setColour(juce::Colour(0.0f, 0.0f, 0.5f, 1.0f));
				double maxWidth = std::max<double>(getZoomedWidth(), localBounds.getWidth());
				for (int w = 0; w < maxWidth / grid_.horizontalGridInterval / zoomRate_; ++w)
				{
					double xPos = w * grid_.horizontalGridInterval;
					g.drawText(
						grid_.horizontalLabel(w),
						calcScreenXPos(xPos) + border + (w == 0 ? 10 : 0),
						localBounds.getHeight() - border - textHeight,
						grid_.horizontalGridInterval * zoomRate_ - border * 2.0,
						textHeight,
						juce::Justification::topLeft);
				}
			}
			if (grid_.isVerticalGridVisible)
			{
				g.setColour(juce::Colour(0.0f, 0.0f, 0.5f, 1.0f));
				double interval = 1.0 * localBounds.getHeight() / grid_.verticalGridSubdiv;
				int gridCount = grid_.verticalGridSubdiv;
				if (grid_.isVerticalSubGridVisible)
				{
					gridCount *= grid_.verticalSubGridSubdiv;
					interval /= grid_.verticalSubGridSubdiv;
				}
				for (int h = 0; h < gridCount; ++h)
				{
					double yPos = (gridCount - h) * interval;
					g.drawText(
						grid_.verticalLabel(h),
						border,
						yPos - textHeight - border - (h == 0 ? 10 : 0),
						200.0,
						textHeight,
						juce::Justification::topLeft);
				}
			}
		}

		// Draw VoicePos Indicator
		g.setColour(juce::Colours::grey);
		for (const auto& i : envManager_->getVoiceControllerRef())
		{
			g.drawLine(
				calcScreenXPos(i->getXPos()),
				0.0,
				calcScreenXPos(i->getXPos()),
				getHeight(),
				2.0f
			);
		}

		if (envgen)
		{
			auto pointsPtr = envgen;

			for (int band = 0; band < (isMultiband_ ? 3 : 1); ++band)
			{
				previousPoint = initPoint;
				index = 0;
				previousXPosition = 0.0;
				std::string currentBand = "";
				if (isMultiband_)
				{
					switch (band)
					{
					case 0:
						currentBand = "low";
						//points = &envManager_->getSpecificEnvGeneratorPtrFromName("low")->points;
						pointsPtr = envManager_->getSpecificEnvGeneratorPtrFromName("low");
						envgen = envManager_->getSpecificEnvGeneratorPtrFromName("low");
						break;
					case 1:
						currentBand = "mid";
						pointsPtr = envManager_->getSpecificEnvGeneratorPtrFromName("mid");
						envgen = envManager_->getSpecificEnvGeneratorPtrFromName("mid");
						break;
					case 2:
						currentBand = "high";
						pointsPtr = envManager_->getSpecificEnvGeneratorPtrFromName("high");
						envgen = envManager_->getSpecificEnvGeneratorPtrFromName("high");
						break;
					}
				}


				for (int k = 0 ; k < pointsPtr->getNumPoint(); ++k)
				{
					auto& i = pointsPtr->getPoint(k);
					if (isMultiband_)
					{
						switch (band)
						{
						case 0:
							g.setColour(juce::Colour(82, 125, 255)); break;
						case 1:
							g.setColour(juce::Colour(168, 111, 206)); break;
						case 2:
							g.setColour(juce::Colour(252, 86, 156)); break;
						}
					}
					else
					{
						g.setColour(juce::Colours::white);
					}

					// Draw interpolated lines

					// Skip when segments and a branch is out of the bounds.
					if (calcScreenXPos(i.x) < localBounds.getX() || calcScreenXPos(previousPoint.x) > localBounds.getWidth())
					{
						previousXPosition = i.x * zoomRate_;
						previousPoint = i;
						index++;
						continue;
					}

					bool thicken = false;
					if (isMultiband_)
					{
						if (currentBand == editingBand_) thicken = (index - 1) == mouseIntersectedBranchIndex_;
					}
					else
					{
						thicken = (index - 1) == mouseIntersectedBranchIndex_;
					}
					double linethickness = thicken ? 3.0 : 1.0;

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
						double plotInterval = 3 / zoomRate_;
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
							stroke.startNewSubPath(calcScreenXPos(prevX), calcScreenYPos(envgen->getInterpolatedValue(prevX)));

							while (length >= 0)
							{
								stroke.lineTo(calcScreenXPos(nextX), calcScreenYPos(envgen->getInterpolatedValue(nextX)));

								//prevX = nextX;
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

					// Draw segments
					bool drawSeg = true;
					if (isMouseOnBounds_)
					{
						if (isMultiband_)
						{
							if (currentBand != editingBand_)
							{
								drawSeg = false;
							}
						}

						if (drawSeg)
						{
							g.setColour(juce::Colours::white);
							g.drawEllipse(
								calcScreenXPos(i.x) - circleRadius_,
								calcScreenYPos(i.y) - circleRadius_,
								circleRadius_ * 2.0,
								circleRadius_ * 2.0,
								1.0);

							if (index == mouseIntersectedHandleIndex_)
							{
								g.setColour(juce::Colours::mediumpurple);
								g.fillEllipse(
									calcScreenXPos(i.x) - circleRadius_,
									calcScreenYPos(i.y) - circleRadius_,
									circleRadius_ * 2.0,
									circleRadius_ * 2.0
								);
							}
						}
					}

					previousXPosition = i.x * zoomRate_;
					previousPoint = i;
					index++;
				}
			}
		}		
	}
}

void myplug::envelope::PointEditMB::update()
{
	this->repaint();

	// Change Mouse Cursor
	if (mouseIntersectedHandleIndex_ >= 0)
	{
		setMouseCursor(juce::MouseCursor::PointingHandCursor);
	}
	else if (mouseIntersectedBranchIndex_ >= 0)
	{
		setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
	}
	else
	{
		setMouseCursor(juce::MouseCursor::NormalCursor);
	}
}

void myplug::envelope::PointEditMB::resized()
{

}

void myplug::envelope::PointEditMB::mouseMove(const juce::MouseEvent& e)
{
	updateHandleIntersectionInfo(e);
	updateGridIntersectionInfo(e);
	prevMousePos = e.position;
}

void myplug::envelope::PointEditMB::mouseDown(const juce::MouseEvent& e)
{
	if (e.source.getCurrentModifiers().isRightButtonDown())
	{
		if (mouseIntersectedHandleIndex_ >= 0)
		{
			juce::PopupMenu m;
			juce::PopupMenu::Options option;
			auto pointsPtr = envManager_->getEnvGeneratorPtr();
			if (isMultiband_)
			{
				pointsPtr = envManager_->getSpecificEnvGeneratorPtrFromName(editingBand_);
			}

			m.addSectionHeader("Shape");
			m.addItem(1, "Linear");
			m.addItem(2, "Step");
			m.addItem(3, "Curve");

			if ((mouseIntersectedHandleIndex_ > 0 &&mouseIntersectedHandleIndex_ < pointsPtr->getNumPoint() - 1) || (mouseIntersectedHandleIndex_ == pointsPtr->getNumPoint() - 1 && isEndPointMovableHorizontally == true))
			{
				m.addSeparator();
				m.addItem(4, "Delete Point");
			}
			m.showMenuAsync(option, [&](int i)
				{
					auto pointsPtr = envManager_->getEnvGeneratorPtr();
					if (isMultiband_)
					{
						pointsPtr = envManager_->getSpecificEnvGeneratorPtrFromName(editingBand_);
					}

					if (mouseIntersectedHandleIndex_ < 0) { return; }
					switch (i)
					{
					case 1:
						pointsPtr->getPoint(mouseIntersectedHandleIndex_).interpolation = envelope::InterpolationType::Linear;
						break;
					case 2:
						pointsPtr->getPoint(mouseIntersectedHandleIndex_).interpolation = envelope::InterpolationType::Step;
						break;
					case 3:
						pointsPtr->getPoint(mouseIntersectedHandleIndex_).interpolation = envelope::InterpolationType::Sigmoid;
						break;
					case 4:
						pointsPtr->removePoint(mouseIntersectedHandleIndex_);
						pointsPtr->render();
					default:
						break;
					}
					callAllListeners();
				});
		}
	}
}

void myplug::envelope::PointEditMB::mouseDrag(const juce::MouseEvent& e)
{
	auto pointsPtr = envManager_->getEnvGeneratorPtr();
	if (isMultiband_)
	{
		pointsPtr = envManager_->getSpecificEnvGeneratorPtrFromName(editingBand_);
	}

	// LMB is Down
	if (e.source.getCurrentModifiers().isLeftButtonDown())
	{
		// Move Handles
		// X
		if (mouseIntersectedHandleIndex_ >= 0)
		{
			if (0 < mouseIntersectedHandleIndex_ && mouseIntersectedHandleIndex_ < pointsPtr->getNumPoint()-1)
			{
				// Clamp or move the specified handle
				auto& prevPointPosX = pointsPtr->getPoint(mouseIntersectedHandleIndex_-1).x;
				auto& midPointPosX = pointsPtr->getPoint(mouseIntersectedHandleIndex_).x;
				auto& nextPointPosX = pointsPtr->getPoint(mouseIntersectedHandleIndex_+1).x;

				if (calcScreenXPos(prevPointPosX) < e.x && e.x < calcScreenXPos(nextPointPosX))
				{
					if (mouseIntersectedHGridIndex_ >= 0)
					{
						// Snap
						double xInterval = grid_.horizontalGridInterval;
						if (grid_.isHorizontalSubGridVisible)
						{
							xInterval /= grid_.horizontalSubGridSubdiv;
						}

						midPointPosX = xInterval * mouseIntersectedHGridIndex_;
					}
					else
					{
						midPointPosX = calcEnvGenXPos(e.x);
					}
				}
				else if (calcScreenXPos(prevPointPosX) >= e.x)
				{
					midPointPosX = prevPointPosX;
				}
				else if (calcScreenXPos(nextPointPosX) <= e.x)
				{
					midPointPosX = nextPointPosX;
				}
			}
			else if (mouseIntersectedHandleIndex_ == 0)
			{

			}
			else if (mouseIntersectedHandleIndex_ == pointsPtr->getNumPoint() - 1)
			{
				auto& prevPointPosX = pointsPtr->getPoint(mouseIntersectedHandleIndex_ - 1).x;
				auto& lastPointPosX = pointsPtr->getPoint(mouseIntersectedHandleIndex_    ).x;

				if (isEndPointMovableHorizontally)
				{
					if (calcScreenXPos(prevPointPosX) < e.x)
					{

						lastPointPosX = calcEnvGenXPos(e.x);

					}
					else if (calcScreenXPos(prevPointPosX) >= e.x)
					{
						lastPointPosX = prevPointPosX;
					}
				}
			}
		}

		// Y
		if (mouseIntersectedHandleIndex_ >= 0)
		{
			if (mouseIntersectedVGridIndex_ >= 0)
			{
				// Snap
				double yInterval = 1.0 * getLocalBounds().getHeight() / grid_.verticalGridSubdiv;
				if (grid_.isHorizontalSubGridVisible)
				{
					yInterval /= grid_.verticalSubGridSubdiv;
				}

				pointsPtr->getPoint(mouseIntersectedHandleIndex_).y = calcEnvGenYPos(yInterval * mouseIntersectedVGridIndex_);
			}
			else
			{
				pointsPtr->getPoint(mouseIntersectedHandleIndex_).y = calcEnvGenYPos(std::clamp<double>(e.y, 0.0, getLocalBounds().getHeight()));
			}
		}

		// Move Braches
		if (mouseIntersectedBranchIndex_ >= 0 && mouseIntersectedBranchIndex_ <= pointsPtr->getNumPoint() - 2)
		{
			auto deltaPos = e.position - prevMousePos;
			auto envgen = envManager_->getEnvGeneratorPtr();
			if (isMultiband_)
			{
				envgen = envManager_->getSpecificEnvGeneratorPtrFromName(editingBand_);
			}
			auto& point = pointsPtr->getPoint(mouseIntersectedBranchIndex_);
			const double ySensitivity = 0.05 * pow(1.05, abs(point.param1));
			double direction = -1.0;

			if (pointsPtr->getPoint(mouseIntersectedBranchIndex_).y > pointsPtr->getPoint(mouseIntersectedBranchIndex_+1).y)
				direction = 1.0;

			point.param1 = std::clamp(point.param1 + direction * deltaPos.y * ySensitivity, -100.0, 100.0);
		}

	}
	// MMB is Down
	else if (e.source.getCurrentModifiers().isMiddleButtonDown())
	{

	}

	pointsPtr->render();
	updateGridIntersectionInfo(e);
	prevMousePos = e.position;
}

void myplug::envelope::PointEditMB::mouseUp(const juce::MouseEvent& e)
{
	updateHandleIntersectionInfo(e);
	updateGridIntersectionInfo(e);
	callbackWhenShownRangeChanged_();
	callAllListeners();
}

void myplug::envelope::PointEditMB::mouseExit(const juce::MouseEvent& e)
{
	updateHandleIntersectionInfo(e);
	updateGridIntersectionInfo(e);
}

void myplug::envelope::PointEditMB::mouseDoubleClick(const juce::MouseEvent& e)
{
	if (e.mods.isLeftButtonDown())
	{
		auto envgen = envManager_->getEnvGeneratorPtr();
		if (isMultiband_)
		{
			envgen = envManager_->getSpecificEnvGeneratorPtrFromName(editingBand_);
		}

		if (mouseIntersectedHandleIndex_ > 0)
		{
			if (mouseIntersectedHandleIndex_ != envgen->getNumPoint() - 1 || isEndPointMovableHorizontally != false)
			{
				envgen->removePoint(mouseIntersectedHandleIndex_);
			}
		}
		else
		{
			envgen->addPoint(calcEnvGenXPos(e.x), calcEnvGenYPos(e.y), envelope::InterpolationType::Sigmoid);
		}
		envgen->render();
		callAllListeners();
	}
}

void myplug::envelope::PointEditMB::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
	zoomRate_ = std::clamp<double>(
		zoomRate_ + zoomRate_ * wheel.deltaY,
		canShrinkBeyondEndPoint_ ? baseZoomRate_ / zoomRateRange_ : baseZoomRate_,
		baseZoomRate_ * zoomRateRange_);
	callbackWhenShownRangeChanged_();
}

void myplug::envelope::PointEditMB::callAllListeners()
{
	for (const auto& i : listener_)
	{
		i->handleMoved(this);
	}
}
