/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "master_controls_interface.h"

#include "oscillator_advanced_section.h"
#include "oscilloscope.h"
#include "fonts.h"
#include "full_interface.h"
#include "skin.h"
#include "load_save.h"
#include "synth_gui_interface.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "synth_strings.h"
#include "text_look_and_feel.h"
#include "text_selector.h"

namespace {
  const std::string kTuningNames[] = {
    "Default",
    "Just - 7 Limit",
    "Just - 5 Limit",
    "Pythagorean",
    "Custom"
  };

  const std::string kFrequencyDisplayNames[] = {
    "Semitones",
    "Hz"
  };
}

TuningSelector::TuningSelector(String name) : TextSelector(name) {
  setRange(0, kNumTunings, 1.0);
  for (int i = 0; i <= kNumTunings; ++i)
    strings_[i] = kTuningNames[i];

  setStringLookup(strings_);
  setValue(kNumTunings);
}

TuningSelector::~TuningSelector() { }

void TuningSelector::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseDown(e);
    return;
  }

  const std::string* lookup = string_lookup_;
  if (long_lookup_)
    lookup = long_lookup_;

  PopupItems options;

  for (int i = 0; i < kNumTunings; ++i)
    options.addItem(i, lookup[i]);

  options.addItem(-1, "");
  options.addItem(kNumTunings, "Load Tuning File...");

  parent_->showPopupSelector(this, e.getPosition(), options, [=](int selection) { setTuning(selection); });
}

void TuningSelector::valueChanged() {
  TextSelector::valueChanged();
  if (findParentComponentOfClass<SynthGuiInterface>())
    loadTuning(static_cast<TuningStyle>((int)getValue()));
}

void TuningSelector::parentHierarchyChanged() {
  setCustomString(getTuningName().toStdString());
  TextSelector::parentHierarchyChanged();
}

void TuningSelector::setTuning(int tuning) {
  if (tuning != getValue())
    setValue(tuning);
  else if (tuning == kNumTunings)
    loadTuning(kNumTunings);
}

void TuningSelector::loadTuning(TuningStyle tuning) {
  if (tuning == kNumTunings) {
    loadTuningFile();
    return;
  }
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  parent->getSynth()->getTuning()->setName(kTuningNames[tuning]);

  String text;
  StringArray lines;
  switch (tuning) {
    case k7Limit:
      text = String(BinaryData::_7_Limit_scl, BinaryData::_7_Limit_sclSize);
      lines.addTokens(text, "\n", "");
      parent->getSynth()->getTuning()->loadScalaFile(lines);
      break;
    case k5Limit:
      text = String(BinaryData::_5_Limit_scl, BinaryData::_5_Limit_sclSize);
      lines.addTokens(text, "\n", "");
      parent->getSynth()->getTuning()->loadScalaFile(lines);
      break;
    case kPythagorean:
      text = String(BinaryData::Pythagorean_scl, BinaryData::Pythagorean_sclSize);
      lines.addTokens(text, "\n", "");
      parent->getSynth()->getTuning()->loadScalaFile(lines);
      break;
    default:
      parent->getSynth()->getTuning()->setDefaultTuning();
      break;
  }
}

void TuningSelector::loadTuningFile() {
  setCustomString("Custom");
  FileChooser load_box("Load Tuning", File(), Tuning::allFileExtensions());
  if (load_box.browseForFileToOpen())
    loadTuningFile(load_box.getResult());

  setCustomString(getTuningName().toStdString());
}

void TuningSelector::loadTuningFile(const File& file) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  parent->getSynth()->loadTuningFile(file);
}

String TuningSelector::getTuningName() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent) {
    String name = parent->getSynth()->getTuning()->getName();
    if (name.isEmpty())
      return "Default";
    return name;
  }
  return "Custom";
}

class VoiceSettings : public SynthSection {
  public:
    VoiceSettings() : SynthSection("VOICE") {
      setSidewaysHeading(false);

      mpe_enabled_ = std::make_unique<SynthButton>("mpe_enabled");
      addButton(mpe_enabled_.get());
      mpe_enabled_->setLookAndFeel(TextLookAndFeel::instance());
      mpe_enabled_->setButtonText("MPE ENABLED");

      voice_priority_ = std::make_unique<TextSelector>("voice_priority");
      addSlider(voice_priority_.get());
      voice_priority_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      voice_priority_->setLookAndFeel(TextLookAndFeel::instance());
      voice_priority_->setLongStringLookup(strings::kVoicePriorityNames);

      voice_override_ = std::make_unique<TextSelector>("voice_override");
      addSlider(voice_override_.get());
      voice_override_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      voice_override_->setLookAndFeel(TextLookAndFeel::instance());
      voice_override_->setLongStringLookup(strings::kVoiceOverrideNames);

      tuning_ = std::make_unique<TuningSelector>("tuning");
      addAndMakeVisible(tuning_.get());
      addOpenGlComponent(tuning_->getImageComponent());
      addOpenGlComponent(tuning_->getQuadComponent());
      tuning_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      tuning_->setLookAndFeel(TextLookAndFeel::instance());
      tuning_->setLongStringLookup(kTuningNames);

      voice_tune_ = std::make_unique<SynthSlider>("voice_tune");
      addSlider(voice_tune_.get());
      voice_tune_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      voice_tune_->setLookAndFeel(TextLookAndFeel::instance());

      voice_transpose_ = std::make_unique<SynthSlider>("voice_transpose");
      addSlider(voice_transpose_.get());
      voice_transpose_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      voice_transpose_->setLookAndFeel(TextLookAndFeel::instance());
      voice_transpose_->setSensitivity(kTransposeMouseSensitivity);
    }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);

      g.setColour(findColour(Skin::kTextComponentBackground, true));
      g.fillRoundedRectangle(mpe_enabled_->getBounds().toFloat(), findValue(Skin::kLabelBackgroundRounding));

      drawTextComponentBackground(g, voice_priority_->getBounds(), true);
      drawTextComponentBackground(g, voice_override_->getBounds(), true);
      drawTextComponentBackground(g, tuning_->getBounds(), true);
      drawTextComponentBackground(g, voice_tune_->getBounds(), true);
      drawTextComponentBackground(g, voice_transpose_->getBounds(), true);

      setLabelFont(g);
      drawLabelForComponent(g, "NOTE PRIORITY", voice_priority_.get(), true);
      drawLabelForComponent(g, "VOICE OVERRIDE", voice_override_.get(), true);
      drawLabelForComponent(g, "TUNING", tuning_.get(), true);
      drawLabelForComponent(g, "TUNE", voice_tune_.get(), true);
      drawLabelForComponent(g, "TRANSPOSE", voice_transpose_.get(), true);
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void resized() override {
      SynthSection::resized();

      int widget_margin = getWidgetMargin();
      int title_width = getTitleWidth();
      int component_height = getKnobSectionHeight() - widget_margin;
      int width = getWidth() - 2 * widget_margin;
      int x = widget_margin;
      int y = title_width + widget_margin;

      int width_left = (width - widget_margin) / 2;
      int voice_y = getHeight() - widget_margin - component_height;
      voice_tune_->setBounds(x, voice_y, width_left, component_height);

      int x_right = x + width_left + widget_margin;
      int width_right = getWidth() - x_right - widget_margin;
      voice_transpose_->setBounds(x_right, voice_y, width_right, component_height);

      int mpe_height = findValue(Skin::kTextButtonHeight);
      mpe_enabled_->setBounds(x, voice_y - mpe_height - widget_margin, width, mpe_height);

      int remaining_height = mpe_enabled_->getY() - y;
      int override_y = y + remaining_height / 3;
      int tuning_y = y + (2 * remaining_height) / 3;
      voice_priority_->setBounds(x, y, width, override_y - y - widget_margin);
      voice_override_->setBounds(x, override_y, width, tuning_y - override_y - widget_margin);
      tuning_->setBounds(x, tuning_y, width, mpe_enabled_->getY() - tuning_y - widget_margin);
    }

    void buttonClicked(Button* clicked_button) override {
      if (clicked_button == mpe_enabled_.get())
        setMpeEnabled(mpe_enabled_->getToggleState());
      
      SynthSection::buttonClicked(clicked_button);
    }
    
    void setAllValues(vital::control_map& controls) override {
      SynthSection::setAllValues(controls);
      setMpeEnabled(mpe_enabled_->getToggleState());
    }

  private:
    void setMpeEnabled(bool enabled) {
      mpe_enabled_->setToggleState(enabled, NotificationType::dontSendNotification);

      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      if (parent)
        parent->getSynth()->setMpeEnabled(enabled);
    }

    std::unique_ptr<SynthButton> mpe_enabled_;
    std::unique_ptr<TextSelector> voice_priority_;
    std::unique_ptr<TextSelector> voice_override_;
    std::unique_ptr<TuningSelector> tuning_;

    std::unique_ptr<SynthSlider> voice_tune_;
    std::unique_ptr<SynthSlider> voice_transpose_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceSettings)
};

class OversampleSettings : public SynthSection {
  public:
    OversampleSettings() : SynthSection("OVERSAMPLING") {
      setSidewaysHeading(false);

      oversampling_1x_ = std::make_unique<OpenGlToggleButton>("");
      oversampling_1x_->addListener(this);
      oversampling_1x_->setLookAndFeel(TextLookAndFeel::instance());
      oversampling_1x_->setButtonText("1x (Draft)");
      addAndMakeVisible(oversampling_1x_.get());
      addOpenGlComponent(oversampling_1x_->getGlComponent());

      oversampling_2x_ = std::make_unique<OpenGlToggleButton>("");
      oversampling_2x_->addListener(this);
      oversampling_2x_->setLookAndFeel(TextLookAndFeel::instance());
      oversampling_2x_->setButtonText("2x (Recommended)");
      addAndMakeVisible(oversampling_2x_.get());
      addOpenGlComponent(oversampling_2x_->getGlComponent());

      oversampling_4x_ = std::make_unique<OpenGlToggleButton>("");
      oversampling_4x_->addListener(this);
      oversampling_4x_->setLookAndFeel(TextLookAndFeel::instance());
      oversampling_4x_->setButtonText("4x (High CPU)");
      addAndMakeVisible(oversampling_4x_.get());
      addOpenGlComponent(oversampling_4x_->getGlComponent());

      oversampling_8x_ = std::make_unique<OpenGlToggleButton>("");
      oversampling_8x_->addListener(this);
      oversampling_8x_->setLookAndFeel(TextLookAndFeel::instance());
      oversampling_8x_->setButtonText("8x (Ultra CPU)");
      addAndMakeVisible(oversampling_8x_.get());
      addOpenGlComponent(oversampling_8x_->getGlComponent());
    }

    void setAllValues(vital::control_map& controls) override {
      SynthSection::setAllValues(controls);
      setSelectedOversamplingButton(controls["oversampling"]->value());
    }

    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);

      g.setColour(findColour(Skin::kTextComponentBackground, true));
      float rounding = findValue(Skin::kLabelBackgroundRounding);
      g.fillRoundedRectangle(oversampling_1x_->getBounds().toFloat(), rounding);
      g.fillRoundedRectangle(oversampling_2x_->getBounds().toFloat(), rounding);
      g.fillRoundedRectangle(oversampling_4x_->getBounds().toFloat(), rounding);
      g.fillRoundedRectangle(oversampling_8x_->getBounds().toFloat(), rounding);
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void resized() override {
      SynthSection::resized();

      int widget_margin = getWidgetMargin();
      int title_width = getTitleWidth();
      int width = getWidth() - 2 * widget_margin;
      int x = widget_margin;
      int y = title_width + widget_margin;
      int oversampling_bottom = getHeight();
      int oversampling_height = oversampling_bottom - y;
      int oversample_2x_y = y + oversampling_height / 4;
      int oversample_4x_y = y + (2 * oversampling_height) / 4;
      int oversample_8x_y = y + (3 * oversampling_height) / 4;
      oversampling_1x_->setBounds(x, y, width, oversample_2x_y - y - widget_margin);
      oversampling_2x_->setBounds(x, oversample_2x_y, width, oversample_4x_y - oversample_2x_y - widget_margin);
      oversampling_4x_->setBounds(x, oversample_4x_y, width, oversample_8x_y - oversample_4x_y - widget_margin);
      oversampling_8x_->setBounds(x, oversample_8x_y, width, oversampling_bottom - oversample_8x_y - widget_margin);
    }

    void buttonClicked(Button* clicked_button) override {
      if (clicked_button == oversampling_1x_.get())
        setOversamplingAmount(0);
      else if (clicked_button == oversampling_2x_.get())
        setOversamplingAmount(1);
      else if (clicked_button == oversampling_4x_.get())
        setOversamplingAmount(2);
      else if (clicked_button == oversampling_8x_.get())
        setOversamplingAmount(3);
    }

  private:
    void setSelectedOversamplingButton(int oversampling_amount) {
      oversampling_1x_->setToggleState(oversampling_amount == 0, NotificationType::dontSendNotification);
      oversampling_2x_->setToggleState(oversampling_amount == 1, NotificationType::dontSendNotification);
      oversampling_4x_->setToggleState(oversampling_amount == 2, NotificationType::dontSendNotification);
      oversampling_8x_->setToggleState(oversampling_amount == 3, NotificationType::dontSendNotification);
    }

    void setOversamplingAmount(int oversampling_amount) {
      setSelectedOversamplingButton(oversampling_amount);

      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      if (parent) {
        parent->getSynth()->valueChangedInternal("oversampling", oversampling_amount);
        parent->getSynth()->notifyOversamplingChanged();
      }
    }

    std::unique_ptr<OpenGlToggleButton> oversampling_1x_;
    std::unique_ptr<OpenGlToggleButton> oversampling_2x_;
    std::unique_ptr<OpenGlToggleButton> oversampling_4x_;
    std::unique_ptr<OpenGlToggleButton> oversampling_8x_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OversampleSettings)
};

class DisplaySettings : public SynthSection {
  public:
    DisplaySettings() : SynthSection("DISPLAY") {
      setSidewaysHeading(false);

      frequency_display_ = std::make_unique<TextSelector>("frequency_display");
      frequency_display_->setRange(0.0, 1.0, 1.0);
      frequency_display_->setValue(LoadSave::displayHzFrequency());
      addSlider(frequency_display_.get());
      frequency_display_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      frequency_display_->setLookAndFeel(TextLookAndFeel::instance());
      frequency_display_->setStringLookup(kFrequencyDisplayNames);
      frequency_display_->setLongStringLookup(kFrequencyDisplayNames);

      LoadSave::getAllSkins(skins_);
      File default_skin = LoadSave::getDefaultSkin();
      skin_value_ = 0;
      if (default_skin.exists()) {
        String skin_name = LoadSave::getLoadedSkin();
        for (int i = 0; i < skins_.size(); ++i) {
          if (skins_[i].getFileNameWithoutExtension() == skin_name)
            skin_value_ = i + 1;
        }
        if (skin_value_ == 0)
          skin_value_ = skins_.size() + 1;
      }

      short_skin_strings_ = std::make_unique<std::string[]>(skins_.size() + 2);
      long_skin_strings_ = std::make_unique<std::string[]>(skins_.size() + 2);
      short_skin_strings_[0] = "Default";
      long_skin_strings_[0] = "Default";
      short_skin_strings_[skins_.size() + 1] = "Custom";
      long_skin_strings_[skins_.size() + 1] = "Load Custom Skin...";

      for (int i = 0; i < skins_.size(); ++i) {
        short_skin_strings_[i + 1] = skins_[i].getFileNameWithoutExtension().toStdString();
        long_skin_strings_[i + 1] = skins_[i].getFileNameWithoutExtension().toStdString();
      }

      skin_ = std::make_unique<TextSelector>("skin");
      skin_->setRange(0.0, skins_.size() + 1, 1);
      skin_->setValue(skin_value_, dontSendNotification);
      skin_->setScrollEnabled(false);
      addSlider(skin_.get());
      skin_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      skin_->setLookAndFeel(TextLookAndFeel::instance());
      skin_->setStringLookup(short_skin_strings_.get());
      skin_->setLongStringLookup(long_skin_strings_.get());
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void paintBackground(Graphics& g) override {
      SynthSection::paintBackground(g);

      drawTextComponentBackground(g, frequency_display_->getBounds(), true);
      drawTextComponentBackground(g, skin_->getBounds(), true);
      setLabelFont(g);
      drawLabelForComponent(g, "FREQUENCY UNITS", frequency_display_.get(), true);
      drawLabelForComponent(g, "SKIN", skin_.get(), true);
    }

    void resized() override {
      SynthSection::resized();

      int widget_margin = getWidgetMargin();
      int title_width = getTitleWidth();
      int width = getWidth() - 2 * widget_margin;
      int x = widget_margin;

      int y = title_width + widget_margin;
      int bottom = getHeight() - widget_margin;

      int frequency_height = (bottom - y - widget_margin) / 2;

      frequency_display_->setBounds(x, y, width, frequency_height);
      int skin_y = y + frequency_height + widget_margin;
      skin_->setBounds(x, skin_y, width, bottom - skin_y);
    }

    void setDisplayValue(Skin::ValueId id, float value) {
      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      if (parent && parent->getGui())
        parent->getGui()->setSkinValue(id, value);
    }

    void parentHierarchyChanged() override {
      SynthSection::parentHierarchyChanged();
      setDisplayValue(Skin::kFrequencyDisplay, frequency_display_->getValue());
    }

    void loadSkin(Skin& skin) {
      FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
      full_interface->reloadSkin(skin);
    }

    void sliderValueChanged(Slider* changed_slider) override {
      if (changed_slider == frequency_display_.get()) {
        setDisplayValue(Skin::kFrequencyDisplay, frequency_display_->getValue());
        LoadSave::saveDisplayHzFrequency(frequency_display_->getValue() != 0.0f);
      }
      else if (changed_slider == skin_.get()) {
        File default_skin = LoadSave::getDefaultSkin();
        if (skin_->getValue() == 0.0f) {
          if (default_skin.exists() && default_skin.hasWriteAccess())
            default_skin.deleteFile();

          Skin skin;
          skin.loadDefaultSkin();
          loadSkin(skin);
        }
        else if (skin_->getValue() == skins_.size() + 1) {
          FileChooser open_box("Open Skin", File(), String("*.") + vital::kSkinExtension);
          if (open_box.browseForFileToOpen()) {
            File skin_file = open_box.getResult();
            skin_file.copyFileTo(LoadSave::getDefaultSkin());
            Skin skin;

            skin.loadFromFile(skin_file);
            loadSkin(skin);
          }
        }
        else {
          Skin skin;
          File skin_file = skins_[skin_->getValue() - 1];
          if (!skin_file.exists())
            return;

          LoadSave::saveLoadedSkin(skin_file.getFileNameWithoutExtension().toStdString());
          skin_file.copyFileTo(default_skin);
          skin.loadFromFile(skin_file);
          loadSkin(skin);
        }
        skin_value_ = skin_->getValue();
      }
    }

  private:
    std::unique_ptr<TextSelector> frequency_display_;
    std::unique_ptr<TextSelector> skin_;
    int skin_value_;

    Array<File> skins_;
    std::unique_ptr<std::string[]> short_skin_strings_;
    std::unique_ptr<std::string[]> long_skin_strings_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisplaySettings)
};

class OutputDisplays : public SynthSection {
  public:
    OutputDisplays() : SynthSection("ANALYSIS") {
      setSidewaysHeading(false);

      oscilloscope_ = std::make_unique<Oscilloscope>();
      addOpenGlComponent(oscilloscope_.get());

      spectrogram_ = std::make_unique<Spectrogram>();
      addOpenGlComponent(spectrogram_.get());
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void resized() override {
      SynthSection::resized();

      int widget_margin = getWidgetMargin();
      int x = widget_margin;
      int width = getWidth() - 2 * widget_margin;
      int oscilloscope_y = getTitleWidth() + widget_margin;
      int oscilloscope_height = (getHeight() - oscilloscope_y) / 2;
      oscilloscope_->setBounds(x, oscilloscope_y, width, oscilloscope_height);

      int spectrogram_y = oscilloscope_->getBottom() + widget_margin;
      int spectrogram_height = getHeight() - spectrogram_y - widget_margin;
      spectrogram_->setBounds(x, spectrogram_y, width, spectrogram_height);
    }

    void setOscilloscopeMemory(const vital::poly_float* memory) {
      oscilloscope_->setOscilloscopeMemory(memory);
    }

    void setAudioMemory(const vital::StereoMemory* memory) {
      spectrogram_->setAudioMemory(memory);
    }

  private:
    std::unique_ptr<Oscilloscope> oscilloscope_;
    std::unique_ptr<Spectrogram> spectrogram_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputDisplays)
};

MasterControlsInterface::MasterControlsInterface(
    const vital::output_map& mono_modulations,
    const vital::output_map& poly_modulations, bool synth) : SynthSection("master_controls") {

  if (synth) {
    for (int i = 0; i < vital::kNumOscillators; ++i) {
      oscillator_advanceds_[i] = std::make_unique<OscillatorAdvancedSection>(i + 1, mono_modulations,
                                                                             poly_modulations);
      addSubSection(oscillator_advanceds_[i].get());
    }
  }

  voice_settings_ = std::make_unique<VoiceSettings>();
  addSubSection(voice_settings_.get());

  oversample_settings_ = std::make_unique<OversampleSettings>();
  addSubSection(oversample_settings_.get());

  display_settings_ = std::make_unique<DisplaySettings>();
  addSubSection(display_settings_.get());

  output_displays_ = std::make_unique<OutputDisplays>();
  addSubSection(output_displays_.get());

  setOpaque(false);
  setSkinOverride(Skin::kAdvanced);
}

MasterControlsInterface::~MasterControlsInterface() { }

void MasterControlsInterface::paintBackground(Graphics& g) {
  paintChildrenBackgrounds(g);
}

void MasterControlsInterface::resized() {
  SynthSection::resized();
  int large_padding = findValue(Skin::kLargePadding);
  int padding = findValue(Skin::kPadding);
  int settings_top = padding;
  if (oscillator_advanceds_[vital::kNumOscillators - 1])
    settings_top = oscillator_advanceds_[vital::kNumOscillators - 1]->getBottom() + large_padding;

  int settings_height = getHeight() - settings_top;
  int panel_width = getWidth() * 0.22f;
  voice_settings_->setBounds(0, settings_top, panel_width, settings_height);
  int oversample_x = voice_settings_->getRight() + padding;
  int display_height = getTitleWidth() + getWidgetMargin() + 1.5 * getKnobSectionHeight();
  int oversample_height = settings_height - display_height - padding;

  oversample_settings_->setBounds(oversample_x, settings_top, panel_width, oversample_height);
  int display_y = oversample_settings_->getBottom() + padding;
  display_settings_->setBounds(oversample_x, display_y, panel_width, display_height);

  int displays_x = display_settings_->getRight() + padding;
  int displays_width = getWidth() - displays_x;
  output_displays_->setBounds(displays_x, settings_top, displays_width, settings_height);
}

void MasterControlsInterface::passOscillatorSection(int index, const OscillatorSection* oscillator) {
  oscillator_advanceds_[index]->passOscillatorSection(oscillator);
}

void MasterControlsInterface::setOscilloscopeMemory(const vital::poly_float* memory) {
  output_displays_->setOscilloscopeMemory(memory);
}

void MasterControlsInterface::setAudioMemory(const vital::StereoMemory* memory) {
  output_displays_->setAudioMemory(memory);
}
