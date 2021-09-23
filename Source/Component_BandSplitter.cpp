#include "Component_BandSplitter.h"

double myplug::BandSplitView::calcNormalizedLogPos(double hz)
{
    double logStart = log10(minFreq_);
    double logLength = log10(maxFreq_) - logStart;
    return (log10(hz) - logStart) / logLength;
}

double myplug::BandSplitView::calcScreenLogPos(double hz)
{
    return getWidth() * calcNormalizedLogPos(hz);
}

double myplug::BandSplitView::calcHzFromLogScreenPos(double x)
{
    double logStart = log10(minFreq_);
    double logLength = log10(maxFreq_) - logStart;
    return std::pow(10.0, x / getWidth() * logLength + logStart);
}

void myplug::BandSplitView::callListeners()
{
    for (auto& i : listeners_)
    {
        i->selectedBandChanged(this);
        i->splitFrequencyChanged(this);
    }
}

void myplug::BandSplitView::paint(juce::Graphics& g)
{
    auto bgColour = juce::Colour(0.0f, 0.0f, 0.15f, 1.0f);
	g.setColour(bgColour);
	g.fillAll();

    // Draw Spectrum
    juce::Path topline;
    juce::PathStrokeType stroke(1.5, juce::PathStrokeType::JointStyle::curved);
    topline.startNewSubPath(-1.0, getHeight());
    int beginningIndex = (float)fftSize / sampleRate * minFreq_;
    int endingIndex = (float)fftSize / sampleRate * std::clamp<float>(sampleRate / 2.0, 0, maxFreq_);
    if (beginningIndex == 0) ++beginningIndex; // ignore lowest band

    for (int i = beginningIndex; i < endingIndex; ++i)
    {
        float freq = (float)sampleRate / fftSize * i;
        float amplitude = 0.0;
        if (freq == 0) freq = 1;
        for (int bufIndex = 0; bufIndex < recentfftResult.size(); ++bufIndex)
        {
            amplitude += recentfftResult[bufIndex][i];
        }
        if (recentfftResult.size() != 0)
            amplitude /= recentfftResult.size();

        float xPos = calcScreenLogPos(freq);
        float yPos = (amplitude / 60.0 + 1.0) * (-1.0) * getHeight() + getHeight();
        topline.lineTo(xPos, yPos);
    }
      
    g.setColour(juce::Colours::grey);
    g.strokePath(topline, stroke);

    // Draw Band Splitter
    g.setColour(juce::Colours::grey.brighter());
    g.drawLine(calcScreenLogPos(lowmidFreq_), 0.0, calcScreenLogPos(lowmidFreq_), getHeight(), mouseInsectedHandle_ == 0 ? 3.0 : 1.5f);
    g.drawLine(calcScreenLogPos(midhighFreq_), 0.0, calcScreenLogPos(midhighFreq_), getHeight(), mouseInsectedHandle_ == 1 ? 3.0 : 1.5f);

    // Draw BGColour
    g.setColour(juce::Colour(82, 125, 255).withAlpha(currentBand_ == Band::low ? 0.3f : 0.1f).brighter(mouseInsectedBand_ == Band::low ? 0.8f : 0.0f));
    g.fillRect(0.0, 0.0, calcScreenLogPos(lowmidFreq_), getHeight());
    g.setColour(juce::Colour(168, 111, 206).withAlpha(currentBand_ == Band::mid ? 0.3f : 0.1f).brighter(mouseInsectedBand_ == Band::mid ? 0.8f : 0.0f));
    g.fillRect(calcScreenLogPos(lowmidFreq_), 0.0, calcScreenLogPos(midhighFreq_) - calcScreenLogPos(lowmidFreq_), getHeight());
    g.setColour(juce::Colour(252, 86, 156).withAlpha(currentBand_ == Band::high ? 0.3f : 0.1f).brighter(mouseInsectedBand_ == Band::high ? 0.8f : 0.0f));
    g.fillRect(calcScreenLogPos(midhighFreq_), 0.0, getWidth(), getHeight());

    // Draw Freq
    auto font = g.getCurrentFont();
    juce::String lowmidLabel;
    lowmidLabel << static_cast<int>(lowmidFreq_) << "Hz";
    juce::Rectangle<int> lowmidlabel_area = { static_cast<int>(calcScreenLogPos(lowmidFreq_)) + 5, 1, font.getStringWidth(lowmidLabel), 10 };
    if (lowmidlabel_area.getRight() > getWidth())
    {
        lowmidlabel_area.setX(lowmidlabel_area.getX() - (lowmidlabel_area.getRight() - getWidth()) - 5);
    }
    g.setColour(juce::Colours::grey.brighter());
    g.drawText(lowmidLabel, lowmidlabel_area, juce::Justification::centredTop);

    juce::String midhighLabel;
    midhighLabel << static_cast<int>(midhighFreq_) << "Hz";
    juce::Rectangle<int> midhighlabel_area = { static_cast<int>(calcScreenLogPos(midhighFreq_)) + 5, 1, font.getStringWidth(midhighLabel), 10 };
    if (midhighlabel_area.intersectRectangle(lowmidlabel_area))
    {
        midhighlabel_area.setY(midhighlabel_area.getY() + 15);
    }
    if (midhighlabel_area.getRight() > getWidth())
    {
        midhighlabel_area.setX(midhighlabel_area.getX() - (midhighlabel_area.getRight() - getWidth()) - 5);
    }
    g.setColour(juce::Colours::grey.brighter());
    g.drawText(midhighLabel, midhighlabel_area, juce::Justification::centredTop);
}

void myplug::BandSplitView::update()
{
    if (isNextFFTBlockReady_)
    {
        // set fftResult
        int index = 0;
        fftResult.fill(0.0);
        float hzPerBin = (float)sampleRate / fftSize;
        for (auto& i : fftResult)
        {
            // convert to gain & tilting
            float log2coeff = index == 0 ? 0.0f : log2f(hzPerBin * index / 1000.0);
            i = std::clamp<float>(juce::Decibels::gainToDecibels((fftData_[index] + fftData_[fftSize - index - 1]) / (float)fftSize, -200.0f) + log2coeff * 4.5, -60.0, 0.0);
            ++index;
        }

        recentfftResult.push_back(fftResult);
        if (recentfftResult.size() > integralSampleLength)
        {
            recentfftResult.pop_front();
        }
        
        isNextFFTBlockReady_ = false;

        repaint();
    }

    // Update Mouse Cursor
    if (mouseInsectedHandle_ >= 0)
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void myplug::BandSplitView::mouseDown(const juce::MouseEvent& e)
{
    if (mouseInsectedBand_ >= 0)
    {
        currentBand_ = mouseInsectedBand_;
        callListeners();
    }
}

void myplug::BandSplitView::mouseDrag(const juce::MouseEvent& e)
{
    if (mouseInsectedHandle_ == 0)
    {
        lowmidFreq_ = std::clamp<double>(calcHzFromLogScreenPos(e.x), minFreq_, midhighFreq_ - 10);
        callListeners();
    }
    else if (mouseInsectedHandle_ == 1)
    {
        midhighFreq_ = std::clamp<double>(calcHzFromLogScreenPos(e.x), lowmidFreq_ + 10, maxFreq_);
        callListeners();
    }
}

void myplug::BandSplitView::mouseMove(const juce::MouseEvent& e)
{
    if (e.x < calcScreenLogPos(lowmidFreq_))
    {
        mouseInsectedBand_ = Band::low;
    }
    else if (calcScreenLogPos(lowmidFreq_) <= e.x && e.x < calcScreenLogPos(midhighFreq_))
    {
        mouseInsectedBand_ = Band::mid;
    }
    else
    {
        mouseInsectedBand_ = Band::high;
    }

    int xTolerance = 5;
    if (abs(e.x - calcScreenLogPos(lowmidFreq_)) <= xTolerance)
    {
        mouseInsectedHandle_ = 0;
        mouseInsectedBand_ = -1;
    }
    else if (abs(e.x - calcScreenLogPos(midhighFreq_)) <= xTolerance)
    {
        mouseInsectedHandle_ = 1;
        mouseInsectedBand_ = -1;
    }
    else
    {
        mouseInsectedHandle_ = -1;
    }
}

void myplug::BandSplitView::mouseExit(const juce::MouseEvent& e)
{
    mouseInsectedBand_ = -1;
    mouseInsectedHandle_ = -1;
}
