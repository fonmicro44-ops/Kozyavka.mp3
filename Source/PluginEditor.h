#pragma once

#include "PluginProcessor.h"
#include "Visuals.h"

// Футуристичный стиль по мотивам Valhalla: ряды крупных круглых ручек,
// подпись сверху, значение снизу, режимы — текстом «режим: значение».
class FuturaLook : public juce::LookAndFeel_V4
{
public:
    FuturaLook();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getPopupMenuFont() override;
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText);

    void setColours (juce::Colour fill, juce::Colour accent);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    juce::String label;
    juce::Colour accent;
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

//==============================================================================
// «harmony: octave» — клик открывает меню вариантов
class ModeSelector : public juce::Component
{
public:
    ModeSelector (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                  const juce::String& labelText, const juce::StringArray& itemNames);

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

private:
    juce::String label;
    juce::StringArray items;
    juce::Colour accent;
    int selected = 0;
    juce::ParameterAttachment attachment;
};

//==============================================================================
// Текстовая кнопка «[ load sample ]»
class ActionButton : public juce::Component
{
public:
    ActionButton (const juce::String& text, std::function<void()> onClick);

    void setText (const juce::String& t)  { text = t; repaint(); }
    void setAccent (juce::Colour c)       { accent = c; repaint(); }
    void setSmall (bool shouldBeSmall)    { small = shouldBeSmall; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

private:
    bool small = false;
    juce::String text;
    std::function<void()> clicked;
    juce::Colour accent { Theme::cyan };
};

//==============================================================================
class TabButton : public juce::Component
{
public:
    TabButton (const juce::String& text, std::function<void()> onClick);

    void setActive (bool shouldBeActive, juce::Colour accentColour);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { repaint(); }

private:
    juce::String text;
    std::function<void()> clicked;
    bool active = false;
    juce::Colour accent;
};

//==============================================================================
// «‹  pad / night bus  ›» — стрелки листают пресеты, клик по имени открывает меню
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (KozyavkaProcessor&);

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void refresh();
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void select (int index);

    KozyavkaProcessor& processor;
    juce::Colour accent { Theme::cyan };
    int shown = -1;
};

//==============================================================================
class KozyavkaEditor : public juce::AudioProcessorEditor,
                       public juce::FileDragAndDropTarget,
                       private juce::Timer
{
public:
    explicit KozyavkaEditor (KozyavkaProcessor&);
    ~KozyavkaEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override            { dragHover = false; repaint(); }

private:
    struct Group
    {
        juce::String title;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> area;
        int onlyForMode = -1;   // для вкладки smp: группа видна только в этом режиме сэмплера
    };

    struct Page
    {
        juce::String name;
        juce::Colour accent;
        std::array<std::vector<Group>, 2> rows;
        std::vector<juce::Component*> footer;
        bool showsSample = false;
    };

    void timerCallback() override;
    Knob* knob (const juce::String& paramId, const juce::String& label);
    ModeSelector* mode (const juce::String& paramId, const juce::String& label, const juce::StringArray& items);
    ActionButton* action (const juce::String& text, std::function<void()> onClick);
    bool isShown (const Group& group) const;
    void setPage (int index);
    void updateVisibility();
    void renderBackground();
    void chooseFile (KozyavkaProcessor::Slot slot);
    void loadFile (const juce::File& file, KozyavkaProcessor::Slot slot);
    void updateTimer();

    FuturaLook look;
    KozyavkaProcessor& processorRef;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::array<Page, 5> pages;
    std::array<std::unique_ptr<TabButton>, 5> tabs;

    GlitchTitle title;
    WaveMorphView waveMorph;
    AnalyzerView analyzer;
    SampleView sampleView;
    PresetBar presetBar;
    ActionButton animToggle;

    std::unique_ptr<juce::ParameterAttachment> sampleModeWatcher;
    std::unique_ptr<juce::FileChooser> chooser;
    int sampleMode = 1, currentPage = 0;
    bool dragHover = false;
    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KozyavkaEditor)
};
