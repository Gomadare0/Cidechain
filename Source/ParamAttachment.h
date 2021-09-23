#pragma once

#include <JuceHeader.h>
#include "Component_NumericSpinbox.h"
#include "Component_BandSplitter.h"
#include "Envelope_PointEditMultiband.h"

namespace myplug
{
	class NumericSpinboxParameterAttachment : NumericSpinBox::Listener
	{
	public:
		NumericSpinboxParameterAttachment(juce::RangedAudioParameter& parameter, NumericSpinBox& spinbox, juce::UndoManager* undoManager = nullptr)
			: attachment_(parameter, [&spinbox](float num) {spinbox.setNumber(num); }, undoManager)
		{
			sendInitialUpdate();
			spinbox.addListener(this);
		}
		~NumericSpinboxParameterAttachment() override
		{
			spinbox_.removeListener(this);
		}
		void sendInitialUpdate()
		{
			attachment_.sendInitialUpdate();
		}

	protected:
		void onNumberChanged(NumericSpinBox* box) override
		{
			attachment_.setValueAsCompleteGesture(box->getCurrentNumber());
		}

	private:
		NumericSpinBox spinbox_;
		juce::ParameterAttachment attachment_;
	};

	class BandSplitViewParameterAttachment : protected BandSplitView::Listener
	{
	public:
		BandSplitViewParameterAttachment(juce::RangedAudioParameter& parameter, BandSplitView& view, envelope::PointEditMB& pointedit, int leftBand, juce::UndoManager* undoManager = nullptr)
			: attachment_(parameter, [&](float num) 
				{
					if (leftband_ == 0)
					{
						view_.setLowMidFreq(num);
					}
					else if (leftband_ == 1)
					{
						view_.setMidHighFreq(num);
					}
				}, undoManager)
			, view_(view)
			, leftband_(leftBand)
			, pointedit_(pointedit)
		{
			sendInitialUpdate();
			view_.addListener(this);
		}
		~BandSplitViewParameterAttachment() override
		{
			view_.removeListener(this);
		}
		void sendInitialUpdate()
		{
			attachment_.sendInitialUpdate();
		}
	protected:
		void splitFrequencyChanged(BandSplitView* v) override
		{
			if (leftband_ == 0)
			{
				attachment_.setValueAsCompleteGesture(v->getLowMidFreq());
			}
			else if (leftband_ == 1)
			{
				attachment_.setValueAsCompleteGesture(v->getMidHighFreq());
			}
		}
		void selectedBandChanged(BandSplitView* v) override
		{
			int band = v->getSelectedBand();
			switch (band)
			{
			case 0:
				pointedit_.setEditingBand("low");
				break;
			case 1:
				pointedit_.setEditingBand("mid");
				break;
			case 2:
				pointedit_.setEditingBand("high");
				break;
			default:
				break;
			}
		}
	private:
		int leftband_;
		BandSplitView& view_;
		envelope::PointEditMB& pointedit_;
		juce::ParameterAttachment attachment_;
	};

	class Listener_EnvTrg : public juce::ComboBox::Listener
	{
		envelope::PointEditMB& view_;
	public:
		Listener_EnvTrg(envelope::PointEditMB& view)
			: view_(view)
		{

		}
		void comboBoxChanged(juce::ComboBox* c) override
		{
			if (c->getSelectedItemIndex() == 0)
			{
				view_.setMultiband(false);
			}
			else
			{
				view_.setMultiband(true);
			}
		}
	};

	class Listener_PlugEditGUIScaling : public myplug::NumericSpinBox::Listener
	{
		void* proc_;
	public:
		Listener_PlugEditGUIScaling(void* PluginEditorPtr) : proc_(PluginEditorPtr) {}
		
		void onNumberChanged(NumericSpinBox*) override {}
		void startGesture(NumericSpinBox*) override {}		
		void endGesture(NumericSpinBox*) override;
	};

	class GenericListener : public juce::AudioProcessorParameter::Listener
	{
		std::function<void(float)> listenerFunc_ = nullptr;

	public:
		void setListener(std::function<void(float)> func)
		{
			listenerFunc_ = func;
		}

		void parameterValueChanged(int parameterIndex, float newValue) override
		{
			if (listenerFunc_)
				listenerFunc_(newValue);
		}
		void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override
		{

		}
	};
}