/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include "Envelope_Editor.h"
#include "Envelope_PreviewComponent.h"
#include "Component_NumericSpinbox.h"
#include "Component_BandSplitter.h"
#include "customlookandfeel.h"
#include "ParamAttachment.h"

constexpr size_t previewFrames = 10;

//==============================================================================
/**
*/
class VocoderAudioProcessorEditor 
    : public juce::AudioProcessorEditor
    , public myplug::envelope::EnvelopePreviewComponent::Listener
{
public:
    VocoderAudioProcessorEditor (VocoderAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~VocoderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

protected:
    void clicked(myplug::envelope::EnvelopePreviewComponent*) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    VocoderAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvtsRef;

    myplug::CustomLookAndFeel customlookandfeel;
    myplug::envelope::EnvelopeEditor envedit_;
    myplug::envelope::EnvelopePreviewComponent preview_[previewFrames];
    int previewingIndex_ = 0;
    juce::Slider slider_mix_;
    juce::ComboBox combo_midimode_;
    juce::ToggleButton button_lfo_;
    myplug::NumericSpinBox spin_rate_numerator_;
    myplug::NumericSpinBox spin_rate_denominator_;
    myplug::NumericSpinBox spin_rate_seq_;
    myplug::NumericSpinBox spin_rate_offset_;
    juce::ToggleButton button_lfostop_;
    juce::ComboBox combo_envmode_;
    myplug::BandSplitView bandsplitview_;
    juce::ToggleButton button_hold_;
    juce::ToggleButton button_declick_;
    myplug::NumericSpinBox spin_declicklen_;

    // Param Listener
    juce::SliderParameterAttachment param_mix_;
    juce::ComboBoxParameterAttachment param_midimode_;
    juce::ButtonParameterAttachment param_lfoenabled_;
    myplug::NumericSpinboxParameterAttachment param_lfonumerator_;
    myplug::NumericSpinboxParameterAttachment param_lfodenominator_;
    myplug::NumericSpinboxParameterAttachment param_lfomultiplier_;
    myplug::NumericSpinboxParameterAttachment param_offset_;
    myplug::BandSplitViewParameterAttachment param_lowmidFreq_;
    myplug::BandSplitViewParameterAttachment param_midhighFreq_;
    juce::ComboBoxParameterAttachment param_envtrg_;
    juce::ButtonParameterAttachment param_lfostop_;
    juce::ButtonParameterAttachment param_hold_;
    juce::ButtonParameterAttachment param_declick_;
    myplug::NumericSpinboxParameterAttachment param_declicklen_;

    myplug::Listener_EnvTrg paramListener_envtrg_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocoderAudioProcessorEditor)
};
