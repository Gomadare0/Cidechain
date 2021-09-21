/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ValueNormalization.h"
#include "Envelope_Serialization.h"

static const float denomBeatTable[] = { 4, 2, 3, 4.0 / 3.0, 1, 3.0 / 2.0, 2.0 / 3.0, 0.5, 3.0 / 4.0, 1.0 / 3.0, 0.25, 3.0 / 8.0, 1.0 / 6.0, 0.125 };

void VocoderAudioProcessor::pushNextSampleIntoFifo(float sample)
{
    fifo[fifoIndex++] = sample;

    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            windowFunc.multiplyWithWindowingTable(fifo.data(), fftSize);

            fftData.fill(0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());

            fft.performFrequencyOnlyForwardTransform(fftData.data());

            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }
}

void VocoderAudioProcessor::updateLFORate(double bpm)
{
    auto num = param_lfonumerator_->getValue();
    auto denom = param_lfodenominator_->getValue();
    juce::AudioPlayHead::CurrentPositionInfo posinfo;

    float beatTime = (60.0 / bpm) * myplug::denormalizeValue<float>(num, 1.0, 8.0) * denomBeatTable[static_cast<size_t>(myplug::denormalizeValue<float>(denom, 0.0, 13.0))];
    //float blockTime = getBlockSize() / getSampleRate();
    float multiplier = param_lfomultiplier_->convertFrom0to1(param_lfomultiplier_->getValue());

    envVoice.setDelta(1.0 / beatTime * multiplier / getSampleRate());
}

void VocoderAudioProcessor::recalcAndUpdateXPos(juce::AudioPlayHead& playhead)
{
    auto num = param_lfonumerator_->getValue();
    auto denom = param_lfodenominator_->getValue();
    auto denomFloat = denomBeatTable[static_cast<size_t>(myplug::denormalizeValue<float>(denom, 0.0, 13.0))];
    juce::AudioPlayHead::CurrentPositionInfo posinfo;
    playhead.getCurrentPosition(posinfo);

    float multiplier = (4.0f / denomFloat) / (myplug::denormalizeValue<float>(num, 1.0, 8.0) / posinfo.timeSigNumerator * 4.0f);
    float posInBar = std::fmod(posinfo.ppqPosition, posinfo.timeSigNumerator) / posinfo.timeSigNumerator;

    envVoice.setXPos(std::fmod(posInBar * multiplier + param_lfooffset_->getValue(), 1.0));
}

juce::AudioProcessorValueTreeState::ParameterLayout VocoderAudioProcessor::createParamLayout()
{
    auto layout = juce::AudioProcessorValueTreeState::ParameterLayout();

    layout.add(std::make_unique<juce::AudioParameterInt>("Banks", "Banks", 1, 10, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 100.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Midi_Trg_Mode", "Midi Trigger Mode", juce::StringArray{ "OFF", "Loop while note-on", "Oneshot" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>("Lfo_Enabled", "LFO Enabled", true));
    layout.add(std::make_unique<juce::AudioParameterInt>("Lfo_Numerator", "LFO Rate Numerator", 1, 8, 1));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Lfo_Denominator", "LFO Rate Denominator"
        , juce::StringArray{ "1/1","1/2","1/2d","1/2t","1/4","1/4d","1/4t","1/8","1/8d","1/8t","1/16","1/16d","1/16t","1/32" }, 4));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Lfo_Multiplier", "LFO Rate Multiplier", juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Lfo_Offset", "LFO Offset", juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>("Lfo_Stop", "Stop LFO when paused", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>("EnvTrg", "Multiband", juce::StringArray{ "Disabled", "Enabled" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Low_Mid_Freq", "Low-Mid Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.01f), 200.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Mid_High_Freq", "Mid-High Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.01f), 8000.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>("HoldValue", "Hold Value", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("Declick", "Declick", true));
    layout.add(std::make_unique<juce::AudioParameterInt>("DeclickLength", "Declick Length", 1, 1000, 100));

    return layout;
}

//==============================================================================
VocoderAudioProcessor::VocoderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
    , apvts(*this, nullptr, "Parameters", std::move(createParamLayout()))
    , windowFunc(fftSize, juce::dsp::WindowingFunction<float>::WindowingMethod::blackmanHarris)
    , fft(fftOrder)
{
    DEBUG_windowedfifo.fill(0.0);

    // Init Envelopes
    for (int i = 0; i < 10; ++i)
    {
        envgen_[i].addPoint(0.0, 0.0);
        envgen_[i].addPoint(0.2, 1.0);
        envgen_[i].addPoint(0.8, 1.0, myplug::envelope::InterpolationType::Sigmoid, -10.0);
        envgen_[i].addPoint(1.0, 0.0);
        envgen_[i].render();

        auto& points = envgen_[i];

        points.getFirstPoint().loop = myplug::envelope::LoopState::LoopStart;
        points.getLastPoint().loop = myplug::envelope::LoopState::LoopEnd;

        // Multiband
        for (int k = 0; k < 3; ++k)
        {
            envgenMB_[i][k].addPoint(0.0, 0.0);
            envgenMB_[i][k].addPoint(0.2, 1.0);
            envgenMB_[i][k].addPoint(0.8, 1.0, myplug::envelope::InterpolationType::Sigmoid, -10.0);
            envgenMB_[i][k].addPoint(1.0, 0.0);
            envgenMB_[i][k].render();

            auto& pointsMB = envgenMB_[i][k];

            pointsMB.getFirstPoint().loop = myplug::envelope::LoopState::LoopStart;
            pointsMB.getLastPoint().loop = myplug::envelope::LoopState::LoopEnd;
        }
        envgenMB_[i][0].setName("low");
        envgenMB_[i][1].setName("mid");
        envgenMB_[i][2].setName("high");
    }

    envmng_.registerEnvGenerator(&envgen_[0]);
    envmng_.registerEnvGenerator(&envgenMB_[0][0]);
    envmng_.registerEnvGenerator(&envgenMB_[0][1]);
    envmng_.registerEnvGenerator(&envgenMB_[0][2]);
    envmng_.addEnvVoiceController(&envVoice);
    envmng_.setLoopMode(myplug::envelope::LoopMode::LoopThenRelease);
    envVoice.notifyLoopPosChanged();

    param_banks_ = apvts.getParameter("Banks");
    param_mix_ = apvts.getParameter("Mix");
    param_miditrg_ = apvts.getParameter("Midi_Trg_Mode");
    param_lfoenabled_ = apvts.getParameter("Lfo_Enabled");
    param_lfonumerator_ = apvts.getParameter("Lfo_Numerator");
    param_lfodenominator_ = apvts.getParameter("Lfo_Denominator");
    param_lfomultiplier_ = apvts.getParameter("Lfo_Multiplier");
    param_lfooffset_ = apvts.getParameter("Lfo_Offset");
    param_lfostop_ = apvts.getParameter("Lfo_Stop");
    param_envTrg_ = apvts.getParameter("EnvTrg");
    param_lowmidFreq_ = apvts.getParameter("Low_Mid_Freq");
    param_midhighFreq_ = apvts.getParameter("Mid_High_Freq");
    param_hold_ = apvts.getParameter("HoldValue");
    param_declick_ = apvts.getParameter("Declick");
    param_declicklen_ = apvts.getParameter("DeclickLength");

    listener_banks_.setListener([&](float newValue)
        {
            int bank = param_banks_->convertFrom0to1(newValue);
            --bank;
            envmng_.getEnvGeneratorsPtr()[0] = &envgen_[bank];
            envmng_.getEnvGeneratorsPtr()[1] = &envgenMB_[bank][0];
            envmng_.getEnvGeneratorsPtr()[2] = &envgenMB_[bank][1];
            envmng_.getEnvGeneratorsPtr()[3] = &envgenMB_[bank][2];
        });
    listener_lowmidFreq_.setListener([&](float newValue)
        {
            float freq = param_lowmidFreq_->convertFrom0to1(newValue);
            for (int channel = 0; channel < 2; ++channel)
            {
                for (int order = 0; order < 2; ++order)
                {
                    lowmid_lp_[order][channel].calcCoeff(samplerate, freq, false);
                    lowmid_hp_[order][channel].calcCoeff(samplerate, freq, true);
                }
            }
        });
    listener_midhighFreq_.setListener([&](float newValue)
        {
            float freq = param_lowmidFreq_->convertFrom0to1(newValue);
            for (int channel = 0; channel < 2; ++channel)
            {
                for (int order = 0; order < 2; ++order)
                {
                    midhigh_lp_[order][channel].calcCoeff(samplerate, freq, false);
                    midhigh_hp_[order][channel].calcCoeff(samplerate, freq, true);
                }
            }
        });

    param_banks_->addListener(&listener_banks_);
    param_lowmidFreq_->addListener(&listener_lowmidFreq_);
    param_midhighFreq_->addListener(&listener_midhighFreq_);

    // Initial Update
    param_lowmidFreq_->sendValueChangedMessageToListeners(param_lowmidFreq_->getValue());
    param_midhighFreq_->sendValueChangedMessageToListeners(param_midhighFreq_->getValue());

    // Init Cache
    declickCacheMB_[0].fill(0.0); declickCacheMB_[1].fill(0.0); declickCacheMB_[2].fill(0.0);
    declickCache_.fill(0.0);
}

VocoderAudioProcessor::~VocoderAudioProcessor()
{
    param_banks_->removeListener(&listener_banks_);
    param_lowmidFreq_->removeListener(&listener_lowmidFreq_);
    param_midhighFreq_->removeListener(&listener_midhighFreq_);
}

//==============================================================================
const juce::String VocoderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VocoderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VocoderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VocoderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VocoderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VocoderAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VocoderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VocoderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VocoderAudioProcessor::getProgramName (int index)
{
    return {};
}

void VocoderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VocoderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    updateLFORate();
    samplerate = sampleRate;
}
void VocoderAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VocoderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VocoderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    auto playhead = getPlayHead();
    if (playhead)
    {
        juce::AudioPlayHead::CurrentPositionInfo posinfo;
        playhead->getCurrentPosition(posinfo);
        updateLFORate(posinfo.bpm);
    }
    else
    {
        updateLFORate();
    }

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool useLFO = false;
    bool useLFObutignoreplayhead = false;
    bool isMidiOneshot = param_miditrg_->getValue() == 1.0;
    bool isMidiLoop = param_miditrg_->getValue() == 0.5;
    bool forceUpdate = false;
    bool holdEnabled = param_hold_->getValue() == 1.0;

    // LFO@Retrigger
    if (param_lfoenabled_->getValue() == 1.0)
    {
        useLFObutignoreplayhead = true;
        do
        {
            if (param_lfostop_->getValue() == 1.0f)
            {
                if (playhead)
                {
                    juce::AudioPlayHead::CurrentPositionInfo info;
                    playhead->getCurrentPosition(info);
                    if (!info.isPlaying)
                    {
                        useLFO = false; // If not playing, don't apply LFO
                        break;
                    }
                }
            }
            useLFO = true;
        } while (false);

        if (playhead)
        {
            juce::AudioPlayHead::CurrentPositionInfo info;
            playhead->getCurrentPosition(info);

            if (info.isPlaying && !prevPosInfo_.isPlaying)
            {
                // Retrigger on start
                envVoice.reset();
                recalcAndUpdateXPos(*playhead);
            }
            if (abs(info.timeInSamples - prevPosInfo_.timeInSamples) >= buffer.getNumSamples() * 3)
            {
                // When jumped, retrigger.
                envVoice.reset();
                recalcAndUpdateXPos(*playhead);
            }
        }
    }
    else {
        // if lfo is disabled, make sure to reset on play
        juce::AudioPlayHead::CurrentPositionInfo info;
        playhead->getCurrentPosition(info);

        if (info.isPlaying && !prevPosInfo_.isPlaying)
        {
            // Retrigger on start
            envVoice.reset();
        }
    }

    // MIDI
    for (const auto& i : midiMessages)
    {
        auto message = i.getMessage();
        if (message.isNoteOn())
        {
            ++midiVoices;
            if (param_miditrg_->getValue() != 0)
            {
                isMidiTriggered = true;
                envVoice.reset(); // retrigger
                if (!useLFO)
                {
                    forceUpdate = true;
                }
            }
        }
        else if (message.isNoteOff())
        {
            --midiVoices;
            if (midiVoices <= 0)
            {
                midiVoices = 0;
                isMidiTriggered = false;
            }
        }
        else if (message.isAllNotesOff())
        {
            midiVoices = 0;
            isMidiTriggered = false;
        }
    }

    bool isLFOOnly = useLFObutignoreplayhead && !isMidiLoop && !isMidiLoop;
    bool declick = param_declick_->getValue() == 1.0;

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    // Sample Proc

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        bool isRunning = false;
        bool canProcess = true;

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            if (channel == 0 || channel == 1)
            {
                auto* channelData = buffer.getWritePointer(channel);
                // Per-sample update on first channel
                if (channel == 0)
                {
                    // update FFT
                    if (totalNumInputChannels == 1)
                    {
                        pushNextSampleIntoFifo(buffer.getReadPointer(0)[i]);
                    }
                    else
                    {
                        pushNextSampleIntoFifo((buffer.getReadPointer(0)[i] + buffer.getReadPointer(1)[i]) / 2.0);
                    }

                    // Voice Update
                    if (useLFO)
                    {
                        envVoice.update(true);
                        isRunning = true;
                    }
                    else
                    {
                        if (isMidiOneshot && envVoice.getIsXAmongEnvelope())
                        {
                            envVoice.update(false); // false ... don't loop
                            isRunning = true;
                        }
                        else if (isMidiLoop)
                        {
                            // Loop
                            if (isMidiTriggered)
                            {
                                // loop when note-on
                                envVoice.update(true);
                                isRunning = true;
                            }
                            else
                            {
                                // just update to consume residue when note-off
                                if (isMidiLoop && envVoice.getIsXAmongEnvelope())
                                {
                                    envVoice.update(false);
                                    isRunning = true;
                                }
                            }
                        }
                        else
                        {
                            // when LFO & MIDI is disabled
                        }
                    }
                    if (forceUpdate)
                    {
                        envVoice.update(false);
                        isRunning = true;
                    }

                    // Hold switch
                    if (isLFOOnly && !isRunning)
                    {
                        // LFO Only and not running
                        if (!holdEnabled)
                        {
                            // not holded
                            canProcess = false;
                        }
                    }
                    else
                    {
                        if (envVoice.getXPos() >= envgen_->getLastPoint().x)
                        {
                            // when XPos exceeds envelope
                            if (!holdEnabled)
                            {
                                // not holded
                                canProcess = false;
                            }
                        }
                    }
                }

                float drySignal = channelData[i];
                float wetSignal = channelData[i];
                int declickLen = param_declicklen_->convertFrom0to1(param_declicklen_->getValue());

                // Apply ducking effect
                if (canProcess)
                {
                    auto xPos = envVoice.getXPos();

                    if (param_envTrg_->getValue() == 1.0f)
                    {
                        float lowBand = lowmid_lp_[0][channel].processSample(wetSignal); lowBand = lowmid_lp_[1][channel].processSample(lowBand);
                        float midhighBand = lowmid_hp_[0][channel].processSample(wetSignal); midhighBand = lowmid_hp_[1][channel].processSample(midhighBand);
                        float midBand = midhigh_lp_[0][channel].processSample(midhighBand); midBand = midhigh_lp_[1][channel].processSample(midBand);
                        float highBand = midhigh_hp_[0][channel].processSample(midhighBand); highBand = midhigh_hp_[1][channel].processSample(highBand);

                        //lowBand *= envmng_.getEnvGeneratorsPtr()[1]->getCachedInterpolatitonValue(xPos);
                        //midBand *= envmng_.getEnvGeneratorsPtr()[2]->getCachedInterpolatitonValue(xPos);
                        //highBand *= envmng_.getEnvGeneratorsPtr()[3]->getCachedInterpolatitonValue(xPos);
                        if (declick)
                        {
                            declickCacheMB_[0][declickCacheMBIndex_] = (envmng_.getEnvGeneratorsPtr()[1]->getCachedInterpolatitonValue(xPos));
                            declickCacheMB_[1][declickCacheMBIndex_] = (envmng_.getEnvGeneratorsPtr()[2]->getCachedInterpolatitonValue(xPos));
                            declickCacheMB_[2][declickCacheMBIndex_] = (envmng_.getEnvGeneratorsPtr()[3]->getCachedInterpolatitonValue(xPos));
                            for (int b = 0; b < 3; ++b)
                            {
                                float avg = 0.0;
                                //avg = std::accumulate(declickCacheMB_[b].begin(), declickCacheMB_[b].begin() + declickLen, 0.0);
                                for (int k = 0; k < declickLen; ++k)
                                {
                                    avg += declickCacheMB_[b][k];
                                }
                                avg /= declickLen;
                                switch (b)
                                {
                                case 0: lowBand *= avg; break;
                                case 1: midBand *= avg; break;
                                case 2: highBand *= avg; break;
                                }
                            }
                            ++declickCacheMBIndex_;
                        }
                        else
                        {

                            lowBand *= envmng_.getEnvGeneratorsPtr()[1]->getCachedInterpolatitonValue(xPos);
                            midBand *= envmng_.getEnvGeneratorsPtr()[2]->getCachedInterpolatitonValue(xPos);
                            highBand *= envmng_.getEnvGeneratorsPtr()[3]->getCachedInterpolatitonValue(xPos);
                        }

                        wetSignal = lowBand + midBand + highBand;
                    }
                    else
                    {
                        float yvalue = envmng_.getEnvGeneratorsPtr()[0]->getCachedInterpolatitonValue(xPos);
                        if (declick)
                        {
                            declickCache_[declickCacheIndex_] = yvalue;
                            float avg = 0.0;
                            //avg = std::accumulate(declickCache_.begin(), declickCache_.begin() + declickLen, 0.0);
                            for (int k = 0; k < declickLen; ++k)
                            {
                                avg += declickCache_[k];
                            }
                            avg /= declickLen;
                            ++declickCacheIndex_;
                            wetSignal *= avg;
                        }
                        else
                        {
                            wetSignal *= envmng_.getEnvGeneratorsPtr()[0]->getCachedInterpolatitonValue(xPos);
                        }

                    }
                }

                // Mix
                channelData[i] = wetSignal * param_mix_->getValue() + drySignal * (1.0 - param_mix_->getValue());

                // Pop cache
                if (declickLen - 1 <= declickCacheIndex_)
                {
                    declickCacheIndex_ = 0;
                }
                if (declickLen - 1 <= declickCacheMBIndex_)
                {
                    declickCacheMBIndex_ = 0;
                }
            }

        }

        // ..do something to the data...
    }

    // Update Cache
    if (playhead)
    {
        juce::AudioPlayHead::CurrentPositionInfo info;
        playhead->getCurrentPosition(info);
        prevPosInfo_ = info;
    }
}

//==============================================================================
bool VocoderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VocoderAudioProcessor::createEditor()
{
    return new VocoderAudioProcessorEditor (*this, apvts);
}

//==============================================================================
void VocoderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    using namespace myplug::envelope;
    juce::ValueTree tree("Parameters");

    tree.addChild(apvts.state, 0, nullptr);
    
    for (int i = 0; i < 10; ++i)
    {
        tree.addChild(serializeEnvGen(envgen_[i]), i * 4 + 1, nullptr);
        tree.addChild(serializeEnvGen(envgenMB_[i][0]), i * 4 + 2, nullptr);
        tree.addChild(serializeEnvGen(envgenMB_[i][1]), i * 4 + 3, nullptr);
        tree.addChild(serializeEnvGen(envgenMB_[i][2]), i * 4 + 4, nullptr);
    }

    auto primitiveParam = tree.createXml();
    copyXmlToBinary(*primitiveParam, destData);
    //primitiveParam->writeTo(juce::File::getCurrentWorkingDirectory().getChildFile("params.xml"));
}

void VocoderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    using namespace myplug::envelope;
    juce::ValueTree tree;
    auto xml = getXmlFromBinary(data, sizeInBytes);
    tree = juce::ValueTree::fromXml(xml->toString());

    apvts.state.copyPropertiesAndChildrenFrom(tree.getChild(0), nullptr);
    for (int i = 0; i < 10; ++i)
    {
        deserializeEnvGen(envgen_[i], tree.getChild(i * 4 + 1));
        deserializeEnvGen(envgenMB_[i][0], tree.getChild(i * 4 + 2));
        deserializeEnvGen(envgenMB_[i][1], tree.getChild(i * 4 + 3));
        deserializeEnvGen(envgenMB_[i][2], tree.getChild(i * 4 + 4));

        envgen_[i].render();
        envgenMB_[i][0].render();
        envgenMB_[i][1].render();
        envgenMB_[i][2].render();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocoderAudioProcessor();
}