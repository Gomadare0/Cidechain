#include "Envelope_Editor.h"

myplug::envelope::EnvelopeEditor::EnvelopeEditor()
	: pointEditScroll_(false)
	, pointEdit_(envManager_)
{
	pointEdit_.setFramesPerSecond(60);
	pointEdit_.setCallbackWhenShownRangeChanged([&]() {this->updateScrollBarRange(); });
	pointEdit_.getGridDataRef().verticalGridSubdiv = 4;
	pointEdit_.getGridDataRef().horizontalGridInterval = 0.25;
	pointEditScroll_.setAutoHide(false);
	showgridButton_.setButtonText("Show Grid");
	showgridButton_.setToggleState(true, juce::NotificationType::sendNotification);
	snapgridButton_.setButtonText("Snap to Grid");
	snapgridButton_.setToggleState(true, juce::NotificationType::sendNotification);

	label_griddiv_.setText("Grid Division", juce::NotificationType::dontSendNotification);
	spin_griddivY_.setPrefix("Y ");
	spin_griddivY_.setNumber(4);
	spin_griddivY_.setMinimum(1);
	spin_griddivY_.setMaximum(16);
	spin_griddivY_.addListener(this);
	spin_griddivX_.setPrefix("X ");
	spin_griddivX_.setNumber(4);
	spin_griddivX_.setMinimum(1);
	spin_griddivX_.setMaximum(16);
	spin_griddivX_.addListener(this);

	showgridButton_.addListener(this);
	snapgridButton_.addListener(this);

	addAndMakeVisible(pointEdit_);
	addAndMakeVisible(pointEditScroll_);
	addAndMakeVisible(showgridButton_);
	addAndMakeVisible(snapgridButton_);
	addAndMakeVisible(label_griddiv_);
	addAndMakeVisible(spin_griddivY_);
	addAndMakeVisible(spin_griddivX_);

	pointEditScroll_.addListener(this);
}

myplug::envelope::EnvelopeEditor::~EnvelopeEditor()
{
	showgridButton_.removeListener(this);
	snapgridButton_.removeListener(this);
	pointEditScroll_.removeListener(this);
}

void myplug::envelope::EnvelopeEditor::paint(juce::Graphics& g)
{
	g.setColour(juce::Colour(0.0f, 0.0f, 0.1f, 1.0f));
	g.fillAll();
}

void myplug::envelope::EnvelopeEditor::resized()
{
	// Set PointEdit Bounds
	const int border = 10;
	const int margin = 5;
	auto pointEditBounds = getLocalBounds().reduced(border, border).withTrimmedBottom(50);
	pointEdit_.setBounds(pointEditBounds);
	pointEditScroll_.setBounds(pointEditBounds.withPosition(pointEditBounds.getX(), pointEditBounds.getHeight() + pointEditBounds.getY() + margin).withHeight(10));

	// Buttons
	double footerYPos = getLocalBounds().getHeight() - 32;
	showgridButton_.setSize(100,20);
	showgridButton_.setTopLeftPosition(border, footerYPos);
	snapgridButton_.setSize(100, 20);
	snapgridButton_.setTopLeftPosition(border + 100, footerYPos);

	label_griddiv_.setBounds(border + 250, footerYPos, 100, 20);
	spin_griddivY_.setBounds(border + 350, footerYPos, 50, 20);
	spin_griddivX_.setBounds(border + 410, footerYPos, 50, 20);

	// Adjust ScrollBar Range
	updateScrollBarRange();
}

void myplug::envelope::EnvelopeEditor::updateScrollBarRange()
{
	if (envManager_ != nullptr)
	{
		if (envManager_->getEnvGeneratorPtr() == nullptr) return;

		auto pointsPtr = envManager_->getEnvGeneratorPtr();
		if (pointsPtr->getNumPoint() <= 1)
		{
			pointEditScroll_.setRangeLimits(0.0, 1.0);
			pointEditScroll_.setCurrentRange(0.0, 1.0);
		}
		else
		{
			pointEditScroll_.setRangeLimits(0.0, pointEdit_.getZoomedWidth());
			pointEditScroll_.setCurrentRange(pointEdit_.getOffsetX(), pointEdit_.getLocalBounds().getWidth());
		}
	}
}

void myplug::envelope::EnvelopeEditor::scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	pointEdit_.setOffsetX(newRangeStart);
}

void myplug::envelope::EnvelopeEditor::buttonClicked(juce::Button* button)
{
	if (button == &showgridButton_)
	{
		if (showgridButton_.getToggleState())
		{
			pointEdit_.getGridDataRef().isHorizontalGridVisible = true;
			pointEdit_.getGridDataRef().isVerticalGridVisible = true;
		}
		else
		{
			pointEdit_.getGridDataRef().isHorizontalGridVisible = false;
			pointEdit_.getGridDataRef().isVerticalGridVisible = false;
		}
	}
	else if (button == &snapgridButton_)
	{
		if (snapgridButton_.getToggleState())
		{
			pointEdit_.setSnap(true);
		}
		else
		{
			pointEdit_.setSnap(false);
		}
	}
}

void myplug::envelope::EnvelopeEditor::onNumberChanged(NumericSpinBox* spinbox)
{
	if (spinbox == &spin_griddivY_)
	{
		pointEdit_.getGridDataRef().verticalGridSubdiv = spin_griddivY_.getCurrentNumber();
	}
	else if (spinbox == &spin_griddivX_)
	{
		pointEdit_.getGridDataRef().horizontalGridInterval = 1.0 / spin_griddivX_.getCurrentNumber();
	}
}
