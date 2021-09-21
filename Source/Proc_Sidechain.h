#pragma once

#include <JuceHeader.h>

namespace myplug
{
	class SidechainProc : public juce::AudioProcessor
	{
	public:
		SidechainProc();

		const juce::String getName() const override
		{
			return "Sidechain gain processor";
		}
		void releaseResources() override
		{

		}
		void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override
		{

		}
		void processBlock(juce::AudioBuffer< float >& buffer, juce::MidiBuffer& midiMessages)
		{

		}
		double getTailLengthSeconds() const override
		{
			return 0;
		}
		bool acceptsMidi() const override
		{
			return true;
		}
		bool producesMidi() const override
		{
			return false;
		}
		juce::AudioProcessorEditor* createEditor() override
		{
			return nullptr;
		}
		bool hasEditor() const override
		{
			return false;
		}
		int getNumPrograms() override
		{
			return 0;
		}
		int getCurrentProgram() override
		{
			return 0;
		}
		void setCurrentProgram(int index) override
		{
			return;
		}
		const juce::String	getProgramName(int index) override
		{
			return "";
		}
		void changeProgramName(int index, const juce::String& newName) override
		{

		}
		void getStateInformation(juce::MemoryBlock& destData) override
		{

		}
		void getCurrentProgramStateInformation(juce::MemoryBlock& destData) override
		{

		}
		void setStateInformation(const void* data, int sizeInBytes) override
		{

		}
	};
}