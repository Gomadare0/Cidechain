#pragma once

#include <JuceHeader.h>

namespace myplug
{
	class NumericSpinBox : public juce::Component, public juce::TextEditor::Listener
	{	
	public:
		enum class Colours
		{
			Background,
			BackgroundMouseover,
			Border,
			Font
		};

		class Listener
		{
		public:
			virtual ~Listener() = default;
			virtual void onNumberChanged(NumericSpinBox*) = 0;
			virtual void startGesture(NumericSpinBox*){}
			virtual void endGesture(NumericSpinBox*){}
		};

	private:
		class GlobalMouseListener : public juce::MouseListener
		{
			NumericSpinBox& parent_;
		public:
			GlobalMouseListener(NumericSpinBox& component) : parent_(component)
			{

			}
			void mouseDown(const juce::MouseEvent& e) override
			{
				parent_.mouseGlobDown(e);
			}
		} globalMouseListener_;

		std::vector<NumericSpinBox::Listener*> listeners_;
		juce::String prefix_;
		juce::String suffix_;
		std::vector<juce::String> valueList_;
		bool useValueList_ = false;
		std::function<double(const juce::String&)> customParser_;
		bool useCustomParser_ = false;
		double num_ = 0.0;
		double incrementAmount_ = 1.0;
		double max_ = std::numeric_limits<double>::max();
		double min_ = std::numeric_limits<double>::min();
		int decimalDigit_ = 0;
		double dy_ = 0.0;
		bool isInteger_ = true;
		double mouseSensitivity_ = 1.0;

		juce::Point<int> prevMousePos_;

		juce::TextEditor textinput_;

		juce::Colour colour_background_ = juce::Colour(0.0f, 0.0f, 0.2f, 1.0f);;
		juce::Colour colour_background_mouseover_ = juce::Colour(0.0f, 0.0f, 0.3f, 1.0f);
		juce::Colour colour_border_ = juce::Colours::mediumpurple;
		juce::Colour colour_font_ = juce::Colours::white;

		void updateMouseCur(const juce::MouseEvent& g);
		void updateNumFromTextinput();

	public:
		NumericSpinBox();
		~NumericSpinBox();

		void paint(juce::Graphics& g) override;
		void resized() override;

		double getCurrentNumber() { return num_; }
		double getIncrementAmount() { return incrementAmount_; }
		double getMaximum() { return max_; }
		double getMinimum() { return min_; }
		int getDecimalDigit() { return decimalDigit_; }
		juce::String getPrefix() { return prefix_; }
		juce::String getSuffix() { return suffix_; }
		const std::vector<juce::String>& getValueList() { return valueList_; }

		void useValueList() { useValueList_ = true; }
		void dontUseValueList() { useValueList_ = false; }
		void useCustomParser() { useCustomParser_ = true; }
		void dontUseCustomParser() { useCustomParser_ = false; }
		void setNumber(double num) { num_ = std::clamp<double>(num, min_, max_); notifyValueChanged(); }
		void setIncrementAmount(double incrementAmount) { incrementAmount_ = incrementAmount; }
		void setMaximum(double max)
		{
			if (max >= min_)
			{ max_ = max; 
			} }
		void setMinimum(double min) { if (min <= max_) { min_ = min; } }
		void setDicimalDigit(int decimalDigit) { decimalDigit_ = decimalDigit; }
		void setPrefix(const juce::String& prefix) { prefix_ = prefix; }
		void setSuffix(const juce::String& suffix) { suffix_ = suffix; }
		void setIsInteger(bool shouldbeint) { isInteger_ = shouldbeint; decimalDigit_ = 0; }
		void setMouseSensitivity(double sensitivity = 1.0) { mouseSensitivity_ = sensitivity; }
		void setColour(NumericSpinBox::Colours colourType, const juce::Colour& newColour);

		void notifyValueChanged() { repaint(); }

		// When using value list, the ratio between (maxValue - minValue) is used as index
		// if automaticallyAdjustMinMax is true, min is set to 0 & max is set to list.size()-1 & incrementamount is set to 1
		void setValueList(const std::vector<juce::String>& list, bool useValueList = true, bool automaticallyAdjustMinMax = true) 
		{ 
			valueList_ = list; 
			useValueList_ = useValueList;
			if(automaticallyAdjustMinMax) min_ = 0; max_ = list.size() - 1; incrementAmount_ = 1;
		}

		// test input is passed, and expected to return number(known as index or value)
		void setCustomParser(std::function<double(const juce::String&)> parser, bool useCustomParser = true) { customParser_ = parser; useCustomParser_ = useCustomParser; }

		void addListener(NumericSpinBox::Listener* listener) { listeners_.push_back(listener); }
		void removeListener(NumericSpinBox::Listener* listener) 
		{
			listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
		}

		void mouseDown(const juce::MouseEvent& e) override;
		void mouseGlobDown(const juce::MouseEvent& e);
		void mouseDrag(const juce::MouseEvent& e) override;
		void mouseMove(const juce::MouseEvent& e) override;
		void mouseExit(const juce::MouseEvent& e) override;
		void mouseUp(const juce::MouseEvent& e) override;
		void mouseDoubleClick(const juce::MouseEvent& e) override;
		void textEditorReturnKeyPressed(juce::TextEditor& e) override;
	};
}