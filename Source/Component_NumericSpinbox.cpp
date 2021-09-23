#include "Component_NumericSpinbox.h"

void myplug::NumericSpinBox::updateMouseCur(const juce::MouseEvent& e)
{
	if (getLocalBounds().contains(e.x, e.y))
	{
		setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
	}
	else
	{
		setMouseCursor(juce::MouseCursor::NormalCursor);
	}
}

void myplug::NumericSpinBox::updateNumFromTextinput()
{
	if (textinput_.getText() != "")
	{
		if (useCustomParser_)
		{
			setNumber(customParser_(textinput_.getText()));
		}
		else
		{
			if (isInteger_)
			{
				setNumber(textinput_.getText().getIntValue());
			}
			else
			{
				setNumber(textinput_.getText().getDoubleValue());
			}
		}
		for (const auto& i : listeners_)
		{
			i->onNumberChanged(this);
			i->startGesture(this);
			i->endGesture(this);
		}
	}

	textinput_.setVisible(false);
	textinput_.setText("");
}

myplug::NumericSpinBox::NumericSpinBox()
	: globalMouseListener_(*this)
{
	textinput_.addListener(this);
	addChildComponent(textinput_);

	juce::Desktop::getInstance().addGlobalMouseListener(&globalMouseListener_);
}

myplug::NumericSpinBox::~NumericSpinBox()
{
	textinput_.removeListener(this);
	juce::Desktop::getInstance().removeGlobalMouseListener(&globalMouseListener_);
}

void myplug::NumericSpinBox::paint(juce::Graphics& g)
{
	g.setColour(isMouseOver() ? colour_background_mouseover_ : colour_background_);
	g.fillAll();

	g.setColour(colour_border_);
	g.drawRect(getLocalBounds(), 1.0f);

	float border = getLocalBounds().getHeight() * 0.1f;
	float textHeight = getLocalBounds().getHeight() - border * 2;
	float textMaxWidth = getLocalBounds().getWidth() - border * 2;
	g.setColour(colour_font_);
	auto font = g.getCurrentFont();
	font.setHeight(textHeight);
	g.setFont(font);

	if (useValueList_)
	{
		size_t index = static_cast<size_t>((num_ - min_) / (max_ - min_) * (valueList_.size() - 1));
		g.drawText(prefix_ + valueList_[index] + suffix_, border, border, textMaxWidth, textHeight, juce::Justification::centred);
	}
	else
	{
		std::ostringstream numToText;
		numToText << std::fixed << std::setprecision(decimalDigit_) << num_;
		g.drawText(prefix_ + juce::String(numToText.str()) + suffix_, border, border, textMaxWidth, textHeight, juce::Justification::centred);
	}
}

void myplug::NumericSpinBox::resized()
{
	textinput_.setBounds(getLocalBounds());
}

void myplug::NumericSpinBox::setColour(NumericSpinBox::Colours colourType, const juce::Colour& newColour)
{
	switch (colourType)
	{
	case NumericSpinBox::Colours::Background: colour_background_ = newColour; break;
	case NumericSpinBox::Colours::BackgroundMouseover: colour_background_mouseover_ = newColour; break;
	case NumericSpinBox::Colours::Border: colour_border_ = newColour; break;
	case NumericSpinBox::Colours::Font: colour_font_ = newColour; break;
	default: break;
	}
	repaint();
}

void myplug::NumericSpinBox::mouseDown(const juce::MouseEvent& e)
{
	for (const auto& i : listeners_)
	{
		i->startGesture(this);
	}
}

void myplug::NumericSpinBox::mouseGlobDown(const juce::MouseEvent& e)
{
	if (!getScreenBounds().contains(e.getScreenPosition()) && textinput_.isVisible())
	{
		updateNumFromTextinput();
	}
}

void myplug::NumericSpinBox::mouseDrag(const juce::MouseEvent& e)
{
	auto delta = e.getPosition() - prevMousePos_;
	dy_ += -delta.y / 50.0 * mouseSensitivity_;
	int dyFloor = floor(dy_);
	if (dyFloor >= 1 || dyFloor <= -1)
	{
		num_ = std::clamp<double>(num_ + incrementAmount_ * dyFloor, min_, max_);
		dy_ -= dyFloor;
		for (const auto& i : listeners_)
		{
			i->onNumberChanged(this);
		}
	}

	repaint();
	prevMousePos_ = e.getPosition();
}

void myplug::NumericSpinBox::mouseMove(const juce::MouseEvent& e)
{
	repaint();
	prevMousePos_ = e.getPosition();
	updateMouseCur(e);
}

void myplug::NumericSpinBox::mouseExit(const juce::MouseEvent& e)
{
	repaint();
	updateMouseCur(e);
	for (const auto& i : listeners_)
	{
		i->endGesture(this);
	}
}

void myplug::NumericSpinBox::mouseUp(const juce::MouseEvent& e)
{
	dy_ = 0.0;
}

void myplug::NumericSpinBox::mouseDoubleClick(const juce::MouseEvent& e)
{
	textinput_.setVisible(true);
	textinput_.grabKeyboardFocus();
}

void myplug::NumericSpinBox::textEditorReturnKeyPressed(juce::TextEditor& e)
{
	updateNumFromTextinput();
}
