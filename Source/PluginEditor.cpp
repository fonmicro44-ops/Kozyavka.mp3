#include "PluginEditor.h"
#include "Presets.h"

namespace Layout
{
    constexpr int width = 960, height = 660;
    constexpr int knobWidth = 96, knobHeight = 142, groupGap = 36;
    constexpr int stripTop = 104, stripHeight = 120;
    constexpr int rowTops[] { 236, 410 };
    constexpr int footerTop = 592, footerLine = 584;
}

//==============================================================================
FuturaLook::FuturaLook()
{
    setColour (juce::PopupMenu::backgroundColourId, Theme::bgBottom);
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::headerTextColourId, Theme::textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::cyan.withAlpha (0.2f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

juce::Font FuturaLook::getPopupMenuFont()
{
    return sansFont (15.0f);
}

void FuturaLook::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const auto fill   = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto accent = slider.findColour (juce::Slider::thumbColourId);

    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Тонкое неоновое кольцо значения вокруг диска
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (accent.withAlpha (0.12f));
    g.strokePath (track, juce::PathStrokeType (2.0f));

    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float arcStart = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    if (std::abs (angle - arcStart) > 0.01f)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                             std::min (arcStart, angle), std::max (arcStart, angle), true);
        g.setColour (accent.withAlpha (0.18f));
        g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Диск, как у Valhalla: плоский круг с лёгким объёмом
    const float discRadius = radius - 7.0f;
    const auto disc = juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (fill.brighter (0.25f), disc.getTopLeft(),
                                             fill.darker (0.3f), disc.getBottomRight(), false));
    g.fillEllipse (disc);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (disc.reduced (0.5f), 1.0f);

    juce::Path tick;
    tick.addRoundedRectangle (-2.5f, -discRadius + 3.0f, 5.0f, discRadius * 0.34f, 1.5f);
    g.setColour (Theme::tick);
    g.fillPath (tick, juce::AffineTransform::rotation (angle).translated (centre));
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText)
    : label (labelText), attachment (state, paramId, slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (220);
    slider.onValueChange = [this] { repaint (getLocalBounds().removeFromBottom (24)); };

    // Двойной клик возвращает значение по умолчанию
    if (auto* p = state.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

    addAndMakeVisible (slider);
}

void Knob::setColours (juce::Colour fill, juce::Colour accentColour)
{
    accent = accentColour;
    slider.setColour (juce::Slider::rotarySliderFillColourId, fill);
    slider.setColour (juce::Slider::thumbColourId, accentColour);
}

void Knob::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (24).withTrimmedBottom (24));
}

void Knob::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();

    g.setColour (Theme::text);
    g.setFont (sansFont (15.0f));
    g.drawText (label, area.removeFromTop (22), juce::Justification::centred);

    g.setColour (accent.withAlpha (0.85f));
    g.setFont (monoFont (13.0f));
    g.drawText (slider.getTextFromValue (slider.getValue()), area.removeFromBottom (22), juce::Justification::centred);
}

//==============================================================================
ModeSelector::ModeSelector (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                            const juce::String& labelText, const juce::StringArray& itemNames)
    : label (labelText),
      items (itemNames),
      attachment (*state.getParameter (paramId), [this] (float v) { selected = juce::roundToInt (v); repaint(); })
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attachment.sendInitialUpdate();
}

void ModeSelector::paint (juce::Graphics& g)
{
    const auto font = sansFont (15.0f);
    const auto labelText = label + ":  ";
    const int labelWidth = (int) std::ceil (juce::GlyphArrangement::getStringWidth (font, labelText));
    auto area = getLocalBounds();

    g.setFont (font);
    g.setColour (Theme::text);
    g.drawText (labelText, area.removeFromLeft (labelWidth), juce::Justification::centredLeft);

    g.setColour (isMouseOver() ? accent.brighter (0.4f) : accent);
    g.drawText (items[selected].toLowerCase(), area, juce::Justification::centredLeft);
}

void ModeSelector::mouseDown (const juce::MouseEvent&)
{
    juce::PopupMenu menu;
    for (int i = 0; i < items.size(); ++i)
        menu.addItem (i + 1, items[i].toLowerCase(), true, i == selected);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis = juce::Component::SafePointer<ModeSelector> (this)] (int result)
                        {
                            if (safeThis != nullptr && result > 0)
                                safeThis->attachment.setValueAsCompleteGesture ((float) (result - 1));
                        });
}

//==============================================================================
ActionButton::ActionButton (const juce::String& t, std::function<void()> onClick)
    : text (t), clicked (std::move (onClick))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ActionButton::paint (juce::Graphics& g)
{
    g.setFont (small ? monoFont (11.0f) : sansFont (15.0f));
    g.setColour (isMouseOver() ? accent.brighter (0.5f) : accent);
    g.drawText ("[ " + text + " ]", getLocalBounds(), juce::Justification::centredLeft);
}

void ActionButton::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && clicked)
        clicked();
}

//==============================================================================
TabButton::TabButton (const juce::String& t, std::function<void()> onClick)
    : text (t), clicked (std::move (onClick))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void TabButton::setActive (bool shouldBeActive, juce::Colour accentColour)
{
    active = shouldBeActive;
    accent = accentColour;
    repaint();
}

void TabButton::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setFont (sansFont (17.0f).withExtraKerningFactor (0.15f));
    g.setColour (active ? juce::Colours::white : (isMouseOver() ? Theme::text : Theme::textDim));
    g.drawText (text, area.removeFromTop (area.getHeight() - 6.0f), juce::Justification::centred);

    if (active)
    {
        const auto line = area.withSizeKeepingCentre (area.getWidth() * 0.6f, 2.0f);
        g.setColour (accent.withAlpha (0.25f));
        g.fillRoundedRectangle (line.expanded (0.0f, 2.0f), 2.0f);
        g.setColour (accent);
        g.fillRect (line);
    }
}

void TabButton::mouseUp (const juce::MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && clicked)
        clicked();
}

//==============================================================================
PresetBar::PresetBar (KozyavkaProcessor& p) : processor (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PresetBar::refresh()
{
    if (shown != processor.getCurrentProgram())
    {
        shown = processor.getCurrentProgram();
        repaint();
    }
}

void PresetBar::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (Theme::panel.withAlpha (0.8f));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (accent.withAlpha (0.3f));
    g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);

    g.setFont (sansFont (16.0f));
    g.setColour (Theme::text);
    g.drawText ("<", b.withWidth (24.0f), juce::Justification::centred);
    g.drawText (">", b.withTrimmedLeft (b.getWidth() - 24.0f), juce::Justification::centred);

    const auto name = processor.getProgramName (processor.getCurrentProgram());
    const auto category = name.upToFirstOccurrenceOf (" / ", false, false);
    const auto title = name.fromFirstOccurrenceOf (" / ", false, false);
    const auto font = sansFont (15.0f);
    const auto categoryText = title.isEmpty() ? juce::String() : category + "  /  ";
    const float total = juce::GlyphArrangement::getStringWidth (font, categoryText + (title.isEmpty() ? name : title));
    auto area = b.withSizeKeepingCentre (total + 2.0f, b.getHeight());

    g.setFont (font);
    g.setColour (accent.withAlpha (0.8f));
    const float categoryWidth = juce::GlyphArrangement::getStringWidth (font, categoryText);
    g.drawText (categoryText, area.removeFromLeft (categoryWidth), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white);
    g.drawText (title.isEmpty() ? name : title, area, juce::Justification::centredLeft);
}

void PresetBar::select (int index)
{
    const int count = processor.getNumPrograms();
    processor.setCurrentProgram ((index + count) % count);
    refresh();
}

void PresetBar::mouseUp (const juce::MouseEvent& e)
{
    if (e.x < 26)                { select (processor.getCurrentProgram() - 1); return; }
    if (e.x > getWidth() - 26)   { select (processor.getCurrentProgram() + 1); return; }

    juce::PopupMenu menu;
    menu.addItem (1, "init", true, processor.getCurrentProgram() == 0);

    const auto& presets = Presets::all();
    for (const auto& category : Presets::categories)
    {
        juce::PopupMenu sub;
        for (size_t i = 0; i < presets.size(); ++i)
            if (category == presets[i].category)
                sub.addItem ((int) i + 2, presets[i].name, true, processor.getCurrentProgram() == (int) i + 1);
        menu.addSubMenu (category.toLowerCase(), sub);
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis = juce::Component::SafePointer<PresetBar> (this)] (int result)
                        {
                            if (safeThis != nullptr && result > 0)
                                safeThis->select (result - 1);
                        });
}

//==============================================================================
KozyavkaEditor::KozyavkaEditor (KozyavkaProcessor& p)
    : AudioProcessorEditor (p), processorRef (p),
      waveMorph (p), analyzer (p), sampleView (p), presetBar (p),
      animToggle ("anim: on", [this] { processorRef.visualsEnabled = ! processorRef.visualsEnabled; updateTimer(); })
{
    setLookAndFeel (&look);

    auto oscGroup = [this] (int i)
    {
        return Group { "osc " + juce::String (i + 1),
                       { knob (Params::osc (i, "level"),  "volume"),
                         knob (Params::osc (i, "coarse"), "coarse"),
                         knob (Params::osc (i, "fine"),   "fine"),
                         knob (Params::osc (i, "pan"),    "pan") }, {} };
    };

    // osc: три осциллятора и голоса
    auto& osc = pages[0];
    osc.name = "osc";
    osc.accent = Theme::cyan;
    osc.rows[0] = { oscGroup (0), oscGroup (1) };
    osc.rows[1] = { oscGroup (2),
                    Group { "voice", { knob (Params::glide, "glide"), knob (Params::voices, "voices"),
                                       knob (Params::master, "master") }, {} } };
    osc.footer = { mode (Params::osc (0, "wave"), "osc 1", Params::waveNames),
                   mode (Params::osc (1, "wave"), "osc 2", Params::waveNames),
                   mode (Params::osc (2, "wave"), "osc 3", Params::waveNames),
                   mode (Params::harmony, "harmony", Params::harmonyNames) };

    // smp: сэмплер, гранулярный и спектральный режимы
    auto& smp = pages[1];
    smp.name = "smp";
    smp.accent = Theme::lime;
    smp.showsSample = true;
    smp.rows[0] = { Group { "sample", { knob (Params::smpLevel, "volume"), knob (Params::smpCoarse, "coarse"),
                                        knob (Params::smpFine, "fine"), knob (Params::smpPan, "pan") }, {} },
                    Group { "play", { knob (Params::smpPosition, "position"), knob (Params::smpScan, "scan") }, {} } };
    smp.rows[1] = { Group { "granular", { knob (Params::smpGrain, "size"), knob (Params::smpDensity, "density"),
                                          knob (Params::smpSpray, "spray"), knob (Params::smpJitter, "jitter"),
                                          knob (Params::smpReverse, "reverse") }, {}, 1 },
                    Group { "spectral", { knob (Params::smpBlur, "blur"), knob (Params::smpShift, "shift"),
                                          knob (Params::smpSparse, "sparse"), knob (Params::smpWhisper, "whisper") }, {}, 2 } };
    smp.footer = { mode (Params::smpSource, "source", Params::sampleSourceNames),
                   mode (Params::smpMode, "mode", Params::sampleModeNames),
                   mode (Params::smpLoop, "loop", Params::onOffNames),
                   mode (Params::smpKeytrack, "keytrack", Params::onOffNames),
                   action ("load sample", [this] { chooseFile (KozyavkaProcessor::Slot::sample); }) };

    // shape: огибающая, фильтр, LFO, драйв, форманты
    auto& shape = pages[2];
    shape.name = "shape";
    shape.accent = Theme::mint;
    shape.rows[0] = { Group { "envelope", { knob (Params::attack, "attack"), knob (Params::decay, "decay"),
                                            knob (Params::sustain, "sustain"), knob (Params::release, "release") }, {} },
                      Group { "filter", { knob (Params::cutoff, "cutoff"), knob (Params::resonance, "resonance"),
                                          knob (Params::filterEnv, "env") }, {} } };
    shape.rows[1] = { Group { "lfo", { knob (Params::lfoRate, "rate"), knob (Params::lfoAmount, "depth") }, {} },
                      Group { "drive", { knob (Params::drive, "drive") }, {} },
                      Group { "formant", { knob (Params::vowel, "vowel"), knob (Params::vowelMix, "mix") }, {} } };
    shape.footer = { mode (Params::lfoTarget, "lfo target", Params::lfoTargetNames),
                     mode (Params::driveMode, "drive mode", Params::driveModeNames) };

    // fx: биткрашер, ритм, кристаллы, пространство
    auto& fxPage = pages[3];
    fxPage.name = "fx";
    fxPage.accent = Theme::violet;
    fxPage.rows[0] = { Group { "crush", { knob (Params::crushBits, "bits"), knob (Params::crushRate, "rate"),
                                          knob (Params::crushMix, "mix") }, {} },
                       Group { "motion", { knob (Params::motionDepth, "depth") }, {} },
                       Group { "crystal", { knob (Params::crystalMix, "mix"), knob (Params::crystalFeedback, "feedback") }, {} } };
    fxPage.rows[1] = { Group { "space", { knob (Params::spaceMix, "mix"), knob (Params::spaceSize, "size"),
                                          knob (Params::spacePredelay, "predelay"), knob (Params::shimmer, "shimmer") }, {} } };
    fxPage.footer = { mode (Params::motionRate, "motion rate", Params::motionRateNames),
                      mode (Params::motionShape, "motion shape", Params::motionShapeNames),
                      mode (Params::crystalPitch, "crystal pitch", Params::crystalPitchNames),
                      mode (Params::spaceColor, "color mode", Params::spaceColorNames),
                      mode (Params::freeze, "freeze", Params::freezeNames) };

    // atmos: атмосфера
    auto& atmos = pages[4];
    atmos.name = "atmos";
    atmos.accent = Theme::pink;
    atmos.rows[0] = { Group { "texture", { knob (Params::textureAmount, "amount"), knob (Params::textureTone, "tone"),
                                           knob (Params::textureDensity, "density"), knob (Params::textureVariation, "variation") }, {} } };
    atmos.rows[1] = { Group { "stereo", { knob (Params::textureWidth, "width") }, {} },
                      Group { "generator", { knob (Params::textureSeed, "seed") }, {} } };
    atmos.footer = { mode (Params::textureType, "texture", Params::textureNames),
                     mode (Params::textureFollow, "follow", Params::textureFollowNames),
                     action ("regen", [this]
                     {
                         if (auto* seed = processorRef.apvts.getParameter (Params::textureSeed))
                         {
                             seed->beginChangeGesture();
                             seed->setValueNotifyingHost (seed->convertTo0to1 ((float) juce::Random::getSystemRandom().nextInt (1000)));
                             seed->endChangeGesture();
                         }
                     }),
                     action ("load texture", [this] { chooseFile (KozyavkaProcessor::Slot::texture); }) };

    // Диски темнеют → светлеют вдоль ряда, как у Valhalla
    for (auto& page : pages)
    {
        for (auto& row : page.rows)
        {
            int total = 0;
            for (auto& group : row) total += (int) group.knobs.size();

            int index = 0;
            for (auto& group : row)
                for (auto* k : group.knobs)
                    k->setColours (Theme::knobDark.interpolatedWith (Theme::knobLight, (float) index++ / (float) std::max (1, total - 1)),
                                   page.accent);
        }

        for (auto* c : page.footer)
        {
            if (auto* m = dynamic_cast<ModeSelector*> (c))   m->setAccent (page.accent);
            if (auto* a = dynamic_cast<ActionButton*> (c))   a->setAccent (page.accent);
        }
    }

    for (size_t i = 0; i < tabs.size(); ++i)
    {
        tabs[i] = std::make_unique<TabButton> (pages[i].name, [this, i] { setPage ((int) i); });
        addAndMakeVisible (*tabs[i]);
    }

    sampleView.onLoadRequest = [this] { chooseFile (KozyavkaProcessor::Slot::sample); };
    animToggle.setSmall (true);

    for (juce::Component* c : { (juce::Component*) &title, (juce::Component*) &waveMorph, (juce::Component*) &analyzer,
                                (juce::Component*) &sampleView, (juce::Component*) &presetBar, (juce::Component*) &animToggle })
        addAndMakeVisible (c);

    // Набор ручек на вкладке smp зависит от режима сэмплера
    sampleModeWatcher = std::make_unique<juce::ParameterAttachment> (*processorRef.apvts.getParameter (Params::smpMode),
                                                                     [this] (float v)
                                                                     {
                                                                         sampleMode = juce::roundToInt (v);
                                                                         resized();
                                                                         updateVisibility();
                                                                         repaint();
                                                                     });
    sampleModeWatcher->sendInitialUpdate();

    setSize (Layout::width, Layout::height);
    setPage (juce::jlimit (0, (int) pages.size() - 1, processorRef.editorPage));
    updateTimer();
}

KozyavkaEditor::~KozyavkaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

Knob* KozyavkaEditor::knob (const juce::String& paramId, const juce::String& label)
{
    auto k = std::make_unique<Knob> (processorRef.apvts, paramId, label);
    auto* raw = k.get();
    addChildComponent (*raw);
    owned.push_back (std::move (k));
    return raw;
}

ModeSelector* KozyavkaEditor::mode (const juce::String& paramId, const juce::String& label, const juce::StringArray& items)
{
    auto m = std::make_unique<ModeSelector> (processorRef.apvts, paramId, label, items);
    auto* raw = m.get();
    addChildComponent (*raw);
    owned.push_back (std::move (m));
    return raw;
}

ActionButton* KozyavkaEditor::action (const juce::String& text, std::function<void()> onClick)
{
    auto a = std::make_unique<ActionButton> (text, std::move (onClick));
    auto* raw = a.get();
    addChildComponent (*raw);
    owned.push_back (std::move (a));
    return raw;
}

bool KozyavkaEditor::isShown (const Group& group) const
{
    return group.onlyForMode < 0 || group.onlyForMode == sampleMode;
}

void KozyavkaEditor::setPage (int index)
{
    currentPage = index;
    processorRef.editorPage = index;

    const auto& page = pages[(size_t) index];
    title.setAccent (page.accent);
    waveMorph.setAccent (page.accent);
    analyzer.setAccent (page.accent);
    sampleView.setAccent (page.accent);
    presetBar.setAccent (page.accent);
    animToggle.setAccent (page.accent);

    for (size_t p = 0; p < pages.size(); ++p)
        tabs[p]->setActive ((int) p == index, pages[p].accent);

    updateVisibility();
    renderBackground();
    repaint();
}

void KozyavkaEditor::updateVisibility()
{
    for (size_t p = 0; p < pages.size(); ++p)
    {
        const bool visible = (int) p == currentPage;
        for (auto& row : pages[p].rows)
            for (auto& group : row)
                for (auto* k : group.knobs)
                    k->setVisible (visible && isShown (group));
        for (auto* c : pages[p].footer)
            c->setVisible (visible);
    }

    const bool sample = pages[(size_t) currentPage].showsSample;
    sampleView.setVisible (sample);
    analyzer.setVisible (! sample);
}

void KozyavkaEditor::updateTimer()
{
    // Без анимаций таймер нужен только чтобы замечать смену пресета из FL
    startTimerHz (processorRef.visualsEnabled ? 30 : 4);
    animToggle.setText (processorRef.visualsEnabled ? "anim: on" : "anim: off");
}

void KozyavkaEditor::timerCallback()
{
    presetBar.refresh();

    if (! processorRef.visualsEnabled)
    {
        analyzer.tick (false);   // просто забираем звук, чтобы буфер не переполнялся
        return;
    }

    title.tick();
    waveMorph.tick();
    const bool sample = pages[(size_t) currentPage].showsSample;
    analyzer.tick (! sample);
    if (sample)
        sampleView.tick();
}

//==============================================================================
void KozyavkaEditor::chooseFile (KozyavkaProcessor::Slot slot)
{
    chooser = std::make_unique<juce::FileChooser> (slot == KozyavkaProcessor::Slot::sample ? "load a sample" : "load a texture",
                                                   juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, slot] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file.existsAsFile())
                                  loadFile (file, slot);
                          });
}

void KozyavkaEditor::loadFile (const juce::File& file, KozyavkaProcessor::Slot slot)
{
    juce::String error;
    if (! processorRef.loadSampleFile (file, slot, error))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "kozyavka.mp3",
                                                "could not load " + file.getFileName() + ": " + error);
        return;
    }

    if (slot == KozyavkaProcessor::Slot::sample && currentPage != 1)
        setPage (1);
    repaint();
}

bool KozyavkaEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (juce::File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg"))
            return true;
    return false;
}

void KozyavkaEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragHover = false;
    for (const auto& f : files)
    {
        const juce::File file (f);
        if (file.hasFileExtension ("wav;aif;aiff;flac;mp3;ogg"))
        {
            // На вкладке atmos файл становится текстурой, на остальных — сэмплом
            loadFile (file, currentPage == 4 ? KozyavkaProcessor::Slot::texture : KozyavkaProcessor::Slot::sample);
            break;
        }
    }
    repaint();
}

//==============================================================================
void KozyavkaEditor::renderBackground()
{
    if (getWidth() <= 0)
        return;

    const auto accent = pages[(size_t) currentPage].accent;
    const int w = getWidth(), h = getHeight();
    background = juce::Image (juce::Image::RGB, w, h, true);
    juce::Graphics g (background);

    g.setGradientFill (juce::ColourGradient (Theme::bgTop, 0.0f, 0.0f, Theme::bgBottom, 0.0f, (float) h, false));
    g.fillAll();

    // Сетка, «зерно» и строки развёртки, как на старом мониторе
    g.setColour (juce::Colours::white.withAlpha (0.02f));
    for (int x = 0; x < w; x += 24) g.drawVerticalLine (x, 0.0f, (float) h);
    for (int y = 0; y < h; y += 24) g.drawHorizontalLine (y, 0.0f, (float) w);

    juce::Random grain (1337);
    for (int i = 0; i < 4000; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (0.015f + 0.03f * grain.nextFloat()));
        g.fillRect (grain.nextInt (w), grain.nextInt (h), 1, 1);
    }

    g.setColour (juce::Colours::black.withAlpha (0.18f));
    for (int y = 0; y < h; y += 3)
        g.drawHorizontalLine (y, 0.0f, (float) w);

    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.12f), 240.0f, 40.0f,
                                             accent.withAlpha (0.0f), 240.0f, 300.0f, true));
    g.fillRect (0, 0, w, h);

    g.setFont (monoFont (11.0f));
    g.setColour (Theme::textDim);
    g.drawText ("// granular . spectral . texture synth", 26, 76, 320, 14, juce::Justification::centredLeft);

    auto glowLine = [&g, w] (int y, juce::Colour c, float alpha)
    {
        juce::ColourGradient line (c.withAlpha (0.0f), 0.0f, (float) y, c.withAlpha (0.0f), (float) w, (float) y, false);
        line.addColour (0.5, c.withAlpha (alpha));
        g.setGradientFill (line);
        g.fillRect (0, y, w, 1);
    };
    glowLine (96, accent, 0.6f);
    glowLine (Layout::footerLine, juce::Colours::white, 0.08f);

    // Уголки-скобки по краям окна
    g.setColour (accent.withAlpha (0.5f));
    const int c = 14, m = 6;
    for (auto [x, y, dx, dy] : { std::tuple { m, m, 1, 1 }, std::tuple { w - m, m, -1, 1 },
                                 std::tuple { m, h - m, 1, -1 }, std::tuple { w - m, h - m, -1, -1 } })
    {
        g.fillRect (dx > 0 ? x : x - c, y, c, 1);
        g.fillRect (x, dy > 0 ? y : y - c, 1, c);
    }

    g.setFont (monoFont (9.0f));
    g.setColour (Theme::textDim.withAlpha (0.7f));
    g.drawText ("k.mp3 // v" JucePlugin_VersionString " // " + pages[(size_t) currentPage].name + " // drop audio anywhere",
                w - 420, h - 16, 400, 12, juce::Justification::centredRight);
}

void KozyavkaEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImageAt (background, 0, 0);

    // Подписи групп: «osc 1 ───────»
    const auto& page = pages[(size_t) currentPage];
    g.setFont (monoFont (12.0f).withExtraKerningFactor (0.2f));

    for (auto& row : page.rows)
    {
        for (auto& group : row)
        {
            if (! isShown (group))
                continue;

            const auto area = group.area.toFloat();
            const float textWidth = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), group.title) + 10.0f;

            g.setColour (page.accent.withAlpha (0.7f));
            g.drawText (group.title, area.withWidth (textWidth), juce::Justification::centredLeft);

            g.setGradientFill (juce::ColourGradient (page.accent.withAlpha (0.35f), area.getX() + textWidth, 0.0f,
                                                     page.accent.withAlpha (0.0f), area.getRight(), 0.0f, false));
            g.fillRect (juce::Rectangle<float> (area.getX() + textWidth, area.getCentreY(), area.getWidth() - textWidth, 1.0f));
        }
    }

    // Подсказки, где вместо ручек нужно пояснение
    g.setFont (sansFont (14.0f));
    g.setColour (Theme::textDim);
    if (currentPage == 1 && sampleMode == 0)
        g.drawFittedText ("classic: the sample plays like an ordinary sampler.\n"
                          "pitch follows the keys, 'position' sets the start point, 'loop' repeats it.",
                          juce::Rectangle<int> (120, Layout::rowTops[1] + 50, 720, 50), juce::Justification::centred, 2);

    if (currentPage == 4)
    {
        const auto* texture = processorRef.getTextureSample();
        g.drawText ("sample texture: " + (texture != nullptr ? texture->name : juce::String ("-")),
                    Layout::width / 2 - 125, Layout::footerTop + 30, 420, 28, juce::Justification::centredLeft);
    }
}

void KozyavkaEditor::paintOverChildren (juce::Graphics& g)
{
    if (! dragHover)
        return;

    const auto accent = pages[(size_t) currentPage].accent;
    g.setColour (Theme::bgTop.withAlpha (0.7f));
    g.fillRect (getLocalBounds());
    g.setColour (accent);
    g.drawRect (getLocalBounds().reduced (8), 2);
    g.setFont (sansFont (24.0f).withExtraKerningFactor (0.2f));
    g.drawText (currentPage == 4 ? "drop to load as texture" : "drop to load as sample", getLocalBounds(), juce::Justification::centred);
}

void KozyavkaEditor::resized()
{
    title.setBounds (22, 12, 500, 62);
    animToggle.setBounds (300, 73, 110, 20);

    auto tabArea = juce::Rectangle<int> (getWidth() - 24 - 380, 14, 380, 36);
    for (auto& tab : tabs)
        tab->setBounds (tabArea.removeFromLeft (76));
    presetBar.setBounds (getWidth() - 24 - 380, 56, 380, 28);

    waveMorph.setBounds (20, Layout::stripTop, 280, Layout::stripHeight);
    const juce::Rectangle<int> right (312, Layout::stripTop, getWidth() - 332, Layout::stripHeight);
    analyzer.setBounds (right);
    sampleView.setBounds (right);

    // Ряды ручек, отцентрованные по ширине окна
    for (auto& page : pages)
    {
        for (size_t r = 0; r < page.rows.size(); ++r)
        {
            auto& row = page.rows[r];
            int rowWidth = 0, groups = 0;
            for (auto& group : row)
                if (isShown (group))
                {
                    rowWidth += (int) group.knobs.size() * Layout::knobWidth;
                    ++groups;
                }
            rowWidth += Layout::groupGap * std::max (0, groups - 1);

            int x = (getWidth() - rowWidth) / 2;
            for (auto& group : row)
            {
                if (! isShown (group))
                    continue;

                const int groupWidth = (int) group.knobs.size() * Layout::knobWidth;
                group.area = { x + 8, Layout::rowTops[r], groupWidth - 16, 18 };

                for (size_t k = 0; k < group.knobs.size(); ++k)
                    group.knobs[k]->setBounds (x + (int) k * Layout::knobWidth, Layout::rowTops[r] + 22,
                                               Layout::knobWidth, Layout::knobHeight);

                x += groupWidth + Layout::groupGap;
            }
        }

        // Внизу режимы и кнопки: до трёх в строке
        const int columnWidth = 290;
        const int left = (getWidth() - columnWidth * 3) / 2 + 20;
        for (size_t i = 0; i < page.footer.size(); ++i)
            page.footer[i]->setBounds (left + (int) (i % 3) * columnWidth, Layout::footerTop + (int) (i / 3) * 30,
                                       columnWidth - 20, 28);
    }

    renderBackground();
}
