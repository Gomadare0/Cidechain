#pragma once

#include <JuceHeader.h>
#include "Envelope_Data.h"
#include "Envelope_Editor.h"

namespace myplug
{
	namespace envelope
	{
		class EnvelopePreviewComponent : public juce::AnimatedAppComponent, public envelope::PointEditMBListener
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;
				virtual void clicked(EnvelopePreviewComponent*) = 0;
			};
		private:
			envelope::EnvelopeGenerator* envgen_ = nullptr;
			std::array<envelope::EnvelopeGenerator*, 3> envgenMB_ = {nullptr, nullptr, nullptr};
			std::vector<EnvelopePreviewComponent::Listener*> listeners_;
			void callListeners()
			{
				for (const auto& i : listeners_)
				{
					i->clicked(this);
				}
			}
		public:
			EnvelopePreviewComponent()
			{
				setFramesPerSecond(10);
			}

			void setEnvelopeGenerator(EnvelopeGenerator* envgen, std::array<envelope::EnvelopeGenerator*, 3> envgenMB)
			{
				envgen_ = envgen;
				envgenMB_ = envgenMB;
			}
			void paint(juce::Graphics& g) override;
			void update() override
			{
				repaint();
			}
			
			void handleMoved(PointEditMB* p) override
			{
				if(p->getEnvelopeManager()->getEnvGeneratorPtr() == envgen_)
					repaint();			
			}

			void addListener(EnvelopePreviewComponent::Listener* listener) { listeners_.push_back(listener); }
			void removeListener(EnvelopePreviewComponent::Listener* listener) {	listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end()); }

			void mouseDown(const juce::MouseEvent& e) override;
			void mouseMove(const juce::MouseEvent& e) override;
			void mouseExit(const juce::MouseEvent& e) override;
		};
	}
}