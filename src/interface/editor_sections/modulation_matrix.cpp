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

#include "modulation_matrix.h"

#include "bar_renderer.h"
#include "fonts.h"
#include "curve_look_and_feel.h"
#include "line_map_editor.h"
#include "lfo_section.h"
#include "load_save.h"
#include "modulation_connection_processor.h"
#include "paths.h"
#include "synth_strings.h"
#include "synth_gui_interface.h"
#include "synth_module.h"
#include "text_look_and_feel.h"
#include "text_selector.h"
#include "shaders.h"
#include "synth_slider.h"
#include "synth_parameters.h"

namespace {
  constexpr float kNumberWidthPercent = 0.2f;
  constexpr float kSourceWidthPercent = 0.2f;
  constexpr float kDestinationWidthPercent = 0.2f;
  constexpr float kPaddingWidthPercent = 0.04f;
  constexpr float kMatrixHeightInRows = 12.0f;

  struct SubMenu {
    std::string prefix;
    std::string name;
    bool local_description = false;
  };

  const std::string kNoConnectionString = "-";
  const SubMenu kDestinationSubMenuPrefixes[] = {
    { "", "" },
    { "osc_1_", "Oscillator 1", true },
    { "osc_2_", "Oscillator 2", true },
    { "osc_3_", "Oscillator 3", true },
    { "sample_", "Sample" },
    { "filter_1_", "Filter 1", true },
    { "filter_2_", "Filter 2", true },
    { "filter_fx_", "Filter FX", true },
    { "", "" },
    { "lfo_", "LFOs" },
    { "random_", "Randoms" },
    { "env_", "Envelopes" },
    { "modulation_", "Mod Matrix" },
    { "", "" },
    { "chorus_", "Chorus" },
    { "compressor_", "Compressor" },
    { "delay_", "Delay" },
    { "distortion_", "Distortion" },
    { "phaser_", "Phaser" },
    { "flanger_", "Flanger" },
    { "reverb_", "Reverb" },
    { "delay_", "Delay" },
    { "eq_", "Equalizer" },
  };

  PopupItems createSubMenuForParameterPrefix(const std::string& name, const String& prefix,
                                             const std::vector<String>& parameter_names, bool local) {
    PopupItems items(name);
    int prefix_length = static_cast<int>(prefix.length());
    int index = 0;
    for (const String& parameter_name : parameter_names) {
      if (parameter_name.substring(0, prefix_length) == prefix) {
        std::string display_name;
        if (local)
          display_name = vital::Parameters::getDetails(parameter_name.toStdString()).local_description;
        else
          display_name = vital::Parameters::getDetails(parameter_name.toStdString()).display_name;
        items.addItem(index, display_name);
      }

      index++;
    }
    return items;
  }

  PopupItems createMiscSubMenu(const std::string& name, const std::vector<String>& parameter_names) {
    int index = 0;
    PopupItems items(name);
    for (const String& parameter_name : parameter_names) {
      bool prefix_match = false;
      for (const auto& prefix : kDestinationSubMenuPrefixes) {
        int prefix_length = static_cast<int>(prefix.prefix.length());
        if (prefix_length && parameter_name.substring(0, prefix_length) == String(prefix.prefix))
          prefix_match = true;
      }

      if (!prefix_match && parameter_name != String(kNoConnectionString)) {
        std::string display_name = vital::Parameters::getDetails(parameter_name.toStdString()).display_name;
        items.addItem(index, display_name);
      }
      index++;
    }
    return items;
  }

  bool compareIndexAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->index() < right->index();
  }

  bool compareIndexDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->index() > right->index();
  }

  bool compareSourceAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->source() < right->source();
  }

  bool compareSourceDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->source() > right->source();
  }

  bool compareBipolarAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->bipolar() < right->bipolar();
  }

  bool compareBipolarDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->bipolar() > right->bipolar();
  }

  bool compareStereoAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->stereo() < right->stereo();
  }

  bool compareStereoDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->stereo() > right->stereo();
  }

  bool compareMorphAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->morph() < right->morph();
  }

  bool compareMorphDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->morph() > right->morph();
  }

  bool compareAmountAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->amount() < right->amount();
  }

  bool compareAmountDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->amount() > right->amount();
  }

  bool compareDestinationAscending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->destination() < right->destination();
  }

  bool compareDestinationDescending(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->destination() > right->destination();
  }

  bool compareConnected(const ModulationMatrixRow* left, const ModulationMatrixRow* right) {
    return left->connected() > right->connected();
  }
}

struct NaturalStringComparator {
  bool operator()(const String& first, const String& second) const {
    return first.compareNatural(second) < 0;
  }
};

class BypassButton : public SynthButton {
  public:
    BypassButton(String name, String on, String off) : SynthButton(std::move(name)),
                                                       on_(std::move(on)), off_(std::move(off)) { }

    void buttonStateChanged() override {
      if (getToggleState())
        setText(on_);
      else
        setText(off_);
      ToggleButton::buttonStateChanged();
    }

  private:
    String on_;
    String off_;
};

class ModulationMeterReadouts : public BarRenderer {
  public:
    ModulationMeterReadouts() : BarRenderer(vital::kMaxModulationConnections, false),
                                parent_(nullptr), modulation_amounts_(), scroll_offset_(0), modulation_active_() { }

    void loadAmountOutputs() {
      std::string modulation_prefix = "modulation_amount_";
      for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
        std::string number = std::to_string(i + 1);
        modulation_amounts_[i] = parent_->getSynth()->getStatusOutput(modulation_prefix + number);
      }
    }

    void parentHierarchyChanged() override {
      if (parent_)
        return;

      parent_ = findParentComponentOfClass<SynthGuiInterface>();

      if (parent_ != nullptr)
        loadAmountOutputs();
    }

    void updatePositions(int index) {
      if (parent_ == nullptr)
        return;

      float width = getWidth();
      float height = getHeight();
      setBarWidth(vital::kMaxModulationConnections * modulation_bounds_[0].getHeight() / height);

      for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
        if (modulation_active_[i]) {
          float min_x = 2.0f * modulation_bounds_[i].getX() / width - 1.0f;
          float max_x = 2.0f * modulation_bounds_[i].getRight() / width - 1.0f;
          float y = 1.0f - 2.0f * (modulation_bounds_[i].getBottom() - scroll_offset_) / height;

          float value = modulation_amounts_[i]->value()[index];
          if (value == vital::StatusOutput::kClearValue)
            value = 0.0f;
          float t = vital::utils::clamp(0.5f * (value + 1.0f), 0.0f, 1.0f);
          float x = vital::utils::interpolate(min_x, max_x, t);
          float center_x = (max_x + min_x) / 2.0f;
          positionBar(i, center_x, y, x - center_x, 0.0f);
        }
        else
          positionBar(i, 0.0f, 0.0f, 0.0f, 0.0f);
      }
    }

    void render(OpenGlWrapper& open_gl, bool animate) override {
      if (animate) {
        updatePositions(0);
        setColor(findColour(Skin::kModulationMeterLeft, true));
        BarRenderer::render(open_gl, animate);

        updatePositions(1);
        setColor(findColour(Skin::kModulationMeterRight, true));
        BarRenderer::render(open_gl, animate);
      }
    }

    void paintBackground(Graphics& g) override { }

    void setMeterBounds(int i, Rectangle<int> bounds) {
      modulation_bounds_[i] = bounds;
    }

    void setMeterActive(int i, bool active) {
      modulation_active_[i] = active;
    }

    void setScrollOffset(int offset) {
      scroll_offset_ = offset;
    }

  private:
    SynthGuiInterface* parent_;
    const vital::StatusOutput* modulation_amounts_[vital::kMaxModulationConnections];
    Rectangle<int> modulation_bounds_[vital::kMaxModulationConnections];
    int scroll_offset_;
    bool modulation_active_[vital::kMaxModulationConnections];
};

String ModulationSelector::getTextFromValue(double value) {
  int index = std::round(value);
  return ModulationMatrix::getMenuSourceDisplayName(selections_->at(index));
}

void ModulationSelector::setValueFromName(const String& name, NotificationType notification_type) {
  int value = getValue();
  for (int i = 0; i < selections_->size(); ++i) {
    if (selections_->at(i) == name) {
      if (value != i)
        setValue(i, notification_type);

      redoImage();
      return;
    }
  }
  if (value != 0) {
    setValue(0, notification_type);
    redoImage();
  }
}

void ModulationSelector::mouseDown(const juce::MouseEvent &e) {
  static constexpr int kWideWidth = 420;
  if (dual_menu_) {
    parent_->showDualPopupSelector(this, Point<int>(0, getHeight()), kWideWidth * parent_->getSizeRatio(),
                                   *popup_items_, [=](int selection) { setValue(selection); });
  }
  else {
    parent_->showPopupSelector(this, Point<int>(0, getHeight()), *popup_items_,
                               [=](int selection) { setValue(selection); });
  }
}

ModulationMatrixRow::ModulationMatrixRow(int index, PopupItems* source_items, PopupItems* destination_items,
                                         const std::vector<String>* sources, const std::vector<String>* destinations) :
    SynthSection(String("MOD ") + String(index)), index_(index), connection_(nullptr), parent_(nullptr),
    last_source_value_(0.0f), last_destination_value_(0.0f), last_amount_value_(0.0),
    updating_(false), selected_(false) {
  createOffOverlay();
  addOpenGlComponent(&highlight_);
  highlight_.setVisible(false);

  source_ = std::make_unique<ModulationSelector>("source", sources, source_items, false);
  addAndMakeVisible(source_.get());
  addOpenGlComponent(source_->getImageComponent());
  source_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  source_->setLookAndFeel(TextLookAndFeel::instance());
  source_->addListener(this);
  source_->setScrollWheelEnabled(false);

  destination_ = std::make_unique<ModulationSelector>("destination", destinations, destination_items, true);
  addAndMakeVisible(destination_.get());
  addOpenGlComponent(destination_->getImageComponent());
  destination_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  destination_->setLookAndFeel(TextLookAndFeel::instance());
  destination_->addListener(this);
  destination_->setScrollWheelEnabled(false);

  amount_slider_ = std::make_unique<SynthSlider>(String("modulation_") + String(index + 1) + String("_amount"));
  addSlider(amount_slider_.get());
  amount_slider_->setBipolar(true);
  amount_slider_->setSliderStyle(Slider::LinearBar);

  power_slider_ = std::make_unique<SynthSlider>(String("modulation_") + String(index + 1) + String("_power"));
  addSlider(power_slider_.get());
  power_slider_->setLookAndFeel(CurveLookAndFeel::instance());
  power_slider_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  String bipolar_text = String("modulation_") + String(index + 1) + String("_bipolar");
  bipolar_ = std::make_unique<OpenGlShapeButton>(bipolar_text);
  bipolar_->useOnColors(true);
  bipolar_->setClickingTogglesState(true);
  addAndMakeVisible(bipolar_.get());
  addOpenGlComponent(bipolar_->getGlComponent());
  bipolar_->addListener(this);
  bipolar_->setShape(Paths::bipolar());

  stereo_ = std::make_unique<SynthButton>(String("modulation_") + String(index + 1) + String("_stereo"));
  stereo_->setText("L/R");
  stereo_->setNoBackground();
  stereo_->setLookAndFeel(TextLookAndFeel::instance());
  addButton(stereo_.get());

  String bypass_string = String("modulation_") + String(index + 1) + String("_bypass");
  bypass_ = std::make_unique<BypassButton>(bypass_string, "X", String(index + 1));
  bypass_->setText(String(index + 1));
  bypass_->setNoBackground();
  bypass_->setLookAndFeel(TextLookAndFeel::instance());
  addButton(bypass_.get());

  setScrollWheelEnabled(false);
}

void ModulationMatrixRow::resized() {
  SynthSection::resized();
  highlight_.setBounds(getLocalBounds());
  highlight_.setColor(findColour(Skin::kLightenScreen, true));

  int width = getWidth();
  int x_padding = width * kPaddingWidthPercent;
  int y_padding = size_ratio_ * 3;
  int source_width = width * kSourceWidthPercent;
  int destination_width = width * kDestinationWidthPercent;
  int component_height = getHeight() - 2 * y_padding;
  int slider_height = getSliderWidth();
  int text_component_height = component_height * 0.7f;
  int text_y = (getHeight() - text_component_height) / 2.0f;

  bypass_->setBounds(y_padding, text_y, getHeight() - 2 * y_padding, text_component_height);
  source_->setBounds(bypass_->getRight() + y_padding, y_padding, source_width, component_height);
  source_->redoImage();

  int bipolar_x = source_->getRight() + x_padding / 2.0f;
  bipolar_->setBounds(bipolar_x, y_padding, component_height, component_height);

  int stereo_x = bipolar_->getRight() + x_padding;
  int expand = x_padding / 2;
  stereo_->setBounds(stereo_x - expand, text_y, component_height + 2 * expand, text_component_height);

  int power_x = stereo_x + component_height + x_padding;
  power_slider_->setBounds(power_x, y_padding, component_height, component_height);

  int destination_x = width - x_padding - destination_width;
  destination_->setBounds(destination_x, y_padding, destination_width + x_padding / 2.0f, component_height);
  destination_->redoImage();

  int widget_margin = findValue(Skin::kWidgetMargin);
  int amount_x = power_slider_->getRight() + x_padding - widget_margin;
  int amount_width = destination_x - amount_x - x_padding / 2.0f + 2 * widget_margin;
  amount_slider_->setBounds(amount_x, (getHeight() - slider_height + 1) / 2, amount_width, slider_height);
}

void ModulationMatrixRow::paintBackground(Graphics& g) {
  g.setColour(findColour(Skin::kBody, true));
  g.fillRect(getLocalBounds());

  g.setColour(findColour(Skin::kTextComponentBackground, true));
  int rounding = findValue(Skin::kWidgetRoundedCorner);
  g.fillRoundedRectangle(source_->getBounds().toFloat(), rounding);
  g.fillRoundedRectangle(destination_->getBounds().toFloat(), rounding);

  paintKnobShadows(g);
  paintOpenGlChildrenBackgrounds(g);
}

void ModulationMatrixRow::buttonClicked(Button* button) {
  SynthSection::buttonClicked(button);
  if (button == bipolar_.get())
    power_slider_->setBipolar(bipolar_->getToggleState());

  for (Listener* listener : listeners_)
    listener->rowSelected(this);
}

void ModulationMatrixRow::updateDisplay() {
  if (updating_ || connection_ == nullptr)
    return;

  source_->setValueFromName(connection_->source_name, dontSendNotification);
  source_->redoImage();
  destination_->setValueFromName(connection_->destination_name, dontSendNotification);
  destination_->redoImage();

  updateDisplayValue();
}

void ModulationMatrixRow::updateDisplayValue() {
  bipolar_->setToggleState(connection_->modulation_processor->isBipolar(), dontSendNotification);
  stereo_->setToggleState(connection_->modulation_processor->isStereo(), dontSendNotification);
  power_slider_->setBipolar(connection_->modulation_processor->isBipolar());
  bypass_->setToggleState(connection_->modulation_processor->isBypassed(), dontSendNotification);

  last_source_value_ = source_->getValue();
  last_destination_value_ = destination_->getValue();

  amount_slider_->setDisplayMultiply(1.0f);
  if (last_destination_value_ > 0.0 && last_source_value_) {
    vital::ValueDetails details = vital::Parameters::getDetails(connection_->destination_name);
    if (details.value_scale == vital::ValueDetails::kLinear || details.value_scale == vital::ValueDetails::kIndexed)
      amount_slider_->setDisplayMultiply(details.max - details.min);

    float current_value = connection_->modulation_processor->currentBaseValue();
    if (current_value != last_amount_value_) {
      amount_slider_->setValue(current_value, dontSendNotification);
      amount_slider_->redoImage();
      last_amount_value_ = current_value;
    }
  }
}

bool ModulationMatrixRow::connected() const {
  return source_->connected() && destination_->connected();
}

bool ModulationMatrixRow::matchesSourceAndDestination(const std::string& source, const std::string& destination) const {
  String source_name = source_->getSelection();
  String destination_name = destination_->getSelection();
  return source == source_name && destination == destination_name;
}

void ModulationMatrixRow::sliderValueChanged(Slider* changed_slider) {
  updating_ = true;
  String source_name = source_->getSelection();
  String destination_name = destination_->getSelection();

  if (changed_slider == source_.get() || changed_slider == destination_.get()) {
    if (last_source_value_ > 0.5f && last_destination_value_ > 0.5f)
      parent_->disconnectModulation(connection_);
    if (source_->getValue() > 0.5 && destination_->getValue() > 0.5) {
      connection_->source_name = source_name.toStdString();
      connection_->destination_name = destination_name.toStdString();
      parent_->connectModulation(connection_);
    }
  }
  else {
    SynthSection::sliderValueChanged(changed_slider);
    parent_->notifyModulationValueChanged(index_);
  }

  last_source_value_ = source_->getValue();
  last_destination_value_ = destination_->getValue();
  updating_ = false;

  for (Listener* listener : listeners_)
    listener->rowSelected(this);
}

Rectangle<int> ModulationMatrixRow::getMeterBounds() {
  Rectangle<int> bounds = destination_->getBounds();
  bounds.setHeight(2);
  return bounds;
}

void ModulationMatrixRow::select(bool select) {
  if (select == selected_)
    return;

  selected_ = select;
  highlight_.setVisible(selected_);
}

String ModulationMatrix::getMenuSourceDisplayName(const String& original) {
  if (original == "aftertouch")
    return "After Touch";

  String modified = original.replaceFirstOccurrenceOf("control_", "");
  StringArray tokens;
  tokens.addTokens(modified, "_", "");
  String result;
  for (const String& token : tokens) {
    String capitalized = String(token.substring(0, 1)).toUpperCase() + token.substring(1);
    result += capitalized + " ";
  }

  return result.trim();
}

String ModulationMatrix::getUiSourceDisplayName(const String& original) {
  return getMenuSourceDisplayName(original).toUpperCase();
}

ModulationMatrix::ModulationMatrix(const vital::output_map& sources, const vital::output_map& destinations) :
    SynthSection("MODULATION MATRIX"), container_("Container") {
  const NaturalStringComparator comparator;

  sort_column_ = kNumber;
  sort_ascending_ = true;
  selected_index_ = 0;
  num_shown_ = 1;

  addAndMakeVisible(viewport_);
  viewport_.setViewedComponent(&container_);
  viewport_.addListener(this);
  viewport_.setScrollBarsShown(false, false, true, false);
      
  source_strings_.push_back(kNoConnectionString);
  for (const auto& source : sources)
    source_strings_.push_back(source.first);
  
  std::sort(source_strings_.begin(), source_strings_.end(), comparator);

  destination_strings_.push_back(kNoConnectionString);
  for (const auto& destination : destinations)
    destination_strings_.push_back(destination.first);

  std::sort(destination_strings_.begin(), destination_strings_.end(), comparator);

  source_popup_items_.addItem(0, "-");
  for (int i = 1; i < source_strings_.size(); ++i) {
    std::string display_name = getMenuSourceDisplayName(source_strings_[i]).toStdString();
    source_popup_items_.addItem(i, display_name);
  }

  destination_popup_items_.addItem(0, kNoConnectionString);
  destination_popup_items_.addItem(createMiscSubMenu("Global", destination_strings_));
  for (const auto& sub_menu_prefix : kDestinationSubMenuPrefixes) {
    if (sub_menu_prefix.prefix.empty())
      destination_popup_items_.addItem(-1, "");
    else {
      bool local = sub_menu_prefix.local_description;
      PopupItems sub_items = createSubMenuForParameterPrefix(sub_menu_prefix.name, sub_menu_prefix.prefix,
                                                             destination_strings_, local);
      destination_popup_items_.addItem(sub_items);
    }
  }

  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    rows_[i] = std::make_unique<ModulationMatrixRow>(i, &source_popup_items_, &destination_popup_items_,
                                                     &source_strings_, &destination_strings_);
    rows_[i]->addListener(this);
    row_order_.push_back(rows_[i].get());
    addSubSection(rows_[i].get(), false);
    container_.addAndMakeVisible(rows_[i].get());
  }

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);
  scroll_bar_->setAlwaysOnTop(true);

  readouts_ = std::make_unique<ModulationMeterReadouts>();
  addOpenGlComponent(readouts_.get());
  readouts_->setInterceptsMouseClicks(false, false);

  grid_size_x_ = std::make_unique<SynthSlider>("grid_size_x");
  grid_size_x_->setRange(1.0, LineEditor::kMaxGridSizeX, 1.0);
  grid_size_x_->setValue(kDefaultGridSizeX);
  grid_size_x_->setLookAndFeel(TextLookAndFeel::instance());
  grid_size_x_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  addSlider(grid_size_x_.get());
  grid_size_x_->addListener(this);
  grid_size_x_->setDoubleClickReturnValue(true, kDefaultGridSizeX);
  grid_size_x_->setMaxDecimalPlaces(0);
  grid_size_x_->setTextHeightPercentage(0.6f);
  grid_size_x_->setSensitivity(0.2f);
  grid_size_x_->overrideValue(Skin::kTextComponentOffset, 0.0f);

  grid_size_y_ = std::make_unique<SynthSlider>("grid_size_y");
  grid_size_y_->setRange(1.0, LineEditor::kMaxGridSizeY, 1.0);
  grid_size_y_->setValue(kDefaultGridSizeY);
  grid_size_y_->setLookAndFeel(TextLookAndFeel::instance());
  grid_size_y_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  addSlider(grid_size_y_.get());
  grid_size_y_->addListener(this);
  grid_size_y_->setDoubleClickReturnValue(true, kDefaultGridSizeY);
  grid_size_y_->setMaxDecimalPlaces(0);
  grid_size_y_->setTextHeightPercentage(0.6f);
  grid_size_y_->setSensitivity(0.2f);
  grid_size_y_->overrideValue(Skin::kTextComponentOffset, 0.0f);

  paint_ = std::make_unique<OpenGlShapeButton>("paint");
  paint_->useOnColors(true);
  paint_->setClickingTogglesState(true);
  addAndMakeVisible(paint_.get());
  addOpenGlComponent(paint_->getGlComponent());
  paint_->addListener(this);
  paint_->setShape(Paths::paintBrush());

  remap_name_ = std::make_unique<PlainTextComponent>("remap_name", String("MOD REMAP ") + String(1));
  addOpenGlComponent(remap_name_.get());
  remap_name_->setFontType(PlainTextComponent::kTitle);

  paint_pattern_ = std::make_unique<PaintPatternSelector>("paint_pattern");
  addSlider(paint_pattern_.get());
  paint_pattern_->addListener(this);
  paint_pattern_->setRange(0.0f, LfoSection::kNumPaintPatterns - 1.0f, 1.0f);
  paint_pattern_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  paint_pattern_->setStringLookup(strings::kPaintPatternNames);
  paint_pattern_->setLookAndFeel(TextLookAndFeel::instance());
  paint_pattern_->setLongStringLookup(strings::kPaintPatternNames);
  paint_pattern_->setTextHeightPercentage(0.45f);
  paint_pattern_->setActive(false);
  paint_pattern_->overrideValue(Skin::kTextComponentOffset, 0.0f);

  smooth_ = std::make_unique<OpenGlShapeButton>("smooth");
  smooth_->useOnColors(true);
  smooth_->setClickingTogglesState(true);
  addAndMakeVisible(smooth_.get());
  addOpenGlComponent(smooth_->getGlComponent());
  smooth_->addListener(this);
  smooth_->setShape(Paths::halfSinCurve());

  preset_selector_ = std::make_unique<PresetSelector>();
  addSubSection(preset_selector_.get());
  preset_selector_->addListener(this);
  setPresetSelector(preset_selector_.get());
  preset_selector_->setText("Linear");

  setSkinOverride(Skin::kModulationMatrix);
}

ModulationMatrix::~ModulationMatrix() { }

void ModulationMatrix::paintScrollableBackground() {
  if (getWidth() <= 0)
    return;

  int row_height = getRowHeight();
  int total_height = kRowPadding + num_shown_ * (row_height + kRowPadding);
  total_height = std::max(total_height, viewport_.getHeight());
  container_.setBounds(container_.getX(), container_.getY(), getWidth(), total_height);

  int mult = getPixelMultiple();
  Image background_image = Image(Image::ARGB, getWidth() * mult, total_height * mult, true);
  Graphics background_graphics(background_image);
  background_graphics.addTransform(AffineTransform::scale(mult));

  for (int i = 0; i < num_shown_; ++i) {
    ModulationMatrixRow* row = row_order_[i];

    background_graphics.saveState();
    Rectangle<int> bounds = row->getBounds();
    background_graphics.reduceClipRegion(bounds);
    background_graphics.setOrigin(bounds.getTopLeft());
    row->paintBackground(background_graphics);
    background_graphics.restoreState();
  }
  background_.setOwnImage(background_image);
}

void ModulationMatrix::paintBackground(Graphics& g) {
  int padding = getPadding();

  Rectangle<int> matrix_bounds(0, 0, getWidth(), viewport_.getBottom());
  int remap_y = matrix_bounds.getBottom() + padding;
  Rectangle<int> remap_bounds(0, remap_y, getWidth(), getHeight() - remap_y);
  paintBody(g, matrix_bounds);
  paintBody(g, remap_bounds);

  int title_width = getTitleWidth();
  int row_height = getRowHeight();

  int width = getWidth();
  int x_padding = width * kPaddingWidthPercent;
  int y_padding = size_ratio_ * 3;
  int source_width = width * kSourceWidthPercent;
  int destination_width = width * kDestinationWidthPercent;
  int component_height = row_height - 2 * y_padding;
  int bipolar_x = source_width + row_height;
  int stereo_x = bipolar_x + component_height + x_padding;
  int morph_x = stereo_x + component_height + x_padding;
  int amount_x = morph_x + component_height + x_padding;
  int destination_x = getWidth() - destination_width - x_padding;

  g.setColour(findColour(Skin::kLightenScreen, true));
  g.fillRect(row_height, 0, 1, title_width);
  g.fillRect(morph_x, 0, 1, title_width);
  g.fillRect(bipolar_x, 0, 1, title_width);
  g.fillRect(stereo_x, 0, 1, title_width);
  g.fillRect(amount_x, 0, 1, title_width);
  g.fillRect(destination_x, 0, 1, title_width);

  g.setColour(findColour(Skin::kTextComponentText, true));
  Font regular = Fonts::instance()->proportional_light().withPointHeight(title_width * 0.4f);
  Font sorted = Fonts::instance()->proportional_regular().withPointHeight(title_width * 0.4f);
  g.setFont(sort_column_ == kNumber ? sorted : regular);
  g.drawText("#", 0, 0, row_height, title_width, Justification::centred);
  g.setFont(sort_column_ == kSource ? sorted : regular);
  g.drawText("SOURCE", row_height, 0, bipolar_x - row_height, title_width, Justification::centred);
  g.setFont(sort_column_ == kBipolar ? sorted : regular);
  g.drawText("BIPOLAR", bipolar_x, 0, stereo_x - bipolar_x, title_width, Justification::centred);
  g.setFont(sort_column_ == kStereo ? sorted : regular);
  g.drawText("STEREO", stereo_x, 0, morph_x - stereo_x, title_width, Justification::centred);
  g.setFont(sort_column_ == kMorph ? sorted : regular);
  g.drawText("MORPH", morph_x, 0, amount_x - morph_x, title_width, Justification::centred);
  g.setFont(sort_column_ == kAmount ? sorted : regular);
  g.drawText("AMOUNT", amount_x, 0, destination_x - amount_x, title_width, Justification::centred);
  g.setFont(sort_column_ == kDestination ? sorted : regular);
  g.drawText("DESTINATION", destination_x - 0.5f * x_padding, 0,
             getWidth() - destination_x + 0.5f * x_padding, title_width, Justification::centred);

  int rounding = findValue(Skin::kBodyRounding);
  int widget_rounding = getWidgetRounding();
  g.setColour(findColour(Skin::kBackground, true));

  g.saveState();
  g.reduceClipRegion(0, title_width, getWidth(), getHeight());
  g.fillRoundedRectangle(0, 0, getWidth(), viewport_.getBottom(), rounding);
  g.restoreState();

  paintBorder(g, matrix_bounds);
  paintBorder(g, remap_bounds);
  viewport_.setColour(ScrollBar::thumbColourId, findColour(Skin::kLightenScreen, true));

  if (map_editors_[selected_index_] && map_editors_[0]) {
    g.saveState();
    Rectangle<int> bounds = getLocalArea(map_editors_[0].get(), map_editors_[0]->getLocalBounds());
    g.reduceClipRegion(bounds);
    g.setOrigin(bounds.getTopLeft());
    map_editors_[selected_index_]->paintBackground(g);
    g.restoreState();
  }

  g.saveState();
  Rectangle<int> preset_bounds = getLocalArea(preset_selector_.get(), preset_selector_->getLocalBounds());
  g.reduceClipRegion(preset_bounds);
  g.setOrigin(preset_bounds.getTopLeft());
  preset_selector_->paintBackground(g);
  g.restoreState();

  g.setColour(findColour(Skin::kPopupSelectorBackground, true));
  g.fillRoundedRectangle(paint_->getX(), paint_->getY(),
                         paint_pattern_->getRight() - paint_->getX(), paint_->getHeight(), widget_rounding);
  g.fillRoundedRectangle(grid_size_x_->getX(), grid_size_x_->getY(),
                         grid_size_y_->getRight() - grid_size_x_->getX(), grid_size_x_->getHeight(), widget_rounding);

  int grid_label_x = grid_size_x_->getX();
  int grid_size_width = std::max(1, grid_size_y_->getRight() - grid_label_x);
  setLabelFont(g);
  g.setColour(findColour(Skin::kBodyText, true));
  g.drawText("-", grid_label_x, grid_size_x_->getY(), grid_size_width, grid_size_x_->getHeight(),
             Justification::centred, false);

  checkNumModulationsShown();
  paintOpenGlChildrenBackgrounds(g);
  setScrollBarRange();
}

void ModulationMatrix::paintBackgroundShadow(Graphics& g) {
  Rectangle<int> matrix_bounds(0, 0, getWidth(), viewport_.getBottom());
  paintTabShadow(g, matrix_bounds);

  int remap_y = viewport_.getBottom() + getPadding();
  Rectangle<int> remap_bounds(0, remap_y, getWidth(), getHeight() - remap_y);
  paintTabShadow(g, remap_bounds);
}

void ModulationMatrix::parentHierarchyChanged() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  vital::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    rows_[i]->setGuiParent(parent);
    vital::ModulationConnection* connection = bank.atIndex(i);
    rows_[i]->setConnection(connection);

    if (map_editors_[i] == nullptr) {
      LineGenerator* map_generator = connection->modulation_processor->lineMapGenerator();
      std::string name = "modulation_source_" + std::to_string(i + 1);
      map_editors_[i] = std::make_unique<LineMapEditor>(map_generator, name);
      map_editors_[i]->setPaintPattern(LfoSection::getPaintPattern(paint_pattern_->getValue()));
      map_editors_[i]->addListener(this);
      addOpenGlComponent(map_editors_[i].get());
      addOpenGlComponent(map_editors_[i]->getTextEditorComponent());
      map_editors_[i]->setVisible(false);
    }
  }
  rows_[0]->select(true);
  map_editors_[0]->setVisible(true);
}

void ModulationMatrix::setRowPositions() {
  int row_height = getRowHeight();
  int matrix_width = getWidth();
  int widget_margin = getWidgetMargin();
  int title_width = getTitleWidth();
  
  int remap_section_y = viewport_.getBottom() + getPadding();
  int remap_y = remap_section_y + title_width;
  Rectangle<int> mapping_bounds(widget_margin, remap_y,
                                getWidth() - 2 * widget_margin, getHeight() - remap_y - widget_margin);

  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    row_order_[i]->setBounds(0, kRowPadding + i * (row_height + kRowPadding), matrix_width, row_height);

    float size_ratio = getSizeRatio();
    if (map_editors_[i]) {
      map_editors_[i]->setBounds(mapping_bounds);
      map_editors_[i]->setSizeRatio(size_ratio);
    }
  }
}

void ModulationMatrix::resized() {
  static constexpr float kScrollBarWidth = 13.0f;

  SynthSection::resized();

  int row_height = getRowHeight();
  int title_width = getTitleWidth();
  int widget_margin = getWidgetMargin();

  int matrix_height = (row_height + kRowPadding) * kMatrixHeightInRows + kRowPadding;
  int matrix_width = getWidth();
  viewport_.setBounds(0, title_width, matrix_width, matrix_height);
  setRowPositions();

  int preset_x = getWidth() / 2;
  int top_bar_height = title_width - 2 * widget_margin;
  int top_bar_y = viewport_.getBottom() + getPadding() + widget_margin;
  preset_selector_->setBounds(preset_x, top_bar_y, getWidth() - preset_x - widget_margin, top_bar_height);

  smooth_->setBounds(preset_x - title_width - widget_margin, top_bar_y, title_width, top_bar_height);
  int grid_y_x = smooth_->getX() - title_width - widget_margin;
  int grid_x_x = grid_y_x - title_width - widget_margin;
  grid_size_y_->setBounds(grid_y_x, top_bar_y, title_width, top_bar_height);
  grid_size_x_->setBounds(grid_x_x, top_bar_y, title_width, top_bar_height);

  paint_pattern_->setPadding(getWidgetMargin());
  int paint_pattern_width = 3 * top_bar_height;
  paint_pattern_->setBounds(grid_x_x - paint_pattern_width - widget_margin, top_bar_y,
                            paint_pattern_width, top_bar_height);

  paint_->setBounds(paint_pattern_->getX() - top_bar_height, top_bar_y, top_bar_height, top_bar_height);

  remap_name_->setBounds(widget_margin, top_bar_y, paint_->getX() - 2 * widget_margin, top_bar_height);
  remap_name_->setTextSize(title_width * 0.45f);
  remap_name_->setColor(findColour(Skin::kHeadingText, true));

  setMeterBounds();
  
  int container_height = kRowPadding + num_shown_ * (row_height + kRowPadding);
  container_.setBounds(0, title_width, matrix_width, container_height);

  int scroll_bar_width = size_ratio_ * kScrollBarWidth;
  scroll_bar_->setBounds(getWidth() - scroll_bar_width - 1, title_width, scroll_bar_width, matrix_height);
  scroll_bar_->setColor(findColour(Skin::kLightenScreen, true));
  setScrollBarRange();

  paintScrollableBackground();
}

void ModulationMatrix::setMeterBounds() {
  readouts_->setBounds(viewport_.getBounds());
  for (int i = 0; i < vital::kMaxModulationConnections; ++i)
    readouts_->setMeterBounds(i, rows_[i]->getMeterBounds() + rows_[i]->getPosition());
}

void ModulationMatrix::sliderValueChanged(Slider* changed_slider) {
  if (changed_slider == grid_size_x_.get()) {
    if (map_editors_[selected_index_])
      map_editors_[selected_index_]->setGridSizeX(grid_size_x_->getValue());
  }
  else if (changed_slider == grid_size_y_.get()) {
    if (map_editors_[selected_index_])
      map_editors_[selected_index_]->setGridSizeY(grid_size_y_->getValue());
  }
  else if (changed_slider == paint_pattern_.get()) {
    if (map_editors_[selected_index_])
      map_editors_[selected_index_]->setPaintPattern(LfoSection::getPaintPattern(paint_pattern_->getValue()));
  }
  else
    SynthSection::sliderValueChanged(changed_slider);
}

void ModulationMatrix::buttonClicked(Button* clicked_button) {
  if (clicked_button == paint_.get()) {
    if (map_editors_[selected_index_])
      map_editors_[selected_index_]->setPaint(paint_->getToggleState());
    paint_pattern_->setActive(paint_->getToggleState());
  }
  else if (clicked_button == smooth_.get()) {
    if (map_editors_[selected_index_])
      map_editors_[selected_index_]->setSmooth(smooth_->getToggleState());
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void ModulationMatrix::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  if (map_editors_[selected_index_])
    smooth_->setToggleState(map_editors_[selected_index_]->getSmooth(), dontSendNotification);
}

void ModulationMatrix::updateModulations() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int i = 0;
  for (auto& row : rows_) {
    row->updateDisplay();
    row->setActive(row->connected());
    readouts_->setMeterActive(i++, row->connected());
  }

  if (map_editors_[selected_index_])
    map_editors_[selected_index_]->setActive(rows_[selected_index_]->connected());

  sort();
}

void ModulationMatrix::updateModulationValue(int index) {
  rows_[index]->updateDisplayValue();
  rowSelected(rows_[index].get());
}

void ModulationMatrix::checkNumModulationsShown() {
  if (row_order_.size() != vital::kMaxModulationConnections)
    return;

  int num_show = 1;
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    if (row_order_[i]->isActive())
      num_show = i + 2;
  }
  num_show = std::min(vital::kMaxModulationConnections, num_show);

  for (int i = 0; i < vital::kMaxModulationConnections; ++i)
    row_order_[i]->setVisible(i < num_show);

  if (num_shown_ != num_show) {
    num_shown_ = num_show;
    paintScrollableBackground();
  }
}

void ModulationMatrix::rowSelected(ModulationMatrixRow* selected_row) {
  if (rows_[selected_row->index()]->selected())
    return;
  
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    bool selected = rows_[i].get() == selected_row;
    rows_[i]->select(selected);
    if (map_editors_[i]) {
      map_editors_[i]->setVisible(selected);

      if (selected) {
        map_editors_[i]->setActive(rows_[i]->connected());

        selected_index_ = i;
        smooth_->setToggleState(map_editors_[i]->getModel()->smooth(), dontSendNotification);
        map_editors_[i]->setGridSizeX(grid_size_x_->getValue());
        map_editors_[i]->setGridSizeY(grid_size_y_->getValue());
        map_editors_[i]->setPaintPattern(LfoSection::getPaintPattern(paint_pattern_->getValue()));
        map_editors_[i]->setPaint(paint_->getToggleState());
        remap_name_->setText(String("MOD REMAP ") + String(i + 1));
      }
    }
  }
}

void ModulationMatrix::mouseDown(const MouseEvent& e) {
  if (e.position.y > getTitleWidth())
    return;
  
  int x = e.position.x;
  int width = getWidth();
  int x_padding = width * kPaddingWidthPercent;
  int row_height = getRowHeight();
  int y_padding = size_ratio_ * 3;
  int component_height = row_height - 2 * y_padding;
  int source_width = width * kSourceWidthPercent;
  int destination_width = width * kDestinationWidthPercent;
  int bipolar_x = source_width + 1.5f * x_padding;
  int stereo_x = bipolar_x + component_height + x_padding;
  int morph_x = stereo_x + component_height + x_padding;
  int amount_x = morph_x + component_height + x_padding;
  int destination_x = getWidth() - destination_width - 1.5f * x_padding;
  
  SortColumn sort_column = kNumColumns;
  if (x < x_padding)
    sort_column = kNumber;
  else if (x < bipolar_x)
    sort_column = kSource;
  else if (x < stereo_x)
    sort_column = kBipolar;
  else if (x < morph_x)
    sort_column = kStereo;
  else if (x < amount_x)
    sort_column = kMorph;
  else if (x < destination_x)
    sort_column = kAmount;
  else
    sort_column = kDestination;
  
  if (sort_column == sort_column_)
    sort_ascending_ = !sort_ascending_;
  else
    sort_ascending_ = true;
  
  sort_column_ = sort_column;
  sort();
}

void ModulationMatrix::sort() {
  if (sort_column_ == kNumber) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareIndexAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareIndexDescending);
  }
  else if (sort_column_ == kSource) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareSourceAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareSourceDescending);
  }
  else if (sort_column_ == kBipolar) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareBipolarAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareBipolarDescending);
  }
  else if (sort_column_ == kStereo) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareStereoAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareStereoDescending);
  }
  else if (sort_column_ == kMorph) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareMorphAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareMorphDescending);
  }
  else if (sort_column_ == kAmount) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareAmountAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareAmountDescending);
  }
  else if (sort_column_ == kDestination) {
    if (sort_ascending_)
      std::stable_sort(row_order_.begin(), row_order_.end(), compareDestinationAscending);
    else
      std::stable_sort(row_order_.begin(), row_order_.end(), compareDestinationDescending);
  }

  std::stable_sort(row_order_.begin(), row_order_.end(), compareConnected);

  checkNumModulationsShown();
  setRowPositions();
  paintScrollableBackground();
  setMeterBounds();
}

void ModulationMatrix::initOpenGlComponents(OpenGlWrapper& open_gl) {
  background_.init(open_gl);
  SynthSection::initOpenGlComponents(open_gl);
}

void ModulationMatrix::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  OpenGlComponent::setViewPort(&viewport_, open_gl);
  ScopedLock open_gl_lock(open_gl_critical_section_);

  float image_width = vital::utils::nextPowerOfTwo(background_.getImageWidth());
  float image_height = vital::utils::nextPowerOfTwo(background_.getImageHeight());
  int mult = getPixelMultiple();
  float width_ratio = image_width / (mult * viewport_.getWidth());
  float height_ratio = image_height / (mult * viewport_.getHeight());
    
  float y_offset = (2.0f * viewport_.getViewPositionY()) / viewport_.getHeight();

  background_.setTopLeft(-1.0f, 1.0f + y_offset);
  background_.setTopRight(-1.0 + 2.0f * width_ratio, 1.0f + y_offset);
  background_.setBottomLeft(-1.0f, 1.0f - 2.0f * height_ratio + y_offset);
  background_.setBottomRight(-1.0 + 2.0f * width_ratio, 1.0f - 2.0f * height_ratio + y_offset);
    
  background_.setColor(Colours::white);
  background_.drawImage(open_gl);
  readouts_->setScrollOffset(viewport_.getViewPositionY());
  map_editors_[selected_index_]->setAnimate(rows_[selected_index_]->isActive());
  SynthSection::renderOpenGlComponents(open_gl, animate);
}

void ModulationMatrix::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  background_.destroy(open_gl);
  SynthSection::destroyOpenGlComponents(open_gl);
}

void ModulationMatrix::prevClicked() {
  File lfo_file = LoadSave::getShiftedFile(LoadSave::kLfoFolderName, String("*.") + vital::kLfoExtension,
                                           "", getCurrentFile(), -1);
  if (lfo_file.exists())
    loadFile(lfo_file);
  
  updatePopupBrowser(this);
}

void ModulationMatrix::nextClicked() {
  File lfo_file = LoadSave::getShiftedFile(LoadSave::kLfoFolderName, String("*.") + vital::kLfoExtension,
                                           "", getCurrentFile(), 1);
  if (lfo_file.exists())
    loadFile(lfo_file);
  
  updatePopupBrowser(this);
}

void ModulationMatrix::textMouseDown(const MouseEvent& e) {
  static constexpr int kBrowserWidth = 500;
  static constexpr int kBrowserHeight = 250;

  int browser_width = kBrowserWidth * size_ratio_;
  int browser_height = kBrowserHeight * size_ratio_;
  Rectangle<int> bounds(preset_selector_->getRight(), preset_selector_->getY(), browser_width, browser_height);
  bounds = getLocalArea(this, bounds);
  showPopupBrowser(this, bounds, LoadSave::getLfoDirectories(), String("*.") + vital::kLfoExtension,
                   LoadSave::kLfoFolderName, "");
}

void ModulationMatrix::lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) {
  if (paint_->getToggleState())
    paint_pattern_->mouseWheelMove(e, wheel);
  else
    grid_size_x_->mouseWheelMove(e, wheel);
}

void ModulationMatrix::togglePaintMode(bool enabled, bool temporary_switch) {
  paint_->setToggleState(enabled != temporary_switch, dontSendNotification);
  paint_pattern_->setActive(enabled != temporary_switch);
}

void ModulationMatrix::importLfo() {
  FileChooser import_box("Import LFO", LoadSave::getUserLfoDirectory(), String("*.") + vital::kLfoExtension);
  if (!import_box.browseForFileToOpen())
    return;

  File choice = import_box.getResult();
  loadFile(choice.withFileExtension(vital::kLfoExtension));
}

void ModulationMatrix::exportLfo() {
  FileChooser export_box("Export LFO", LoadSave::getUserLfoDirectory(), String("*.") + vital::kLfoExtension);
  if (!export_box.browseForFileToSave(true))
    return;

  File choice = export_box.getResult();
  choice = choice.withFileExtension(vital::kLfoExtension);
  if (!choice.exists())
    choice.create();
  choice.replaceWithText(map_editors_[selected_index_]->getModel()->stateToJson().dump());

  String name = choice.getFileNameWithoutExtension();
  map_editors_[selected_index_]->getModel()->setName(name.toStdString());
  preset_selector_->setText(name);
}

void ModulationMatrix::fileLoaded() {
  smooth_->setToggleState(map_editors_[selected_index_]->getModel()->smooth(), dontSendNotification);
  preset_selector_->setText(map_editors_[selected_index_]->getModel()->getName());
}

void ModulationMatrix::loadFile(const File& file) {
  if (!file.exists())
    return;

  current_file_ = file;
  LineMapEditor* current_editor = map_editors_[selected_index_].get();

  try {
    json parsed_file = json::parse(file.loadFileAsString().toStdString(), nullptr, false);
    current_editor->getModel()->jsonToState(parsed_file);
  }
  catch (const json::exception& e) {
    return;
  }

  String name = file.getFileNameWithoutExtension();
  current_editor->getModel()->setName(name.toStdString());
  current_editor->getModel()->setLastBrowsedFile(file.getFullPathName().toStdString());
  preset_selector_->setText(name);

  current_editor->resetPositions();
  smooth_->setToggleState(current_editor->getModel()->smooth(), dontSendNotification);
}

void ModulationMatrix::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
  ScopedLock open_gl_lock(open_gl_critical_section_);
  viewport_.setViewPosition(Point<int>(0, range_start));
}

void ModulationMatrix::setScrollBarRange() {
  scroll_bar_->setRangeLimits(0.0, container_.getHeight());
  scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(), dontSendNotification);
}
