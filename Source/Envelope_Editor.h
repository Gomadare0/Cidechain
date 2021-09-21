#pragma once

#include <JuceHeader.h>
#include "Envelope_Fwd.h"
#include "Envelope_Data.h"
#include "Envelope_Manager.h"
#include "Envelope_PointEditMultiband.h"
#include "Component_NumericSpinbox.h"

namespace myplug
{
	namespace envelope
	{
		class EnvelopeEditor : public juce::Component, public juce::ScrollBar::Listener, public juce::ToggleButton::Listener, public NumericSpinBox::Listener
		{
		public:
			PointEditMB pointEdit_;
		
		private:
			EnvelopeManager* envManager_ = nullptr;

			// Controls
			juce::ScrollBar pointEditScroll_;
			juce::ToggleButton showgridButton_;
			juce::ToggleButton snapgridButton_;
			juce::Label label_griddiv_;
			myplug::NumericSpinBox spin_griddivY_;
			myplug::NumericSpinBox spin_griddivX_;

		public:
			EnvelopeEditor();
			~EnvelopeEditor();

			void paint(juce::Graphics& g) override;
			void resized() override;
			void updateScrollBarRange();

			void fitToEnvelope()
			{
				pointEdit_.setZoomRateToWholeRange();
				updateScrollBarRange();
			}
			void setEnvelopeManager(EnvelopeManager* envManager)
			{
				envManager_ = envManager;
				pointEdit_.setNewEnvelopeManager(envManager);
				updateScrollBarRange();
			}

			EnvelopeManager* const getEnvelopeManager() { return envManager_; }

			void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
			void buttonClicked(juce::Button* button) override;
			void onNumberChanged(NumericSpinBox*) override;
		};
	}
}