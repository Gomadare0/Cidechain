#pragma once

#include <JuceHeader.h>
#include "Constants.h"

namespace myplug
{
	class BandSplitView : public juce::AnimatedAppComponent
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;
			virtual void splitFrequencyChanged(BandSplitView*){}
			virtual void selectedBandChanged(BandSplitView*){}
		};

	private:

		enum Band
		{
			low = 0, mid, high, kNumBands,
		};

		std::array<float, fftSize * 2>& fftData_;
		std::array<float, fftSize> fftResult;
		std::deque<std::array<float, fftSize>> recentfftResult;
		bool& isNextFFTBlockReady_;

		int& sampleRate;
		int integralSampleLength = 48000 / fftSize * 0.1;

		int minFreq_ = 20;
		int maxFreq_ = 20000;
		int currentBand_ = Band::low;

		double lowmidFreq_ = 200.0;
		double midhighFreq_ = 8000.0;

		std::vector<BandSplitView::Listener*> listeners_;

		// Mouse
		int mouseInsectedHandle_ = -1;
		int mouseInsectedBand_ = -1;

		double calcNormalizedLogPos(double hz);
		double calcScreenLogPos(double hz);
		double calcHzFromLogScreenPos(double x);
		void callListeners();
			
	public:
		BandSplitView(std::array<float, fftSize * 2>& fftData, bool& isNextFFTBlockReady, int& samplerate)
			: fftData_(fftData)
			, isNextFFTBlockReady_(isNextFFTBlockReady)
			, sampleRate(samplerate)
		{
			setFramesPerSecond(30);
			fftResult.fill(0.0);
		}

		void paint(juce::Graphics& g) override;
		void update() override;

		int getSampleRate() { return sampleRate; }
		int getSelectedBand() { return currentBand_; }
		double getLowMidFreq() { return lowmidFreq_; }
		double getMidHighFreq() { return midhighFreq_; }

		void setLowMidFreq(double freq) { lowmidFreq_ = freq; }
		void setMidHighFreq(double freq) { midhighFreq_ = freq; }
		//void resized() override;

		void addListener(BandSplitView::Listener* listener) { listeners_.push_back(listener); }
		void removeListener(BandSplitView::Listener* listener)
		{
			listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
		}

		void mouseDown(const juce::MouseEvent& e) override;
		void mouseDrag(const juce::MouseEvent& e) override;
		void mouseMove(const juce::MouseEvent& e) override;
		void mouseExit(const juce::MouseEvent& e) override;
	};
}