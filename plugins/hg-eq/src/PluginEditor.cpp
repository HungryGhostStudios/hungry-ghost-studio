#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// SpectrumAnalyzer
//==============================================================================

SpectrumAnalyzer::SpectrumAnalyzer()
{
    fifo.fill(0.0f);
    fftData.fill(0.0f);
    smoothedMagnitudes.fill(-100.0f);
    startTimerHz(30);
}

void SpectrumAnalyzer::pushSamples(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fifo[static_cast<size_t>(fifoIndex)] = data[i];
        ++fifoIndex;
        if (fifoIndex >= fftSize)
        {
            fifoIndex = 0;
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
            nextBlockReady = true;
        }
    }
}

void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    if (nextBlockReady)
    {
        window.multiplyWithWindowingTable(fftData.data(), static_cast<size_t>(fftSize));
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        for (int i = 0; i < fftSize / 2; ++i)
        {
            float mag = fftData[static_cast<size_t>(i)] / static_cast<float>(fftSize);
            float db = 20.0f * std::log10(std::max(mag, 1e-6f));
            smoothedMagnitudes[static_cast<size_t>(i)] =
                smoothedMagnitudes[static_cast<size_t>(i)] * 0.8f + db * 0.2f;
        }
        nextBlockReady = false;
    }

    auto bounds = getLocalBounds().toFloat();
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float minDb = -80.0f;
    const float maxDb = 6.0f;

    juce::Path specPath;
    bool started = false;

    for (int i = 1; i < fftSize / 2; ++i)
    {
        float freq = static_cast<float>(i) * 44100.0f / static_cast<float>(fftSize);
        if (freq < minFreq || freq > maxFreq) continue;

        float x = bounds.getX() +
            std::log2(freq / minFreq) / std::log2(maxFreq / minFreq) * bounds.getWidth();
        float db = smoothedMagnitudes[static_cast<size_t>(i)];
        float y = bounds.getBottom() -
            (db - minDb) / (maxDb - minDb) * bounds.getHeight();
        y = juce::jlimit(bounds.getY(), bounds.getBottom(), y);

        if (!started) { specPath.startNewSubPath(x, y); started = true; }
        else specPath.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xFF42A5F5).withAlpha(0.3f));
    g.strokePath(specPath, juce::PathStrokeType(1.0f));
}

//==============================================================================
// FrequencyResponseDisplay
//==============================================================================

FrequencyResponseDisplay::FrequencyResponseDisplay(HGEqProcessor& proc)
    : processor(proc)
{
    addAndMakeVisible(analyzer);
    startTimerHz(30);
}

void FrequencyResponseDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xFF1A1A2E));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Grid lines
    g.setColour(juce::Colour(0xFF2A2A3E));
    // Frequency grid: 100, 1k, 10k
    for (float freq : {100.0f, 1000.0f, 10000.0f})
    {
        float x = freqToX(freq, bounds.getWidth()) + bounds.getX();
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }
    // dB grid: -18, -12, -6, 0, 6, 12, 18
    for (float db : {-18.0f, -12.0f, -6.0f, 0.0f, 6.0f, 12.0f, 18.0f})
    {
        float y = dbToY(db, bounds.getHeight()) + bounds.getY();
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }

    // Zero line
    g.setColour(juce::Colour(0xFF3A3A4E));
    float zeroY = dbToY(0.0f, bounds.getHeight()) + bounds.getY();
    g.drawHorizontalLine(static_cast<int>(zeroY), bounds.getX(), bounds.getRight());

    // Draw combined EQ curve
    juce::Path eqCurve;
    bool started = false;
    for (float x = 0; x < bounds.getWidth(); x += 1.0f)
    {
        float freq = xToFreq(x, bounds.getWidth());
        float totalDb = 0.0f;

        for (int b = 0; b < HGEqProcessor::numBands; ++b)
        {
            auto& apvts = processor.apvts;
            auto bandId = juce::String(b + 1);
            float enabled = *apvts.getRawParameterValue("band" + bandId + "_enabled");
            if (enabled < 0.5f) continue;

            int type = static_cast<int>(*apvts.getRawParameterValue("band" + bandId + "_type"));
            float f0 = *apvts.getRawParameterValue("band" + bandId + "_freq");
            float gain = *apvts.getRawParameterValue("band" + bandId + "_gain");
            float q = *apvts.getRawParameterValue("band" + bandId + "_q");

            // Approximate magnitude response
            float ratio = freq / f0;
            float logRatio = std::log2(ratio);

            switch (type)
            {
                case 0: // LP
                    totalDb += std::min(0.0f, -12.0f * std::max(0.0f, logRatio) / (1.0f / q));
                    break;
                case 1: // HP
                    totalDb += std::min(0.0f, -12.0f * std::max(0.0f, -logRatio) / (1.0f / q));
                    break;
                case 2: // Low Shelf
                {
                    float transition = 1.0f / (1.0f + std::pow(ratio, 2.0f * q));
                    totalDb += gain * transition;
                    break;
                }
                case 3: // High Shelf
                {
                    float transition = 1.0f / (1.0f + std::pow(1.0f / ratio, 2.0f * q));
                    totalDb += gain * transition;
                    break;
                }
                case 4: // Peak
                {
                    float bw = 1.0f / q;
                    float x2 = logRatio * logRatio;
                    float shape = std::exp(-x2 / (bw * bw * 0.5f));
                    totalDb += gain * shape;
                    break;
                }
                case 5: // Notch
                {
                    float bw = 1.0f / q;
                    float x2 = logRatio * logRatio;
                    float shape = std::exp(-x2 / (bw * bw * 0.5f));
                    totalDb += -30.0f * shape;
                    break;
                }
                case 6: // Allpass — flat magnitude
                    break;
            }
        }

        float px = bounds.getX() + x;
        float py = dbToY(totalDb, bounds.getHeight()) + bounds.getY();
        py = juce::jlimit(bounds.getY(), bounds.getBottom(), py);

        if (!started) { eqCurve.startNewSubPath(px, py); started = true; }
        else eqCurve.lineTo(px, py);
    }

    g.setColour(juce::Colour(0xFF42A5F5));
    g.strokePath(eqCurve, juce::PathStrokeType(2.0f));

    // Draw band nodes
    for (int b = 0; b < HGEqProcessor::numBands; ++b)
    {
        auto bandId = juce::String(b + 1);
        float enabled = *processor.apvts.getRawParameterValue("band" + bandId + "_enabled");
        if (enabled < 0.5f) continue;

        float freq = *processor.apvts.getRawParameterValue("band" + bandId + "_freq");
        float gain = *processor.apvts.getRawParameterValue("band" + bandId + "_gain");

        float nx = freqToX(freq, bounds.getWidth()) + bounds.getX();
        float ny = dbToY(gain, bounds.getHeight()) + bounds.getY();
        ny = juce::jlimit(bounds.getY(), bounds.getBottom(), ny);

        float radius = (b == selectedBand) ? 7.0f : 5.0f;
        juce::Colour nodeColour = (b == selectedBand)
            ? juce::Colour(0xFFFFFFFF)
            : juce::Colour(0xFF42A5F5).brighter(0.3f);

        g.setColour(nodeColour);
        g.fillEllipse(nx - radius, ny - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(juce::Colour(0xFF1A1A2E));
        g.drawEllipse(nx - radius, ny - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // Band number label
        g.setColour(juce::Colour(0xFF1A1A2E));
        g.setFont(10.0f);
        g.drawText(juce::String(b + 1), static_cast<int>(nx - radius),
                   static_cast<int>(ny - radius), static_cast<int>(radius * 2.0f),
                   static_cast<int>(radius * 2.0f), juce::Justification::centred);
    }
}

void FrequencyResponseDisplay::mouseDown(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    float clickX = static_cast<float>(e.x);
    float clickY = static_cast<float>(e.y);

    // Find nearest node
    float minDist = 20.0f;
    selectedBand = -1;

    for (int b = 0; b < HGEqProcessor::numBands; ++b)
    {
        auto bandId = juce::String(b + 1);
        float enabled = *processor.apvts.getRawParameterValue("band" + bandId + "_enabled");
        if (enabled < 0.5f) continue;

        float freq = *processor.apvts.getRawParameterValue("band" + bandId + "_freq");
        float gain = *processor.apvts.getRawParameterValue("band" + bandId + "_gain");

        float nx = freqToX(freq, bounds.getWidth()) + bounds.getX();
        float ny = dbToY(gain, bounds.getHeight()) + bounds.getY();

        float dist = std::sqrt((clickX - nx) * (clickX - nx) + (clickY - ny) * (clickY - ny));
        if (dist < minDist)
        {
            minDist = dist;
            selectedBand = b;
        }
    }

    repaint();
}

void FrequencyResponseDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (selectedBand < 0) return;

    auto bounds = getLocalBounds().toFloat().reduced(2.0f);
    float dragX = static_cast<float>(e.x) - bounds.getX();
    float dragY = static_cast<float>(e.y) - bounds.getY();

    float freq = xToFreq(dragX, bounds.getWidth());
    float gain = yToDb(dragY, bounds.getHeight());

    freq = juce::jlimit(20.0f, 20000.0f, freq);
    gain = juce::jlimit(-24.0f, 24.0f, gain);

    auto bandId = juce::String(selectedBand + 1);
    if (auto* param = processor.apvts.getParameter("band" + bandId + "_freq"))
        param->setValueNotifyingHost(param->convertTo0to1(freq));
    if (auto* param = processor.apvts.getParameter("band" + bandId + "_gain"))
        param->setValueNotifyingHost(param->convertTo0to1(gain));
}

float FrequencyResponseDisplay::freqToX(float freq, float width) const
{
    return std::log2(freq / minFreq) / std::log2(maxFreq / minFreq) * width;
}

float FrequencyResponseDisplay::xToFreq(float x, float width) const
{
    return minFreq * std::pow(maxFreq / minFreq, x / width);
}

float FrequencyResponseDisplay::dbToY(float db, float height) const
{
    return (1.0f - (db - minDb) / (maxDb - minDb)) * height;
}

float FrequencyResponseDisplay::yToDb(float y, float height) const
{
    return maxDb - (y / height) * (maxDb - minDb);
}

//==============================================================================
// HGEqEditor
//==============================================================================

HGEqEditor::HGEqEditor(HGEqProcessor& p)
    : AudioProcessorEditor(p),
      processorRef(p),
      responseDisplay(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(900, 500);

    addAndMakeVisible(responseDisplay);

    auto& apvts = p.apvts;

    // Setup per-band controls
    const juce::StringArray typeNames{"LP", "HP", "Low Shelf", "High Shelf", "Peak", "Notch", "Allpass"};

    for (int b = 0; b < HGEqProcessor::numBands; ++b)
    {
        auto& band = bands[static_cast<size_t>(b)];
        auto id = juce::String(b + 1);

        // Type selector
        band.typeBox.addItemList(typeNames, 1);
        addAndMakeVisible(band.typeBox);
        band.typeAtt = std::make_unique<ComboBoxAttachment>(apvts, "band" + id + "_type", band.typeBox);

        // Freq knob
        setupKnob(band.freqKnob, band.freqLabel, "Freq");
        band.freqAtt = std::make_unique<SliderAttachment>(apvts, "band" + id + "_freq", band.freqKnob);

        // Gain knob
        setupKnob(band.gainKnob, band.gainLabel, "Gain");
        band.gainAtt = std::make_unique<SliderAttachment>(apvts, "band" + id + "_gain", band.gainKnob);

        // Q knob
        setupKnob(band.qKnob, band.qLabel, "Q");
        band.qAtt = std::make_unique<SliderAttachment>(apvts, "band" + id + "_q", band.qKnob);

        // Enable toggle
        addAndMakeVisible(band.enableBtn);
        band.enableAtt = std::make_unique<ButtonAttachment>(apvts, "band" + id + "_enabled", band.enableBtn);
    }

    // Global controls
    msModeBox.addItemList({"Stereo", "Mid", "Side"}, 1);
    addAndMakeVisible(msModeBox);
    msModeAtt = std::make_unique<ComboBoxAttachment>(apvts, "ms_mode", msModeBox);
    msModeLabel.setText("M/S", juce::dontSendNotification);
    msModeLabel.setJustificationType(juce::Justification::centred);
    msModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    addAndMakeVisible(msModeLabel);

    addAndMakeVisible(linearPhaseBtn);
    linearPhaseAtt = std::make_unique<ButtonAttachment>(apvts, "linear_phase", linearPhaseBtn);

    setupKnob(outputGainKnob, outputGainLabel, "Output");
    outputGainAtt = std::make_unique<SliderAttachment>(apvts, "output_gain", outputGainKnob);
}

HGEqEditor::~HGEqEditor()
{
    setLookAndFeel(nullptr);
}

void HGEqEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E1E1E));

    g.setColour(juce::Colour(0xFF42A5F5));
    g.setFont(14.0f);
    g.drawText("HG EQ", 10, 5, 100, 20, juce::Justification::centredLeft);
}

void HGEqEditor::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(22); // title

    // Frequency response display (top 55%)
    auto displayArea = area.removeFromTop(static_cast<int>(area.getHeight() * 0.55f));
    responseDisplay.setBounds(displayArea);
    responseDisplay.getAnalyzer().setBounds(responseDisplay.getLocalBounds().reduced(2));

    area.removeFromTop(6);

    // Band controls area
    auto bandsArea = area.removeFromTop(static_cast<int>(area.getHeight() * 0.8f));
    int bandWidth = bandsArea.getWidth() / HGEqProcessor::numBands;

    for (int b = 0; b < HGEqProcessor::numBands; ++b)
    {
        auto& band = bands[static_cast<size_t>(b)];
        auto col = bandsArea.removeFromLeft(bandWidth).reduced(2);

        // Enable toggle at top
        band.enableBtn.setBounds(col.removeFromTop(20));

        // Type selector
        band.typeBox.setBounds(col.removeFromTop(22).reduced(1, 1));

        col.removeFromTop(2);

        const int knobH = 48;
        const int labelH = 14;

        // Freq
        band.freqLabel.setBounds(col.removeFromTop(labelH));
        band.freqKnob.setBounds(col.removeFromTop(knobH));

        // Gain
        band.gainLabel.setBounds(col.removeFromTop(labelH));
        band.gainKnob.setBounds(col.removeFromTop(knobH));

        // Q
        band.qLabel.setBounds(col.removeFromTop(labelH));
        band.qKnob.setBounds(col.removeFromTop(knobH));
    }

    area.removeFromTop(4);

    // Global controls at bottom
    auto bottomRow = area;
    int controlW = bottomRow.getWidth() / 4;

    auto msCol = bottomRow.removeFromLeft(controlW);
    msModeLabel.setBounds(msCol.removeFromTop(14));
    msModeBox.setBounds(msCol.removeFromTop(24).reduced(4, 0));

    auto lpCol = bottomRow.removeFromLeft(controlW);
    linearPhaseBtn.setBounds(lpCol.removeFromTop(24).reduced(4, 0));

    auto outCol = bottomRow.removeFromLeft(controlW);
    outputGainLabel.setBounds(outCol.removeFromTop(14));
    outputGainKnob.setBounds(outCol.removeFromTop(48));
}

void HGEqEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& text)
{
    knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    addAndMakeVisible(knob);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    label.setFont(juce::Font(11.0f));
    addAndMakeVisible(label);
}
