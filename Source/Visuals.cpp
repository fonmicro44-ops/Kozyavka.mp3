#include "Visuals.h"

juce::Font sansFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font monoFont (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

juce::Colour heatColour (float value)
{
    static const auto lut = []
    {
        juce::ColourGradient g (juce::Colour (0xff030206), 0.0f, 0.0f, juce::Colour (0xfffffbe6), 1.0f, 0.0f, false);
        g.addColour (0.25, juce::Colour (0xff23093f));
        g.addColour (0.48, juce::Colour (0xff6c1a99));
        g.addColour (0.68, juce::Colour (0xffff3d7f));
        g.addColour (0.85, juce::Colour (0xffffb347));
        std::array<juce::Colour, 256> table;
        for (size_t i = 0; i < table.size(); ++i)
            table[i] = g.getColourAtPosition ((double) i / 255.0);
        return table;
    }();

    return lut[(size_t) juce::jlimit (0, 255, (int) (value * 255.0f))];
}

//==============================================================================
void GlitchTitle::renderText()
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    textImage = juce::Image (juce::Image::ARGB, getWidth() * 2, getHeight() * 2, true);
    juce::Graphics g (textImage);
    g.addTransform (juce::AffineTransform::scale (2.0f));

    const auto font = sansFont (40.0f).withExtraKerningFactor (0.32f);
    const juce::String name ("kozyavka"), ext (".mp3");
    const float nameWidth = juce::GlyphArrangement::getStringWidth (font, name);
    const auto area = getLocalBounds().toFloat();

    g.setFont (font);
    g.setColour (juce::Colours::white);
    g.drawText (name, area, juce::Justification::centredLeft);
    g.setColour (accent);
    g.drawText (ext, area.withTrimmedLeft (nameWidth), juce::Justification::centredLeft);
}

void GlitchTitle::tick()
{
    if (glitchFrames > 0)
    {
        --glitchFrames;
        split = 1.0f + rng.nextFloat() * 6.0f;
        for (auto& o : sliceOffsets)
            o = rng.nextFloat() < 0.5f ? rng.nextInt ({ -16, 17 }) : 0;
        repaint();
    }
    else if (rng.nextFloat() < 0.012f)   // в среднем раз в ~3 секунды
    {
        glitchFrames = 3 + rng.nextInt (6);
    }
}

void GlitchTitle::paint (juce::Graphics& g)
{
    if (! textImage.isValid())
        return;

    const auto bounds = getLocalBounds().toFloat();
    const auto fit = juce::RectanglePlacement::stretchToFit;

    // Мягкое свечение
    g.setColour (accent.withAlpha (0.10f));
    for (auto offset : { juce::Point<float> (-2, 0), { 2, 0 }, { 0, -2 }, { 0, 2 } })
        g.drawImage (textImage, bounds + offset, fit, true);

    g.setOpacity (1.0f);   // свечение рисовалось полупрозрачным — возвращаем полную яркость

    if (glitchFrames == 0)
    {
        g.drawImage (textImage, bounds, fit);
        return;
    }

    // Сбой: цветные «тени» и сдвинутые полосы
    g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.75f));
    g.drawImage (textImage, bounds.translated (-split, 0.0f), fit, true);
    g.setColour (juce::Colour (0xffff2d95).withAlpha (0.75f));
    g.drawImage (textImage, bounds.translated (split, 0.0f), fit, true);

    g.setOpacity (1.0f);
    const int slices = (int) sliceOffsets.size();
    const int sliceHeight = getHeight() / slices + 1;
    for (int k = 0; k < slices; ++k)
        g.drawImage (textImage, sliceOffsets[(size_t) k], k * sliceHeight, getWidth(), sliceHeight,
                     0, k * sliceHeight * 2, textImage.getWidth(), sliceHeight * 2);
}

//==============================================================================
WaveMorphView::WaveMorphView (KozyavkaProcessor& p) : processor (p)
{
    for (int i = 0; i < 3; ++i)
    {
        const char* names[] { "wave", "coarse", "fine", "level" };
        for (size_t k = 0; k < 4; ++k)
            osc[(size_t) i][k] = p.apvts.getRawParameterValue (Params::osc (i, names[k]));
    }
    smpLevel = p.apvts.getRawParameterValue (Params::smpLevel);
}

float WaveMorphView::waveValue (int wave, float phase)
{
    switch (wave)
    {
        case 0:  return std::sin (juce::MathConstants<float>::twoPi * phase);
        case 1:  return 4.0f * std::abs (phase - 0.5f) - 1.0f;
        case 2:  return 2.0f * phase - 1.0f;
        case 3:  return phase < 0.5f ? 1.0f : -1.0f;
        default: return rng.nextFloat() * 2.0f - 1.0f;
    }
}

void WaveMorphView::tick()
{
    time += 1.0f / 30.0f;
    blink += 0.35f;
    const float target = processor.guiActivity.load (std::memory_order_relaxed);
    activity += (target - activity) * 0.25f;
    repaint();
}

void WaveMorphView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Theme::panel.withAlpha (0.85f));
    g.fillRoundedRectangle (b, 4.0f);

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    for (float x = b.getX() + 20.0f; x < b.getRight(); x += 20.0f)
        g.drawVerticalLine ((int) x, b.getY(), b.getBottom());

    auto area = b.reduced (10.0f, 22.0f);
    g.setColour (accent.withAlpha (0.18f));
    g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

    g.setFont (monoFont (10.0f));
    g.setColour (Theme::textDim);
    g.drawText ("[ osc morph ]", b.reduced (8.0f, 4.0f).removeFromTop (12.0f), juce::Justification::centredLeft);

    // Индикатор «живого» звука мигает, когда играют ноты
    const float pulse = activity * (0.6f + 0.4f * std::sin (blink));
    g.setColour (juce::Colour (0xffff3355).withAlpha (0.2f + 0.8f * pulse));
    g.fillEllipse (b.getRight() - 44.0f, b.getY() + 7.0f, 6.0f, 6.0f);
    g.setColour (Theme::textDim);
    g.drawText ("live", juce::Rectangle<float> (b.getRight() - 34.0f, b.getY() + 4.0f, 30.0f, 12.0f), juce::Justification::centredLeft);

    float levels[3], ratios[3], total = 0.0f;
    int waves[3];
    for (size_t i = 0; i < 3; ++i)
    {
        waves[i]  = (int) osc[i][0]->load();
        ratios[i] = std::exp2 ((osc[i][1]->load() + osc[i][2]->load() * 0.01f) / 12.0f);
        levels[i] = osc[i][3]->load();
        total += levels[i];
    }

    if (total < 0.001f)
    {
        g.setColour (accent.withAlpha (0.5f));
        g.drawText (smpLevel->load() > 0.0f ? "sample only" : "silence", area, juce::Justification::centred);
        return;
    }

    constexpr int points = 160;
    const float amp = area.getHeight() * 0.45f * (0.7f + 0.3f * activity);

    // Несколько запаздывающих копий дают «шлейф» движения
    for (int trail = 3; trail >= 0; --trail)
    {
        const float t = time - (float) trail * 0.07f;
        juce::Path path;

        for (int i = 0; i <= points; ++i)
        {
            const float x = (float) i / points;
            const float base = x * 2.0f + t * 0.25f;
            float v = 0.0f;
            for (int o = 0; o < 3; ++o)
                if (levels[o] > 0.0f)
                {
                    float ph = base * ratios[o] + 0.17f * (float) o;
                    ph -= std::floor (ph);
                    v += levels[o] * waveValue (waves[o], ph);
                }
            v /= total;
            v *= 1.0f + 0.06f * std::sin (t * 1.7f + x * 6.0f);   // лёгкое «дыхание»

            const juce::Point<float> pt (area.getX() + x * area.getWidth(), area.getCentreY() - v * amp);
            if (i == 0) path.startNewSubPath (pt);
            else        path.lineTo (pt);
        }

        if (trail == 0)
        {
            g.setColour (accent.withAlpha (0.18f));
            g.strokePath (path, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (accent);
            g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            g.setColour (accent.withAlpha (0.16f / (float) trail));
            g.strokePath (path, juce::PathStrokeType (1.2f));
        }
    }

    if (smpLevel->load() > 0.0f)
    {
        g.setFont (monoFont (10.0f));
        g.setColour (Theme::lime.withAlpha (0.8f));
        g.drawText ("+ smp", b.reduced (8.0f, 4.0f).removeFromBottom (12.0f), juce::Justification::centredRight);
    }
}

//==============================================================================
AnalyzerView::AnalyzerView (KozyavkaProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void AnalyzerView::resized()
{
    const int w = std::max (1, getWidth()), h = std::max (1, getHeight());
    waterfallImage = juce::Image (juce::Image::RGB, w, h, true);
    waterfallImage.clear (waterfallImage.getBounds(), juce::Colour (0xff030206));
    phosphorImage = juce::Image (juce::Image::RGB, w, h, true);
    phosphorImage.clear (phosphorImage.getBounds(), Theme::panel);
    smoothed.assign ((size_t) w, 0.0f);
    peaks.assign ((size_t) w, 0.0f);
}

void AnalyzerView::mouseUp (const juce::MouseEvent&)
{
    mode = (mode + 1) % numModes;
    repaint();
}

void AnalyzerView::tick (bool visible)
{
    // Забираем свежий звук всегда, чтобы буфер не переполнялся
    const int got = processor.tap.pull (incomingL.data(), incomingR.data(), AudioTap::size);
    if (got >= fftSize)
    {
        std::copy (incomingL.begin() + (got - fftSize), incomingL.begin() + got, historyL.begin());
        std::copy (incomingR.begin() + (got - fftSize), incomingR.begin() + got, historyR.begin());
    }
    else if (got > 0)
    {
        std::copy (historyL.begin() + got, historyL.end(), historyL.begin());
        std::copy (historyR.begin() + got, historyR.end(), historyR.begin());
        std::copy (incomingL.begin(), incomingL.begin() + got, historyL.end() - got);
        std::copy (incomingR.begin(), incomingR.begin() + got, historyR.end() - got);
    }

    if (! visible || getWidth() <= 1)
        return;

    computeSpectrum();

    if (mode == waterfall)            drawWaterfallColumns (2);
    else if (mode == scope || mode == lissajous) drawPhosphor();

    repaint();
}

void AnalyzerView::computeSpectrum()
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);
    for (int i = 0; i < fftSize; ++i)
        fftData[(size_t) i] = 0.5f * (historyL[(size_t) i] + historyR[(size_t) i]);

    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    for (size_t k = 0; k < magnitudes.size(); ++k)
        magnitudes[k] = fftData[k] / (fftSize * 0.25f);

    const int w = getWidth();
    for (int x = 0; x < w; ++x)
    {
        const float hz = 25.0f * std::pow (20000.0f / 25.0f, (float) x / (float) (w - 1));
        const float db = juce::Decibels::gainToDecibels (magnitudeAt (hz), -100.0f);
        const float v = juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 84.0f);
        smoothed[(size_t) x] = std::max (v, smoothed[(size_t) x] * 0.88f);
        peaks[(size_t) x] = std::max (v, peaks[(size_t) x] - 0.008f);
    }
}

float AnalyzerView::magnitudeAt (float hz) const
{
    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    const float bin = hz * (float) fftSize / (float) sr;
    const int i = juce::jlimit (0, (int) magnitudes.size() - 2, (int) bin);
    const float frac = juce::jlimit (0.0f, 1.0f, bin - (float) i);
    return magnitudes[(size_t) i] + frac * (magnitudes[(size_t) i + 1] - magnitudes[(size_t) i]);
}

void AnalyzerView::drawWaterfallColumns (int columns)
{
    const int w = waterfallImage.getWidth(), h = waterfallImage.getHeight();
    waterfallImage.moveImageSection (0, 0, columns, 0, w - columns, h);

    for (int y = 0; y < h; ++y)
    {
        const float hz = 25.0f * std::pow (20000.0f / 25.0f, 1.0f - (float) y / (float) (h - 1));
        const float db = juce::Decibels::gainToDecibels (magnitudeAt (hz), -100.0f);
        const auto colour = heatColour (juce::jlimit (0.0f, 1.0f, (db + 96.0f) / 90.0f));
        for (int x = w - columns; x < w; ++x)
            waterfallImage.setPixelAt (x, y, colour);
    }
}

void AnalyzerView::drawPhosphor()
{
    juce::Graphics g (phosphorImage);
    const auto b = phosphorImage.getBounds().toFloat();

    // Люминофор гаснет постепенно, поэтому остаются светящиеся следы
    g.setColour (Theme::panel.withAlpha (0.3f));
    g.fillRect (b);

    if (mode == scope)
    {
        // Синхронизация по переходу через ноль вверх — картинка не «бежит»
        int start = 0;
        for (int i = 1; i < fftSize / 2; ++i)
            if (historyL[(size_t) i - 1] <= 0.0f && historyL[(size_t) i] > 0.0f) { start = i; break; }

        juce::Path path;
        const int span = 1024;
        for (int i = 0; i < span; i += 2)
        {
            const float v = 0.5f * (historyL[(size_t) (start + i)] + historyR[(size_t) (start + i)]);
            const juce::Point<float> pt (b.getX() + b.getWidth() * (float) i / span,
                                         b.getCentreY() - juce::jlimit (-1.0f, 1.0f, v * 2.5f) * b.getHeight() * 0.45f);
            if (i == 0) path.startNewSubPath (pt);
            else        path.lineTo (pt);
        }
        g.setColour (accent.withAlpha (0.35f));
        g.strokePath (path, juce::PathStrokeType (4.0f));
        g.setColour (accent);
        g.strokePath (path, juce::PathStrokeType (1.4f));
    }
    else
    {
        // Гониометр: по горизонтали — стерео-ширина, по вертикали — моно
        const float scale = b.getHeight() * 0.9f;
        g.setColour (accent.withAlpha (0.55f));
        for (int i = fftSize - 1024; i < fftSize; i += 2)
        {
            const float l = historyL[(size_t) i], r = historyR[(size_t) i];
            const float x = b.getCentreX() + (l - r) * 0.7071f * scale;
            const float y = b.getCentreY() - (l + r) * 0.7071f * scale * 0.5f;
            g.fillRect (x, y, 1.6f, 1.6f);
        }
    }
}

void AnalyzerView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    juce::Path clip;
    clip.addRoundedRectangle (b, 4.0f);
    g.saveState();
    g.reduceClipRegion (clip);

    switch (mode)
    {
        case waterfall:
        {
            g.drawImageAt (waterfallImage, 0, 0);
            g.setFont (monoFont (9.0f));
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            for (float hz : { 100.0f, 1000.0f, 10000.0f })
            {
                const float y = b.getHeight() * (1.0f - std::log (hz / 25.0f) / std::log (20000.0f / 25.0f));
                g.drawText (hz >= 1000.0f ? juce::String ((int) (hz / 1000.0f)) + "k" : juce::String ((int) hz),
                            juce::Rectangle<float> (b.getRight() - 30.0f, y - 6.0f, 26.0f, 12.0f), juce::Justification::centredRight);
            }
            break;
        }

        case spectrum:
        {
            g.setColour (Theme::panel);
            g.fillRect (b);
            juce::Path line, fill;
            const int w = getWidth();
            for (int x = 0; x < w; ++x)
            {
                const float y = b.getBottom() - smoothed[(size_t) x] * b.getHeight() * 0.9f;
                if (x == 0) { line.startNewSubPath ((float) x, y); fill.startNewSubPath ((float) x, b.getBottom()); }
                line.lineTo ((float) x, y);
                fill.lineTo ((float) x, y);
            }
            fill.lineTo (b.getRight(), b.getBottom());
            fill.closeSubPath();

            g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.45f), 0.0f, b.getY(),
                                                     accent.withAlpha (0.0f), 0.0f, b.getBottom(), false));
            g.fillPath (fill);
            g.setColour (accent);
            g.strokePath (line, juce::PathStrokeType (1.4f));

            g.setColour (juce::Colours::white.withAlpha (0.6f));
            for (int x = 0; x < w; x += 3)
                g.fillRect ((float) x, b.getBottom() - peaks[(size_t) x] * b.getHeight() * 0.9f, 2.0f, 1.0f);
            break;
        }

        default:
            g.drawImageAt (phosphorImage, 0, 0);
            break;
    }

    g.restoreState();

    static const char* names[] { "waterfall", "spectrum", "scope", "lissajous" };
    g.setFont (monoFont (10.0f));
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawText ("[ " + juce::String (names[mode]) + " ]  click: next view", b.reduced (8.0f, 4.0f).removeFromTop (12.0f),
                juce::Justification::centredLeft);

    g.setColour (accent.withAlpha (0.2f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
}

//==============================================================================
SampleView::SampleView (KozyavkaProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void SampleView::mouseUp (const juce::MouseEvent&)
{
    if (onLoadRequest)
        onLoadRequest();
}

void SampleView::rebuild (const SampleData& data)
{
    const int w = std::max (1, getWidth()), h = std::max (2, getHeight());
    const int waveHeight = (int) (h * 0.42f), specHeight = h - waveHeight;

    // Волна
    waveImage = juce::Image (juce::Image::ARGB, w, waveHeight, true);
    {
        juce::Graphics g (waveImage);
        const int len = data.length();
        const float mid = waveHeight * 0.5f;
        for (int x = 0; x < w; ++x)
        {
            const int from = (int) ((int64_t) x * len / w), to = std::max (from + 1, (int) ((int64_t) (x + 1) * len / w));
            float lo = 0.0f, hi = 0.0f;
            for (int i = from; i < to; ++i)
            {
                lo = std::min (lo, data.audio[(size_t) i]);
                hi = std::max (hi, data.audio[(size_t) i]);
            }
            g.setColour (accent.withAlpha (0.35f + 0.5f * std::min (1.0f, hi - lo)));
            g.drawVerticalLine (x, mid - hi * mid * 0.9f, mid - lo * mid * 0.9f + 1.0f);
        }
    }

    // Спектрограмма сэмпла: звук, превращённый в картинку
    spectrogramImage = juce::Image (juce::Image::RGB, w, specHeight, true);
    float maxMag = 1.0e-6f;
    for (size_t i = 0; i < data.mags.size(); i += 7)
        maxMag = std::max (maxMag, data.mags[i]);

    for (int x = 0; x < w; ++x)
    {
        const int frame = juce::jlimit (0, data.numFrames - 1, x * data.numFrames / w);
        const float* mags = data.frameMags (frame);
        for (int y = 0; y < specHeight; ++y)
        {
            const float hz = 30.0f * std::pow (16000.0f / 30.0f, 1.0f - (float) y / (float) (specHeight - 1));
            const int bin = juce::jlimit (1, SampleData::numBins - 1, (int) (hz * SampleData::fftSize / data.sampleRate));
            const float db = juce::Decibels::gainToDecibels (mags[bin] / maxMag, -100.0f);
            spectrogramImage.setPixelAt (x, y, heatColour (juce::jlimit (0.0f, 1.0f, (db + 80.0f) / 80.0f)));
        }
    }
}

void SampleView::tick()
{
    const SampleData* current = processor.getCurrentSample();
    if (current != cached)
    {
        cached = current;
        if (current != nullptr)
            rebuild (*current);
    }

    // Новые гранулы → частицы, которые взлетают и гаснут
    auto& feed = processor.visualFeed;
    const auto write = feed.grainWrite.load (std::memory_order_relaxed);
    const uint32_t fresh = std::min<uint32_t> (write - lastGrain, SampleVisualFeed::grainSlots);
    for (uint32_t i = write - fresh; i != write; ++i)
    {
        const float pos = feed.grainPos[i % SampleVisualFeed::grainSlots].load (std::memory_order_relaxed);
        if (pos >= 0.0f)
            particles.push_back ({ pos, 0.08f + rng.nextFloat() * 0.3f, 0.002f + rng.nextFloat() * 0.006f, 0.0f });
    }
    lastGrain = write;

    for (auto& p : particles)
    {
        p.age += 1.0f / 30.0f;
        p.y -= p.vy;
    }
    particles.erase (std::remove_if (particles.begin(), particles.end(), [] (const Particle& p) { return p.age > 1.2f || p.y < 0.0f; }),
                     particles.end());
    if (particles.size() > 300)
        particles.erase (particles.begin(), particles.begin() + (long) (particles.size() - 300));

    repaint();
}

void SampleView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (b, 4.0f);

    g.setFont (monoFont (10.0f));

    if (cached == nullptr)
    {
        g.setColour (accent.withAlpha (0.7f));
        g.drawText ("no sample - drop an audio file here or click to load", b, juce::Justification::centred);
        g.setColour (accent.withAlpha (0.2f));
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
        return;
    }

    const float waveHeight = (float) waveImage.getHeight();
    g.drawImageAt (waveImage, 0, 0);
    g.drawImageAt (spectrogramImage, 0, (int) waveHeight);

    // Стартовая позиция
    const float position = processor.apvts.getRawParameterValue (Params::smpPosition)->load();
    const float px = b.getX() + position * b.getWidth();
    g.setColour (accent.withAlpha (0.7f));
    for (float y = 0.0f; y < b.getHeight(); y += 6.0f)
        g.drawVerticalLine ((int) px, y, y + 3.0f);
    juce::Path marker;
    marker.addTriangle (px - 4.0f, 0.0f, px + 4.0f, 0.0f, px, 6.0f);
    g.fillPath (marker);

    // Где сейчас играют голоса
    for (auto& v : processor.visualFeed.voicePos)
    {
        const float pos = v.load (std::memory_order_relaxed);
        if (pos < 0.0f)
            continue;
        const float x = b.getX() + pos * b.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillRect (x - 3.0f, 0.0f, 6.0f, b.getHeight());
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawVerticalLine ((int) x, 0.0f, b.getHeight());
    }

    // Гранулы
    for (const auto& p : particles)
    {
        const float alpha = 1.0f - p.age / 1.2f;
        const float size = 2.0f + 2.5f * alpha;
        g.setColour (accent.brighter (0.6f).withAlpha (alpha));
        g.fillEllipse (b.getX() + p.x * b.getWidth() - size * 0.5f, p.y * b.getHeight() - size * 0.5f, size, size);
    }

    static const char* modes[] { "classic", "granular", "spectral" };
    const int mode = juce::jlimit (0, 2, (int) processor.apvts.getRawParameterValue (Params::smpMode)->load());
    auto label = b.reduced (8.0f, 4.0f).removeFromTop (12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawText ("[ " + cached->name + " ]", label, juce::Justification::centredLeft);
    g.setColour (accent);
    g.drawText (juce::String (modes[mode]) + "  //  click or drop a file to load", label, juce::Justification::centredRight);

    g.setColour (accent.withAlpha (0.2f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
}
