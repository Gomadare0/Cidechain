/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "../JuceLibraryCode/BinaryData.h"

//==============================================================================
VocoderAudioProcessorEditor::VocoderAudioProcessorEditor(VocoderAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), apvtsRef(vts), bandsplitview_(p.fftData, p.nextFFTBlockReady, p.samplerate)
    , param_mix_(*vts.getParameter("Mix"), slider_mix_)
    , param_midimode_(*vts.getParameter("Midi_Trg_Mode"), combo_midimode_)
    , param_lfoenabled_(*vts.getParameter("Lfo_Enabled"), button_lfo_)
    , param_lfonumerator_(*vts.getParameter("Lfo_Numerator"), spin_rate_numerator_)
    , param_lfodenominator_(*vts.getParameter("Lfo_Denominator"), spin_rate_denominator_)
    , param_envtrg_(*vts.getParameter("EnvTrg"), combo_envmode_)
    , param_lfostop_(*vts.getParameter("Lfo_Stop"), button_lfostop_)
    , param_lfomultiplier_(*vts.getParameter("Lfo_Multiplier"), spin_rate_seq_)
    , param_offset_(*vts.getParameter("Lfo_Offset"), spin_rate_offset_)
    , param_lowmidFreq_(*vts.getParameter("Low_Mid_Freq"), bandsplitview_, envedit_.pointEdit_, 0)
    , param_midhighFreq_(*vts.getParameter("Mid_High_Freq"), bandsplitview_, envedit_.pointEdit_, 1)
    , param_hold_(*vts.getParameter("HoldValue"), button_hold_)
    , param_declick_(*vts.getParameter("Declick"), button_declick_)
    , param_declicklen_(*vts.getParameter("DeclickLength"), spin_declicklen_)
    , paramListener_envtrg_(envedit_.pointEdit_)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1280, 720);

    envedit_.setBounds(65, 190, 940, 500);
    envedit_.setEnvelopeManager(&p.envmng_);
    envedit_.fitToEnvelope();
    envedit_.pointEdit_.setEndPointXMovement(false);
    envedit_.pointEdit_.setZoomRateRestriction(false);
    addAndMakeVisible(envedit_);

    for (int i = 0; i < previewFrames; ++i)
    {
        preview_[i].setBounds(0, 0, 108, 91);
        preview_[i].setTopLeftPosition(75 + i * 10 + i * 108, 75);
        preview_[i].setEnvelopeGenerator(&p.envgen_[i], {&p.envgenMB_[i][0], &p.envgenMB_[i][1], &p.envgenMB_[i][2] });
        preview_[i].addListener(this);
        envedit_.pointEdit_.addListener(&preview_[i]);

        addAndMakeVisible(preview_[i]);
    }

    slider_mix_.setRange(0.0, 100.0, 0.01);
    slider_mix_.setNumDecimalPlacesToDisplay(0);
    slider_mix_.setTextValueSuffix("%");
    slider_mix_.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    slider_mix_.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    slider_mix_.textFromValueFunction = nullptr;

    combo_midimode_.addItemList({"OFF", "Loop while note-on", "Oneshot"}, 1);
    param_midimode_.sendInitialUpdate();

    spin_rate_numerator_.setMinimum(1);
    spin_rate_numerator_.setMaximum(8);
    spin_rate_numerator_.setDicimalDigit(0);
    spin_rate_denominator_.setValueList({ "1/1","1/2","1/2d (3/4)","1/2t (1/3)","1/4","1/4d (3/8)","1/4t (1/6)","1/8","1/8d (3/16)","1/8t (1/12)","1/16","1/16d (3/32)","1/16t (1/24)", "1/32"});
    spin_rate_denominator_.setCustomParser(
        [](const juce::String& text) -> double
        {
            bool containSlash = text.contains("/");
            juce::String buf = text.removeCharacters(" ");
            juce::String left = buf.upToFirstOccurrenceOf("/", false, false);
            const char* leftTable[] = {u8"1", u8"2", u8"2d", u8"2t", u8"4", u8"4d", u8"4t", u8"8", u8"8d", u8"8t", u8"16", u8"16d", u8"16t", u8"32"};
            const char* wholeTable1[] = { u8"1/1", u8"1/2", u8"1/2d", u8"1/2t", u8"1/4", u8"1/4d", u8"1/4t", u8"1/8", u8"1/8d", u8"1/8t", u8"1/16", u8"1/16d", u8"1/16t", u8"1/32" };
            const char* wholeTable2[] = { u8"1/1", u8"1/2", u8"3/4", u8"1/3", u8"1/4", u8"3/8", u8"1/6", u8"1/8", u8"3/16", u8"1/12", u8"1/16", u8"3/32", u8"1/24", u8"1/32" };
            if (!containSlash)
            {
                for (int i = 0; i < sizeof(leftTable) / sizeof(char*); ++i)
                {
                    if (std::strncmp(leftTable[i], left.toRawUTF8(), 3) == 0)
                    {
                        return i;
                    }
                }
                if (left == "12")
                {
                    return 9;
                }
                else if (left == "24")
                {
                    return 12;
                }
                return 0;
            }
            else
            {
                for (int i = 0; i < sizeof(wholeTable1) / sizeof(char*); ++i)
                {
                    if (std::strncmp(wholeTable1[i], buf.toRawUTF8(), 6) == 0)
                    {
                        return i;
                    }
                    if (std::strncmp(wholeTable2[i], buf.toRawUTF8(), 5) == 0)
                    {
                        return i;
                    }
                }
                return 0;
            }
        }
    );

    spin_rate_seq_.setMinimum(0.1);
    spin_rate_seq_.setMaximum(10.0);
    spin_rate_seq_.setPrefix("x ");
    spin_rate_seq_.setIncrementAmount(0.1);
    spin_rate_seq_.setDicimalDigit(1);

    spin_rate_offset_.setMinimum(0.0);
    spin_rate_offset_.setMaximum(100.0);
    spin_rate_offset_.setPrefix("Offset ");
    spin_rate_offset_.setSuffix("%");
    spin_rate_offset_.setIncrementAmount(0.1);
    spin_rate_offset_.setDicimalDigit(1);
    spin_rate_offset_.setMouseSensitivity(10.0);

    button_lfostop_.setButtonText("Don't use LFO when paused");

    combo_envmode_.addItemList({"Disabled", "Enabled"}, 1);
    combo_envmode_.addListener(&paramListener_envtrg_);
    param_envtrg_.sendInitialUpdate();

    button_hold_.setButtonText("Hold Value");
    button_declick_.setButtonText("Declick");

    spin_declicklen_.setMinimum(1.0);
    spin_declicklen_.setMaximum(1000.0);
    spin_declicklen_.setSuffix("spls");
    spin_rate_offset_.setIncrementAmount(1);
    spin_rate_offset_.setDicimalDigit(0);

    addAndMakeVisible(slider_mix_);
    addAndMakeVisible(combo_midimode_);
    addAndMakeVisible(button_lfo_);
    addAndMakeVisible(spin_rate_numerator_);
    addAndMakeVisible(spin_rate_denominator_);
    addAndMakeVisible(spin_rate_seq_);
    addAndMakeVisible(spin_rate_offset_);
    addAndMakeVisible(combo_envmode_);
    addAndMakeVisible(button_lfostop_);
    addAndMakeVisible(bandsplitview_);
    addAndMakeVisible(button_hold_);
    addAndMakeVisible(button_declick_);
    addAndMakeVisible(spin_declicklen_);

    setLookAndFeel(&customlookandfeel);
}

VocoderAudioProcessorEditor::~VocoderAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    for (int i = 0; i < previewFrames; ++i)
    {
        preview_[i].removeListener(this);
        envedit_.pointEdit_.removeListener(&preview_[i]);
    }
    combo_envmode_.removeListener(&paramListener_envtrg_);
}

//==============================================================================
void VocoderAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    juce::Image backgroundImg = juce::ImageCache::getFromMemory(BinaryData::bgimg_png, BinaryData::bgimg_pngSize);
    g.drawImageAt(backgroundImg, 0, 0);
}

void VocoderAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    slider_mix_.setBounds(1045,222,60,60);
    combo_midimode_.setBounds(1035, 316, 205, 25);
    button_lfo_.setBounds(1075, 357, 25, 20);
    spin_rate_numerator_.setBounds(1035, 379, 37, 25);
    spin_rate_denominator_.setBounds(1097, 379, 143, 25);
    spin_rate_seq_.setBounds(1035, 415, 205, 25);
    spin_rate_offset_.setBounds(1035, 452, 205, 25);
    button_lfostop_.setBounds(1035, 489, 205, 25);
    combo_envmode_.setBounds(1035, 539, 205, 25);
    bandsplitview_.setBounds(1035, 571, 205, 100);
    button_hold_.setBounds(1124, 197, 116, 25);
    button_declick_.setBounds(1124, 227, 116, 25);
    spin_declicklen_.setBounds(1124, 257, 116, 25);
}

void VocoderAudioProcessorEditor::clicked(myplug::envelope::EnvelopePreviewComponent* c)
{
    for (int i = 0; i < 10; ++i)
    {
        if (c == preview_ + i)
        {
            apvtsRef.getParameter("Banks")->setValueNotifyingHost(i / 9.0f);
            previewingIndex_ = i;
            break;
        }
    }
}