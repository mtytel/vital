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

#include "skin.h"
#include "default_look_and_feel.h"
#include "full_interface.h"
#include "synth_constants.h"
#include "synth_section.h"
#include "wavetable_3d.h"

namespace {
  const std::string kOverrideNames[Skin::kNumSectionOverrides] = {
    "All",
    "Logo",
    "Header",
    "Overlays",
    "Oscillator",
    "Sample",
    "Sub",
    "Filter",
    "Envelope",
    "Lfo",
    "RandomLfo",
    "Voice",
    "Macro",
    "Keyboard",
    "All Effects",
    "Chorus",
    "Compressor",
    "Delay",
    "Distortion",
    "Equalizer",
    "Effects Filter",
    "Flanger",
    "Phaser",
    "Reverb",
    "Modulation Drag Drop",
    "Modulation Matrix",
    "Preset Browser",
    "Popup Browser",
    "Advanced",
    "Wavetable Editor",
  };

  const std::string kValueNames[Skin::kNumSkinValueIds] = {
    "Body Rounding",
    "Label Height",
    "Label Background Height",
    "Label Rounding",
    "Label Offset",
    "Text Component Label Offset",
    "Rotary Option X Offset",
    "Rotary Option Y Offset",
    "Rotary Option Width",
    "Title Width",
    "Padding",
    "Large Padding",
    "Slider Width",
    "Text Component Height",
    "Text Component Offset",
    "Text Component Font Size",
    "Text Button Height",
    "Button Font Size",
    "Knob Arc Size",
    "Knob Arc Thickness",
    "Knob Body Size",
    "Knob Handle Length",
    "Knob Mod Amount Arc Size",
    "Knob Mod Amount Arc Thickness",
    "Knob Mod Meter Arc Size",
    "Knob Mod Meter Arc Thickness",
    "Knob Offset",
    "Knob Section Height",
    "Knob Shadow Width",
    "Knob Shadow Offset",
    "Modulation Button Width",
    "Modulation Font Size",
    "Widget Margin",
    "Widget Rounded Corner",
    "Widget Line Width",
    "Widget Line Boost",
    "Widget Fill Center",
    "Widget Fill Fade",
    "Widget Fill Boost",
    "Wavetable Horizontal Angle",
    "Wavetable Vertical Angle",
    "Wavetable Draw Width",
    "Wavetable Draw Height",
    "Wavetable Y Offset",
  };

  const std::string kColorNames[Skin::kNumColors] = {
    "Background",
    "Body",
    "Body Heading Background",
    "Heading Text",
    "Preset Text",
    "Body Text",
    "Border",
    "Label Background",
    "Label Connection",
    "Power Button On",
    "Power Button Off",

    "Overlay Screen",
    "Lighten Screen",
    "Shadow",
    "Popup Selector Background",
    "Popup Background",
    "Popup Border",

    "Text Component Background",
    "Text Component Text",

    "Rotary Arc",
    "Rotary Arc Disabled",
    "Rotary Arc Unselected",
    "Rotary Arc Unselected Disabled",
    "Rotary Hand",
    "Rotary Body",
    "Rotary Body Border",

    "Linear Slider",
    "Linear Slider Disabled",
    "Linear Slider Unselected",
    "Linear Slider Thumb",
    "Linear Slider Thumb Disabled",

    "Widget Center Line",
    "Widget Primary 1",
    "Widget Primary 2",
    "Widget Primary Disabled",
    "Widget Secondary 1",
    "Widget Secondary 2",
    "Widget Secondary Disabled",
    "Widget Accent 1",
    "Widget Accent 2",
    "Widget Background",

    "Modulation Meter",
    "Modulation Meter Left",
    "Modulation Meter Right",
    "Modulation Meter Control",
    "Modulation Button Selected",
    "Modulation Button Dragging",
    "Modulation Button Unselected",

    "Icon Selector Icon",

    "Icon Button Off",
    "Icon Button Off Hover",
    "Icon Button Off Pressed",
    "Icon Button On",
    "Icon Button On Hover",
    "Icon Button On Pressed",

    "UI Button",
    "UI Button Text",
    "UI Button Hover",
    "UI Button Press",
    "UI Action Button",
    "UI Action Button Hover",
    "UI Action Button Press",

    "Text Editor Background",
    "Text Editor Border",
    "Text Editor Caret",
    "Text Editor Selection"
  };
} // namespace

bool Skin::shouldScaleValue(ValueId value_id) {
  return value_id != kWidgetFillFade && value_id != kWidgetFillBoost &&
         value_id != kWidgetLineBoost && value_id != kKnobHandleLength &&
         value_id != kWidgetFillCenter && value_id != kFrequencyDisplay &&
         value_id != kWavetableHorizontalAngle && value_id != kWavetableVerticalAngle;
}

Skin::Skin() {
  File default_skin = LoadSave::getDefaultSkin();
  if (default_skin.exists()) {
    if (!loadFromFile(default_skin))
      loadDefaultSkin();
  }
  else
    loadDefaultSkin();

  copyValuesToLookAndFeel(DefaultLookAndFeel::instance());
}

void Skin::clearSkin() {
  for (int i = 0; i < kNumSectionOverrides; ++i)
    color_overrides_[i].clear();
  for (int i = 0; i < kNumSectionOverrides; ++i)
    value_overrides_[i].clear();
}

void Skin::loadDefaultSkin() {
  MemoryInputStream skin((const void*)BinaryData::default_vitalskin, BinaryData::default_vitalskinSize, false);
  std::string skin_string = skin.readEntireStreamAsString().toStdString();

  try {
    json data = json::parse(skin_string, nullptr, false);
    jsonToState(data);
  }
  catch (const json::exception& e) {
  }
}

void Skin::setComponentColors(Component* component) const {
  for (int i = 0; i < Skin::kNumColors; ++i) {
    int color_id = i + Skin::kInitialColor;
    Colour color = getColor(static_cast<Skin::ColorId>(color_id));
    component->setColour(color_id, color);
  }
}

void Skin::setComponentColors(Component* component, SectionOverride section_override, bool top_level) const {
  if (top_level) {
    setComponentColors(component);
    return;
  }

  for (int i = kInitialColor; i < kFinalColor; ++i)
    component->removeColour(i);

  for (const auto& color : color_overrides_[section_override])
    component->setColour(color.first, color.second);
}

void Skin::setComponentValues(SynthSection* component) const {
  std::map<ValueId, float> values;
  for (int i = 0; i < kNumSkinValueIds; ++i)
    values[(ValueId)i] = values_[i];

  component->setSkinValues(values);
}

void Skin::setComponentValues(SynthSection* component, SectionOverride section_override, bool top_level) const {
  if (top_level) {
    setComponentValues(component);
    return;
  }
  component->setSkinValues(value_overrides_[section_override]);
}

bool Skin::overridesColor(int section, ColorId color_id) const {
  if (section == kNone)
    return true;

  return color_overrides_[section].count(color_id) > 0;
}

bool Skin::overridesValue(int section, ValueId value_id) const {
  if (section == kNone)
    return true;

  return value_overrides_[section].count(value_id) > 0;
}

void Skin::copyValuesToLookAndFeel(LookAndFeel* look_and_feel) const {
  look_and_feel->setColour(PopupMenu::backgroundColourId, getColor(Skin::kPopupBackground));
  look_and_feel->setColour(PopupMenu::textColourId, getColor(Skin::kBodyText));
  look_and_feel->setColour(TooltipWindow::textColourId, getColor(Skin::kBodyText));

  look_and_feel->setColour(BubbleComponent::backgroundColourId, getColor(Skin::kPopupBackground));
  look_and_feel->setColour(BubbleComponent::outlineColourId, getColor(Skin::kPopupBorder));

  for (int i = kInitialColor; i < kFinalColor; ++i)
    look_and_feel->setColour(i, getColor(static_cast<Skin::ColorId>(i)));
}

Colour Skin::getColor(int section, ColorId color_id) const {
  if (section == kNone)
    return getColor(color_id);

  if (color_overrides_[section].count(color_id))
    return color_overrides_[section].at(color_id);

  return Colours::black;
}

float Skin::getValue(int section, ValueId value_id) const {
  if (value_overrides_[section].count(value_id))
    return value_overrides_[section].at(value_id);

  return getValue(value_id);
}

void Skin::addOverrideColor(int section, ColorId color_id, Colour color) {
  if (section == kNone)
    setColor(color_id, color);
  else
    color_overrides_[section][color_id] = color;
}

void Skin::removeOverrideColor(int section, ColorId color_id) {
  if (section != kNone)
    color_overrides_[section].erase(color_id);
}

void Skin::addOverrideValue(int section, ValueId value_id, float value) {
  if (section == kNone)
    setValue(value_id, value);
  else
    value_overrides_[section][value_id] = value;
}

void Skin::removeOverrideValue(int section, ValueId color_id) {
  if (section != kNone)
    value_overrides_[section].erase(color_id);
}

json Skin::stateToJson() {
  json data;
  for (int i = 0; i < kNumColors; ++i)
    data[kColorNames[i]] = colors_[i].toString().toStdString();

  for (int i = 0; i < kNumSkinValueIds; ++i)
    data[kValueNames[i]] = values_[i];

  json overrides;
  for (int override_index = 0; override_index < kNumSectionOverrides; ++override_index) {
    json override_section;
    for (const auto& color : color_overrides_[override_index]) {
      int index = color.first - Skin::kInitialColor;
      override_section[kColorNames[index]] = color.second.toString().toStdString();
    }

    for (const auto& value : value_overrides_[override_index]) {
      int index = value.first;
      override_section[kValueNames[index]] = value.second;
    }

    overrides[kOverrideNames[override_index]] = override_section;
  }

  data["overrides"] = overrides;
  data["synth_version"] = ProjectInfo::versionNumber;

  return data;
}

String Skin::stateToString() {
  return stateToJson().dump();
}

void Skin::saveToFile(File destination) {
  destination.replaceWithText(stateToString());
}

json Skin::updateJson(json data) {
  int version = 0;
  if (data.count("synth_version"))
    version = data["synth_version"];

  if (version < 0x608) {
    data["Knob Arc Size"] = data["Knob Size"];
    data["Knob Arc Thickness"] = data["Knob Thickness"];
    data["Knob Handle Length"] = data["Knob Handle Radial Amount"];
    data["Knob Mod Amount Arc Size"] = data["Knob Mod Amount Size"];
    data["Knob Mod Amount Arc Thickness"] = data["Knob Mod Amount Thickness"];
    data["Knob Mod Meter Arc Size"] = data["Knob Mod Meter Size"];
    data["Knob Mod Meter Arc Thickness"] = data["Knob Mod Meter Thickness"];
  }

  if (data.count("Widget Fill Boost") == 0)
    data["Widget Fill Boost"] = 1.6f;
  if (data.count("Widget Line Boost") == 0)
    data["Widget Line Boost"] = 1.0f;

  if (version < 0x609)
    data["Modulation Meter"] = Colours::white.toString().toStdString();

  return data;
}

void Skin::jsonToState(json data) {
  clearSkin();
  data = updateJson(data);

  if (data.count("overrides")) {
    json overrides = data["overrides"];
    for (int override_index = 0; override_index < kNumSectionOverrides; ++override_index) {
      std::string name = kOverrideNames[override_index];
      color_overrides_[override_index].clear();
      value_overrides_[override_index].clear();

      if (overrides.count(name) == 0)
        continue;

      json override_section = overrides[name];
      for (int i = 0; i < kNumColors; ++i) {
        if (override_section.count(kColorNames[i])) {
          ColorId color_id = static_cast<Skin::ColorId>(i + Skin::kInitialColor);

          std::string color_string = override_section[kColorNames[i]];
          color_overrides_[override_index][color_id] = Colour::fromString(color_string);
        }
      }

      for (int i = 0; i < kNumSkinValueIds; ++i) {
        if (override_section.count(kValueNames[i])) {
          Skin::ValueId value_id = static_cast<Skin::ValueId>(i);
          float value = override_section[kValueNames[i]];
          value_overrides_[override_index][value_id] = value;
        }
      }
    }
  }

  for (int i = 0; i < kNumColors; ++i) {
    if (data.count(kColorNames[i])) {
      std::string color_string = data[kColorNames[i]];
      colors_[i] = Colour::fromString(color_string);
    }
  }

  for (int i = 0; i < kNumSkinValueIds; ++i) {
    if (data.count(kValueNames[i]))
      values_[i] = data[kValueNames[i]];
    else
      values_[i] = 0.0f;
  }
}

bool Skin::stringToState(String skin_string) {
  try {
    json data = json::parse(skin_string.toStdString(), nullptr, false);
    jsonToState(data);
  }
  catch (const json::exception& e) {
    return false;
  }
  return true;
}

bool Skin::loadFromFile(File source) {
  return stringToState(source.loadFileAsString());
}

class SkinColorPicker : public Component, public Button::Listener, public Slider::Listener, public ChangeListener {
  public:
    static constexpr int kLoadSaveHeight = 20;
    static constexpr int kButtonHeight = 30;

    SkinColorPicker(String name, Skin* skin, FullInterface* full_interface) :
        Component(name), load_button_("Load"), save_button_("Save"),
        override_index_(0), editing_index_(0), skin_(skin), full_interface_(full_interface) {
      addAndMakeVisible(&load_button_);
      load_button_.addListener(this);
      addAndMakeVisible(&save_button_);
      save_button_.addListener(this);

      for (int i = 0; i < Skin::kNumSectionOverrides; ++i)
        addOverrideSection(i);

      addAndMakeVisible(viewport_);
      container_ = new Component("Container");
      viewport_.setViewedComponent(container_);

      for (int i = 0; i < Skin::kNumColors; ++i)
        addColor(i);

      for (int i = 0; i < Skin::kNumSkinValueIds; ++i)
        addValueSlider(i);

      setSliderValues();
      setOverride(override_index_);
    }

    void setSliderValues() {
      for (int i = 0; i < Skin::kNumSkinValueIds; ++i) {
        float value = skin_->getValue(static_cast<Skin::ValueId>(i));
        value_sliders_[i]->setValue(value, NotificationType::dontSendNotification);
      }
    }

    void addOverrideSection(int override_index) {
      override_buttons_.push_back(std::make_unique<TextButton>(kOverrideNames[override_index]));
      size_t index = override_buttons_.size() - 1;
      addAndMakeVisible(override_buttons_[index].get());
      override_buttons_[index]->addListener(this);
    }

    void addColor(int color_index) {
      color_buttons_.push_back(std::make_unique<TextButton>(kColorNames[color_index]));
      size_t index = color_buttons_.size() - 1;
      container_->addAndMakeVisible(color_buttons_[index].get());
      color_buttons_[index]->addListener(this);

      override_toggle_buttons_.push_back(std::make_unique<ToggleButton>(kColorNames[color_index] + " Override"));
      container_->addAndMakeVisible(override_toggle_buttons_[index].get());
      override_toggle_buttons_[index]->addListener(this);
      override_toggle_buttons_[index]->setColour(ToggleButton::tickColourId, Colours::black);
      override_toggle_buttons_[index]->setColour(ToggleButton::tickDisabledColourId, Colours::black);
    }

    void addValueSlider(int value_index) {
      value_sliders_.push_back(std::make_unique<Slider>(kValueNames[value_index]));
      size_t index = value_sliders_.size() - 1;
      container_->addAndMakeVisible(value_sliders_[index].get());
      value_sliders_[index]->setRange(-10000.0f, 10000.0f);
      value_sliders_[index]->setValue(0.0f);
      value_sliders_[index]->setScrollWheelEnabled(false);
      value_sliders_[index]->setSliderStyle(Slider::IncDecButtons);
      value_sliders_[index]->addListener(this);

      value_override_toggle_buttons_.push_back(std::make_unique<ToggleButton>(kValueNames[value_index] + " Override"));
      container_->addAndMakeVisible(value_override_toggle_buttons_[index].get());
      value_override_toggle_buttons_[index]->addListener(this);
      value_override_toggle_buttons_[index]->setColour(ToggleButton::tickColourId, Colours::black);
      value_override_toggle_buttons_[index]->setColour(ToggleButton::tickDisabledColourId, Colours::black);
    }

    void paint(Graphics& g) override {
      g.fillCheckerBoard(getLocalBounds().toFloat(), 20.0f, 20.0f, Colours::grey, Colours::white);

      g.setColour(Colour(0xff444444));
      int x = getWidth() / 3 + kButtonHeight;
      int text_x = 2 * getWidth() / 3;
      int y = -viewport_.getViewPositionY();
      g.fillRect(x, y, 2 * getWidth() / 3, Skin::kNumSkinValueIds * kButtonHeight);

      g.setColour(Colours::white);
      int width = getWidth() / 2;
      for (int i = 0; i < Skin::kNumSkinValueIds; ++i) {
        g.drawText(kValueNames[i], text_x, y, width, kButtonHeight, Justification::centredLeft);
        y += kButtonHeight;
      }
    }

    void resized() override {
      load_button_.setBounds(0, 0, getWidth() / 6, kLoadSaveHeight);
      save_button_.setBounds(getWidth() / 6, 0, getWidth() / 6, kLoadSaveHeight);

      float overrides_y = kLoadSaveHeight * 2;
      float overrides_height = getHeight() - overrides_y;
      float overrides_width = getWidth() / 3;
      for (int i = 0; i < Skin::kNumSectionOverrides; ++i) {
        int override_y = i * overrides_height / Skin::kNumSectionOverrides;
        int override_height = (i + 1) * overrides_height / Skin::kNumSectionOverrides - override_y;
        override_buttons_[i]->setBounds(0, override_y + overrides_y, overrides_width, override_height);
      }

      int y = 0;
      int width = 2 * getWidth() / 3 - 2 * kButtonHeight;
      int slider_height = kButtonHeight * 0.7f;
      int slider_pad = 0.5f * (kButtonHeight - slider_height);

      for (int i = 0; i < Skin::kNumSkinValueIds; ++i) {
        value_sliders_[i]->setBounds(kButtonHeight, y + slider_pad, width / 2, slider_height);
        value_sliders_[i]->setTextBoxStyle(Slider::TextBoxLeft, false, width / 2, slider_height);
        value_override_toggle_buttons_[i]->setBounds(0, y, kButtonHeight, kButtonHeight);
        y += kButtonHeight;
      }

      for (int i = 0; i < color_buttons_.size(); ++i) {
        color_buttons_[i]->setBounds(kButtonHeight, y, width, kButtonHeight);
        override_toggle_buttons_[i]->setBounds(0, y, kButtonHeight, kButtonHeight);
        y += kButtonHeight;
      }

      container_->setBounds(getWidth() / 3, 0, 2 * getWidth() / 3 - 10, y);
      viewport_.setBounds(getWidth() / 3, 0, 2 * getWidth() / 3, getHeight());
    }

    void setOverride(int override_index) {
      override_index_ = override_index;
      for (auto& override_button : override_buttons_)
        override_button->setButtonText(override_button->getName());

      bool show_override = override_index != Skin::kNone;
      for (int i = 0; i < value_override_toggle_buttons_.size(); ++i) {
        value_override_toggle_buttons_[i]->setVisible(show_override);
        Skin::ValueId value_id = static_cast<Skin::ValueId>(i);
        bool overrides = skin_->overridesValue(override_index_, value_id);
        value_override_toggle_buttons_[i]->setToggleState(overrides, dontSendNotification);
      }

      for (int i = 0; i < value_sliders_.size(); ++i) {
        Skin::ValueId value_id = static_cast<Skin::ValueId>(i);
        value_sliders_[i]->setValue(skin_->getValue(override_index_, value_id), dontSendNotification);
      }

      for (int i = 0; i < override_toggle_buttons_.size(); ++i) {
        override_toggle_buttons_[i]->setVisible(show_override);
        Skin::ColorId color_id = static_cast<Skin::ColorId>(Skin::kInitialColor + i);
        bool overrides = skin_->overridesColor(override_index_, color_id);
        override_toggle_buttons_[i]->setToggleState(overrides, dontSendNotification);
      }

      for (int i = 0; i < color_buttons_.size(); ++i) {
        Skin::ColorId color_id = static_cast<Skin::ColorId>(Skin::kInitialColor + i);
        setButtonColor(i, skin_->getColor(override_index_, color_id));
      }

      String new_text = "------ " + override_buttons_[override_index_]->getName() + " ------";
      override_buttons_[override_index_]->setButtonText(new_text);
    }

    void toggleOverride(int color_index) {
      bool visible = override_toggle_buttons_[color_index]->isVisible();
      bool on = override_toggle_buttons_[color_index]->getToggleState();
      Skin::ColorId color_id = static_cast<Skin::ColorId>(Skin::kInitialColor + color_index);
      Colour color = color_buttons_[color_index]->findColour(TextButton::buttonColourId);
      if (on || !visible)
        skin_->addOverrideColor(override_index_, color_id, color);
      else
        skin_->removeOverrideColor(override_index_, color_id);

      repaintWithSettings();
    }

    void toggleValueOverride(int value_index) {
      bool visible = value_override_toggle_buttons_[value_index]->isVisible();
      bool on = value_override_toggle_buttons_[value_index]->getToggleState();
      Skin::ValueId value_id = static_cast<Skin::ValueId>(value_index);
      float value = value_sliders_[value_index]->getValue();
      if (on || !visible)
        skin_->addOverrideValue(override_index_, value_id, value);
      else
        skin_->removeOverrideValue(override_index_, value_id);

      repaintWithSettings();
    }

    void buttonClicked(Button* clicked_button) override {
      if (clicked_button == &load_button_) {
        FileChooser open_box("Open Skin", File(), String("*.") + vital::kSkinExtension);
        if (open_box.browseForFileToOpen()) {
          if (!skin_->loadFromFile(open_box.getResult())) {
            AlertWindow::showNativeDialogBox("Error opening skin", "Skin file is corrupted and won't load.", false);
            return;
          }
          setSliderValues();
          repaintWithSettings();
        }
        return;
      }
      if (clicked_button == &save_button_) {
        FileChooser save_box("Save Skin", File(), String("*.") + vital::kSkinExtension);
        if (save_box.browseForFileToSave(true))
          skin_->saveToFile(save_box.getResult().withFileExtension(vital::kSkinExtension));

        return;
      }

      for (int i = 0; i < override_buttons_.size(); ++i) {
        if (clicked_button == override_buttons_[i].get()) {
          setOverride(i);
          return;
        }
      }

      for (int i = 0; i < value_override_toggle_buttons_.size(); ++i) {
        if (clicked_button == value_override_toggle_buttons_[i].get()) {
          toggleValueOverride(i);
          return;
        }
      }

      for (int i = 0; i < color_buttons_.size(); ++i) {
        if (clicked_button == override_toggle_buttons_[i].get()) {
          toggleOverride(i);
          return;
        }

        if (clicked_button == color_buttons_[i].get())
          editing_index_ = i;
      }

      ColourSelector* color_selector = new ColourSelector();
      color_selector->setCurrentColour(clicked_button->findColour(TextButton::buttonColourId));
      color_selector->addChangeListener(this);
      color_selector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
      color_selector->setSize(300, 400);

      CallOutBox::launchAsynchronously(std::unique_ptr<Component>(color_selector), clicked_button->getScreenBounds(), nullptr);
    }

    void sliderValueChanged(Slider* changed_slider) override {
      for (int i = 0; i < Skin::kNumSkinValueIds; ++i) {
        if (value_sliders_[i].get() == changed_slider) {
          if (value_override_toggle_buttons_[i]->isVisible())
            value_override_toggle_buttons_[i]->setToggleState(true, dontSendNotification);
          toggleValueOverride(i);
        }
      }
    }

    void repaintWithSettings() {
      full_interface_->reloadSkin(*skin_);
    }

    void setButtonColor(int index, Colour color) {
      Colour text_color = color.contrasting(0.9f);
      TextButton* button = color_buttons_[index].get();
      button->setColour(TextButton::buttonColourId, color);
      button->setColour(TextButton::textColourOnId, text_color);
      button->setColour(TextButton::textColourOffId, text_color);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override {
      ColourSelector* selector = dynamic_cast<ColourSelector*>(source);
      if (selector == nullptr)
        return;

      Colour color = selector->getCurrentColour();
      setButtonColor(editing_index_, color);

      if (override_toggle_buttons_[editing_index_]->isVisible())
        override_toggle_buttons_[editing_index_]->setToggleState(true, dontSendNotification);
      toggleOverride(editing_index_);
    }

  protected:
    TextButton load_button_;
    TextButton save_button_;
    std::vector<std::unique_ptr<TextButton>> override_buttons_;
    std::vector<std::unique_ptr<ToggleButton>> override_toggle_buttons_;
    std::vector<std::unique_ptr<ToggleButton>> value_override_toggle_buttons_;
    std::vector<std::unique_ptr<TextButton>> color_buttons_;
    std::vector<std::unique_ptr<Slider>> value_sliders_;
    std::vector<std::unique_ptr<Label>> value_labels_;
    int override_index_;
    int editing_index_;

    Skin* skin_;
    FullInterface* full_interface_;
    Component* container_;
    Viewport viewport_;
};

SkinDesigner::SkinDesigner(Skin* skin, FullInterface* full_interface) :
    DocumentWindow("Skin Designer", Colours::grey, DocumentWindow::closeButton){
  container_ = std::make_unique<SkinColorPicker>("Container", skin, full_interface);
  setContentNonOwned(container_.get(), false);
}

SkinDesigner::~SkinDesigner() = default;
