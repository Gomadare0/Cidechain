/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "Envelope_Data.h"
#include "Envelope_Manager.h"
#include "Envelope_VoiceController.h"
#include "ParamAttachment.h"
#include "Constants.h"
#include "DSP_Filter.h"

//==============================================================================
/**
*/
class MyplugAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MyplugAudioProcessor();
    ~MyplugAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

public:
    // Envelopes
    myplug::envelope::EnvelopeManager envmng_;
    myplug::envelope::CachedEnvelopeGenerator envgen_[10];
    myplug::envelope::CachedEnvelopeGenerator envgenMB_[10][3];
    myplug::envelope::EnvelopeVoiceController envVoice_;

    // Envtrigger Buffer
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    int samplerate = 48000;

private:
    void pushNextSampleIntoFifo(float sample);
    void updateLFORate(double bpm = 120.0);
    void recalcAndUpdateXPos(juce::AudioPlayHead& playhead);

    juce::dsp::WindowingFunction<float> windowFunc;
    juce::dsp::FFT fft;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParamLayout();
    int midiVoices = 0;
    bool isMidiTriggered = false;
    juce::AudioPlayHead::CurrentPositionInfo prevPosInfo_;

    std::array<float, declickCacheLength> declickCache_;
    int declickCacheIndex_ = 0;
    std::array<float, declickCacheLength> declickCacheMB_[3];
    int declickCacheMBIndex_ = 0;

    // Params
    juce::RangedAudioParameter* param_banks_;
    juce::RangedAudioParameter* param_mix_;
    juce::RangedAudioParameter* param_miditrg_;
    juce::RangedAudioParameter* param_lfoenabled_;
    juce::RangedAudioParameter* param_lfonumerator_;
    juce::RangedAudioParameter* param_lfodenominator_;
    juce::RangedAudioParameter* param_lfomultiplier_;
    juce::RangedAudioParameter* param_lfooffset_;
    juce::RangedAudioParameter* param_lfostop_;
    juce::RangedAudioParameter* param_envTrg_;
    juce::RangedAudioParameter* param_lowmidFreq_;
    juce::RangedAudioParameter* param_midhighFreq_;
    juce::RangedAudioParameter* param_hold_;
    juce::RangedAudioParameter* param_declick_;
    juce::RangedAudioParameter* param_declicklen_;

    myplug::GenericListener listener_banks_;
    myplug::GenericListener listener_lowmidFreq_;
    myplug::GenericListener listener_midhighFreq_;

    // Filters
    myplug::DSP::Butterworth_2nd lowmid_lp_[2][2];
    myplug::DSP::Butterworth_2nd lowmid_hp_[2][2];
    myplug::DSP::Butterworth_2nd midhigh_lp_[2][2];
    myplug::DSP::Butterworth_2nd midhigh_hp_[2][2];

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyplugAudioProcessor)
};
