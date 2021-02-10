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

#include "modulation_manager.h"

#include "bar_renderer.h"
#include "skin.h"
#include "modulation_connection_processor.h"
#include "modulation_matrix.h"
#include "modulation_meter.h"
#include "shaders.h"
#include "synth_gui_interface.h"

namespace {
  constexpr float kDefaultModulationRatio = 0.25f;
  constexpr float kModSourceMeterWidth = 0.0018f;
  constexpr float kModSourceMeterBuffer = 0.002f;
  constexpr float kModSourceMinRadius = 0.005f;
  constexpr float kModSmoothDecay = 0.25f;

  bool showingInParents(Component* component) {
    if (component == nullptr || component->getParentComponent() == nullptr)
      return true;
    
    return component->isVisible() && showingInParents(component->getParentComponent());
  }
}

class ExpandModulationButton : public OpenGlToggleButton {
  public:
    ExpandModulationButton() : OpenGlToggleButton("expand modulation"),
                               num_sliders_(0), amount_quad_(Shaders::kRingFragment) {
      setLightenButton();
      setTriggeredOnMouseDown(true);
      setMouseClickGrabsKeyboardFocus(false);
      amount_quad_.setTargetComponent(this);
      amount_quad_.setThickness(2.0f);
    }

    int getNumColumns(int num_sliders) {
      float height_width_ratio = getHeight() * 1.0f / getWidth();

      int columns = 1;
      while (columns * (int)(height_width_ratio * columns) < num_sliders)
        columns++;
      return columns;
    }

    void setSliders(std::vector<ModulationAmountKnob*> sliders) {
      sliders_ = sliders;
      for (int i = 0; i < sliders.size(); ++i)
        colors_[i] = sliders_[i]->findColour(Skin::kRotaryArc, true);
      num_sliders_ = static_cast<int>(sliders_.size());
    }

    std::vector<ModulationAmountKnob*> getSliders() { return sliders_; }

    void renderSliderQuads(OpenGlWrapper& open_gl, bool animate) {
      int num_sliders = num_sliders_;

      float width = getWidth();
      float height = getHeight();

      int columns = getNumColumns(num_sliders);
      float cell_width = width / columns;
      int rows = (num_sliders + columns - 1) / columns;
      int y_offset = (height - (rows * cell_width)) / 2;
      float gl_width = 2.0f * cell_width / width;
      float gl_height = 2.0f * cell_width / height;

      int row = 0;
      int column = 0;
      for (int i = 0; i < num_sliders; ++i) {
        float x = column * cell_width;
        float y = height - y_offset - (row + 1) * cell_width;

        amount_quad_.setColor(colors_[i]);
        amount_quad_.setAltColor(colors_[i].withMultipliedAlpha(0.5f));
        amount_quad_.setQuad(0, 2.0f * x / width - 1.0f, 1.0f - 2.0f * y / height - gl_height, gl_width, gl_height);
        amount_quad_.render(open_gl, animate);

        column++;
        if (column >= columns) {
          row++;
          column = 0;
        }
      }
    }

  private:
    std::vector<ModulationAmountKnob*> sliders_;
    int num_sliders_;
    Colour colors_[vital::kMaxModulationConnections];

    OpenGlQuad amount_quad_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpandModulationButton)
};

class ModulationDestination : public Component {
  public:
    ModulationDestination(SynthSlider* source) : destination_slider_(source), margin_(0), index_(0),
                                                 size_multiple_(1.0f),
                                                 active_(false), rectangle_(false), rotary_(true) {
      setName(source->getName());
    }
    ModulationDestination() = delete;

    virtual ~ModulationDestination() { }

    SynthSlider* getDestinationSlider() { return destination_slider_; }
    void setActive(bool active) { active_ = active; }

    void setSizeMultiple(float multiple) {
      size_multiple_ = multiple;
      repaint();
    }

    Rectangle<float> getFillBounds() {
      static constexpr float kBufferPercent = 0.4f;

      float width = getWidth();
      float height = getHeight();

      if (!rectangle_ && rotary_) {
        float offset = destination_slider_->findValue(Skin::kKnobOffset);
        float rotary_width = size_multiple_ * destination_slider_->findValue(Skin::kKnobModMeterArcSize);
        float x = (width - rotary_width) / 2.0f;
        float y = offset + (height - rotary_width) / 2.0f;
        return Rectangle<float>(x, y, rotary_width, rotary_width);
      }

      if (rectangle_)
        return getLocalBounds().toFloat();

      if (destination_slider_->getSliderStyle() == Slider::LinearBar) {
        float y = height * 0.5f * (1.0f - SynthSlider::kLinearWidthPercent);
        float glow_height = height * SynthSlider::kLinearWidthPercent;
        y -= 2.0f * glow_height * kBufferPercent;
        glow_height += 4.0f * kBufferPercent * glow_height;

        return Rectangle<float>(margin_, y, width - 2 * margin_, glow_height);
      }

      float x = width * 0.5f * (1.0f - SynthSlider::kLinearWidthPercent);
      float glow_width = width * SynthSlider::kLinearWidthPercent;
      x -= 2.0f * glow_width * kBufferPercent;
      glow_width += 4.0f * kBufferPercent * glow_width;
      return Rectangle<float>(x, margin_, glow_width, height - 2 * margin_);
    }

    void setRectangle(bool rectangle) { rectangle_ = rectangle; }
    void setRotary(bool rotary) { rotary_ = rotary; }
    void setMargin(int margin) { margin_ = margin; }
    void setIndex(int index) { index_ = index; }

    bool hasExtraModulationTarget() { return destination_slider_->getExtraModulationTarget() != nullptr; }
    bool isRotary() { return !rectangle_ && rotary_; }
    bool isActive() { return active_; }
    int getIndex() { return index_; }

  private:
    Component* viewport_container_;
    SynthSlider* destination_slider_;
    int margin_;
    int index_;
    float size_multiple_;
    bool active_;
    bool rectangle_;
    bool rotary_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationDestination)
};

ModulationAmountKnob::ModulationAmountKnob(String name, int index) : SynthSlider(name),
                                                                     color_component_(nullptr), index_(index) {
  setModulationKnob();
  bypass_ = false;
  stereo_ = false;
  bipolar_ = false;
  draw_background_ = false;
  name_ = name;
  editing_ = false;

  setShowPopupOnHover(true);
  setTextEntrySizePercent(2.0f, 1.0f);
  setDoubleClickReturnValue(false, 0.0f);
  setWantsKeyboardFocus(false);
  showing_ = true;
  hovering_ = false;
  current_modulator_ = false;
}

void ModulationAmountKnob::mouseDown(const MouseEvent& e) {
  if (e.mods.isMiddleButtonDown())
    toggleBypass();

  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseExit(e);

    PopupItems options;
    options.addItem(kDisconnect, "Remove");
    options.addItem(kToggleBypass, bypass_ ? "Unbypass" : "Bypass");
    options.addItem(kToggleBipolar, bipolar_ ? "Make Unipolar" : "Make Bipolar");
    options.addItem(kToggleStereo, stereo_ ? "Make Mono" : "Make Stereo");
    options.addItem(-1, "");

    if (has_parameter_assignment_)
      options.addItem(kArmMidiLearn, "Learn MIDI Assignment");

    if (has_parameter_assignment_ && synth_interface_->getSynth()->isMidiMapped(getName().toStdString()))
      options.addItem(kClearMidiLearn, "Clear MIDI Assignment");

    options.addItem(kManualEntry, "Enter Value");

    hovering_ = false;
    redoImage();

    auto callback = [=](int selection) { handleModulationMenuCallback(selection); };
    auto cancel = [=]() {
      for (SliderListener* listener : slider_listeners_)
        listener->menuFinished(this);
    };
    parent_->showPopupSelector(this, e.getPosition(), options, callback, cancel);

    for (SliderListener* listener : slider_listeners_)
      listener->mouseDown(this);
  }
  else {
    SynthSlider::mouseDown(e);
    MouseInputSource source = e.source;

    if (source.isMouse() && source.canDoUnboundedMovement()) {
      editing_ = true;
      source.hideCursor();
      source.enableUnboundedMouseMovement(true);
      mouse_down_position_ = e.getScreenPosition();
      for (SliderListener* listener : slider_listeners_)
        listener->beginModulationEdit(this);
    }
  }
}

void ModulationAmountKnob::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu()) {
    SynthSlider::mouseUp(e);

    MouseInputSource source = e.source;
    if (source.isMouse() && source.canDoUnboundedMovement()) {
      source.showMouseCursor(MouseCursor::NormalCursor);
      source.enableUnboundedMouseMovement(false);
      if (getScreenBounds().contains(e.getScreenPosition()))
        editing_ = false;
      source.setScreenPosition(mouse_down_position_.toFloat());
    }
  }
}

void ModulationAmountKnob::mouseExit(const MouseEvent& e) {
  if (!editing_) {
    for (SliderListener* listener : slider_listeners_)
      listener->endModulationEdit(this);
  }
  editing_ = false;
  SynthSlider::mouseExit(e);
}

void ModulationAmountKnob::toggleBypass() {
  bypass_ = !bypass_;
  for (Listener* listener : listeners_)
    listener->setModulationBypass(this, bypass_);
  setColors();
}

void ModulationAmountKnob::handleModulationMenuCallback(int result) {
  if (result == kDisconnect) {
    for (Listener* listener : listeners_)
      listener->disconnectModulation(this);
  }
  else if (result == kToggleBypass)
    toggleBypass();
  else if (result == kToggleBipolar) {
    bipolar_ = !bipolar_;
    for (Listener* listener : listeners_)
      listener->setModulationBipolar(this, bipolar_);
  }
  else if (result == kToggleStereo) {
    stereo_ = !stereo_;
    for (Listener* listener : listeners_)
      listener->setModulationStereo(this, stereo_);
  }
  else
    handlePopupResult(result);

  if (result != kManualEntry) {
    for (SliderListener* listener : slider_listeners_)
      listener->menuFinished(this);
  }
}

void ModulationAmountKnob::makeVisible(bool visible) {
  if (visible == showing_)
    return;

  showing_ = visible;
  setVisible(visible);
  setAlpha((showing_ || hovering_) ? 1.0f : 0.0f);
}

void ModulationAmountKnob::hideImmediately() {
  setAlpha(0.0f, true);
  showing_ = false;
  hovering_ = false;
  setVisible(false);
}

void ModulationAmountKnob::setCurrentModulator(bool current) {
  if (current_modulator_ == current)
    return;

  setColour(Skin::kRotaryArc, findColour(Skin::kModulationMeterControl, true));
  current_modulator_ = current;
}

void ModulationAmountKnob::setDestinationComponent(Component* component, const std::string& name) {
  color_component_ = component;
  setPopupPrefix(vital::Parameters::getDisplayName(name) + ": ");
  
  if (color_component_)
    setColour(Skin::kRotaryArc, color_component_->findColour(Skin::kRotaryArc, true));
}

Colour ModulationAmountKnob::getInternalColor() {
  if (color_component_)
     return color_component_->findColour(Skin::kRotaryArc, true);
  return findColour(Skin::kModulationMeterControl, true);
}

void ModulationAmountKnob::setSource(const std::string& name) {
  setPopupPrefix(ModulationMatrix::getMenuSourceDisplayName(name) + ": ");
  repaint();
}

ModulationManager::ModulationManager(
    std::map<std::string, ModulationButton*> modulation_buttons,
    std::map<std::string, SynthSlider*> sliders,
    vital::output_map mono_modulations,
    vital::output_map poly_modulations) : SynthSection("modulation_manager"),
                                          drag_quad_(Shaders::kRingFragment),
                                          current_modulator_quad_(Shaders::kRoundedRectangleBorderFragment),
                                          editing_rotary_amount_quad_(Shaders::kRotaryModulationFragment),
                                          editing_linear_amount_quad_(Shaders::kLinearModulationFragment),
                                          modifying_(false), dragging_(false), changing_hover_modulation_(false),
                                          current_modulator_(nullptr) {
  current_modulator_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
  drag_quad_.setTargetComponent(this);
  editing_rotary_amount_quad_.setTargetComponent(this);
  editing_rotary_amount_quad_.setActive(false);
  editing_rotary_amount_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
  editing_linear_amount_quad_.setTargetComponent(this);
  editing_linear_amount_quad_.setActive(false);
  editing_linear_amount_quad_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
  addOpenGlComponent(&modulation_expansion_box_);
  modulation_expansion_box_.setVisible(false);
  modulation_expansion_box_.setWantsKeyboardFocus(true);
  modulation_expansion_box_.addListener(this);
  modulation_expansion_box_.setAlwaysOnTop(true);

  setSkinOverride(Skin::kModulationDragDrop);

  last_milliseconds_ = Time::currentTimeMillis();
  current_source_ = nullptr;
  current_expanded_modulation_ = nullptr;
  temporarily_set_destination_ = nullptr;
  temporarily_set_synth_slider_ = nullptr;
  temporarily_set_hover_slider_ = nullptr;
  temporarily_set_bipolar_ = false;
  num_voices_readout_ = nullptr;

  modulation_buttons_ = modulation_buttons;
  for (auto& modulation_button : modulation_buttons_) {
    modulation_button.second->addListener(this);

    modulation_callout_buttons_[modulation_button.first] = std::make_unique<ExpandModulationButton>();
    addChildComponent(modulation_callout_buttons_[modulation_button.first].get());
    addOpenGlComponent(modulation_callout_buttons_[modulation_button.first]->getGlComponent());
    modulation_callout_buttons_[modulation_button.first]->addListener(this);
  }

  modulation_source_meters_ = std::make_unique<BarRenderer>(modulation_buttons.size());
  modulation_source_meters_->setBarWidth(0.0f);
  addAndMakeVisible(modulation_source_meters_.get());
  modulation_source_meters_->setInterceptsMouseClicks(false, false);

  setInterceptsMouseClicks(false, true);

  modulation_destinations_ = std::make_unique<Component>();
  modulation_destinations_->setInterceptsMouseClicks(false, true);

  slider_model_lookup_ = sliders;
  std::map<Viewport*, int> num_rotary_meters;
  std::map<Viewport*, int> num_linear_meters;
  for (auto& slider : slider_model_lookup_) {
    if (mono_modulations[slider.first]) {
      bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve();
      Viewport* viewport = slider.second->findParentComponentOfClass<Viewport>();
      if (rotary)
        num_rotary_meters[viewport] = num_rotary_meters[viewport] + 1;
      else
        num_linear_meters[viewport] = num_linear_meters[viewport] + 1;
    }
  }

  for (auto& rotary_meters : num_rotary_meters) {
    rotary_destinations_[rotary_meters.first] = std::make_unique<OpenGlMultiQuad>(rotary_meters.second,
                                                                                  Shaders::kCircleFragment);
    rotary_destinations_[rotary_meters.first]->setTargetComponent(this);
    rotary_destinations_[rotary_meters.first]->setScissorComponent(rotary_meters.first);
    rotary_destinations_[rotary_meters.first]->setAlpha(0.0f, true);

    rotary_meters_[rotary_meters.first] = std::make_unique<OpenGlMultiQuad>(rotary_meters.second,
                                                                            Shaders::kRotaryModulationFragment);
    rotary_meters_[rotary_meters.first]->setTargetComponent(this);
    rotary_meters_[rotary_meters.first]->setScissorComponent(rotary_meters.first);
  }
  for (auto& linear_meters : num_linear_meters) {
    linear_destinations_[linear_meters.first] = std::make_unique<OpenGlMultiQuad>(linear_meters.second,
                                                                                  Shaders::kRoundedRectangleFragment);
    linear_destinations_[linear_meters.first]->setTargetComponent(this);
    linear_destinations_[linear_meters.first]->setScissorComponent(linear_meters.first);
    linear_destinations_[linear_meters.first]->setAlpha(0.0f, true);

    linear_meters_[linear_meters.first] = std::make_unique<OpenGlMultiQuad>(linear_meters.second,
                                                                            Shaders::kLinearModulationFragment);
    linear_meters_[linear_meters.first]->setTargetComponent(this);
    linear_meters_[linear_meters.first]->setScissorComponent(linear_meters.first);
  }

  for (auto& slider : slider_model_lookup_) {
    std::string name = slider.first;
    const vital::Output* mono_total = mono_modulations[name];

    if (mono_total == nullptr)
      continue;

    bool rotary = slider.second->isRotary() && !slider.second->isTextOrCurve();
    Viewport* viewport = slider.second->findParentComponentOfClass<Viewport>();
    const vital::Output* poly_total = poly_modulations[name];

    if (rotary) {
      int index = num_rotary_meters[viewport] - 1;
      num_rotary_meters[viewport] = index;
      createModulationMeter(mono_total, poly_total, slider.second, rotary_meters_[viewport].get(), index);
    }
    else {
      int index = num_linear_meters[viewport] - 1;
      num_linear_meters[viewport] = index;
      createModulationMeter(mono_total, poly_total, slider.second, linear_meters_[viewport].get(), index);
    }

    slider.second->addSliderListener(this);
    createModulationSlider(name, slider.second, poly_total != nullptr);
  }

  addChildComponent(modulation_destinations_.get());

  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    std::string name = "modulation_" + std::to_string(i + 1) + "_amount"; 
    modulation_amount_sliders_[i] = std::make_unique<ModulationAmountKnob>(name, i);
    modulation_amount_sliders_[i]->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    addSlider(modulation_amount_sliders_[i].get());
    modulation_amount_sliders_[i]->addSliderListener(this);
    modulation_amount_sliders_[i]->addModulationAmountListener(this);
    modulation_amount_lookup_[name] = modulation_amount_sliders_[i].get();

    modulation_hover_sliders_[i] = std::make_unique<ModulationAmountKnob>(name, i);
    modulation_hover_sliders_[i]->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    addSlider(modulation_hover_sliders_[i].get());
    modulation_hover_sliders_[i]->setAlpha(0.0f, true);
    modulation_hover_sliders_[i]->addSliderListener(this);
    modulation_hover_sliders_[i]->addModulationAmountListener(this);
    modulation_hover_sliders_[i]->setDrawWhenNotVisible(true);

    selected_modulation_sliders_[i] = std::make_unique<ModulationAmountKnob>(name, i);
    selected_modulation_sliders_[i]->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    addSlider(selected_modulation_sliders_[i].get());
    selected_modulation_sliders_[i]->setAlpha(0.0f, true);
    selected_modulation_sliders_[i]->addSliderListener(this);
    selected_modulation_sliders_[i]->addModulationAmountListener(this);
    selected_modulation_sliders_[i]->setDrawWhenNotVisible(true);
  }
}

void ModulationManager::createModulationMeter(const vital::Output* mono_total,
                                              const vital::Output* poly_total,
                                              SynthSlider* slider, OpenGlMultiQuad* quads, int index) {
  std::string name = slider->getName().toStdString();
  std::unique_ptr<ModulationMeter> meter = std::make_unique<ModulationMeter>(mono_total, poly_total,
                                                                             slider, quads, index);
  addChildComponent(meter.get());
  meter->setName(name);
  meter->setBounds(getLocalArea(slider, slider->getLocalBounds()));
  meter_lookup_[name] = std::move(meter);
}

void ModulationManager::createModulationSlider(std::string name, SynthSlider* slider, bool poly) {
  std::unique_ptr<ModulationDestination> destination = std::make_unique<ModulationDestination>(slider);
  modulation_destinations_->addAndMakeVisible(destination.get());
  destination->setRectangle(slider->isTextOrCurve());
  destination->setRotary(slider->isRotary());
  destination->setSizeMultiple(slider->getKnobSizeScale());

  destination_lookup_[name] = destination.get();
  all_destinations_.push_back(std::move(destination));
}

ModulationManager::~ModulationManager() { }

void ModulationManager::resized() {
  float meter_thickness = findValue(Skin::kKnobModMeterArcThickness);
  Colour meter_center_color = findColour(Skin::kModulationMeter, true);
  Colour meter_left_color = findColour(Skin::kModulationMeterLeft, true);
  Colour meter_right_color = findColour(Skin::kModulationMeterRight, true);

  editing_rotary_amount_quad_.setColor(meter_center_color);
  editing_rotary_amount_quad_.setAltColor(meter_center_color);
  editing_rotary_amount_quad_.setModColor(meter_center_color);
  editing_linear_amount_quad_.setColor(meter_center_color);
  editing_linear_amount_quad_.setAltColor(meter_center_color);
  editing_linear_amount_quad_.setModColor(meter_center_color);

  for (auto& rotary_meter_group : rotary_meters_) {
    rotary_meter_group.second->setThickness(meter_thickness);
    rotary_meter_group.second->setModColor(meter_center_color);
    rotary_meter_group.second->setColor(meter_left_color);
    rotary_meter_group.second->setAltColor(meter_right_color);
  }

  for (auto& linear_meter_group : linear_meters_) {
    linear_meter_group.second->setModColor(meter_center_color);
    linear_meter_group.second->setColor(meter_left_color);
    linear_meter_group.second->setAltColor(meter_right_color);
  }

  modulation_destinations_->setBounds(getLocalBounds());
  modulation_source_meters_->setBounds(getLocalBounds());

  updateModulationMeterLocations();

  Colour meter_control = findColour(Skin::kModulationMeterControl, true);
  current_modulator_quad_.setColor(meter_control);
  drag_quad_.setColor(meter_control);
  drag_quad_.setAltColor(findColour(Skin::kWidgetBackground, true));

  modulation_expansion_box_.setColor(findColour(Skin::kBody, true));

  Colour lighten_screen = findColour(Skin::kLightenScreen, true);
  float rounding = parent_->findValue(Skin::kLabelBackgroundRounding);

  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->setColor(lighten_screen);

  for (auto& linear_destination_group : linear_destinations_) {
    linear_destination_group.second->setColor(lighten_screen);
    linear_destination_group.second->setRounding(rounding);
  }

  SynthSection::resized();
  clearModulationSource();
  positionModulationAmountSliders();
}

void ModulationManager::parentHierarchyChanged() {
  SynthSection::parentHierarchyChanged();
  if (!modulation_source_readouts_.empty())
    return;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (auto& mod_button : modulation_buttons_) {
    modulation_source_readouts_[mod_button.first] = parent->getSynth()->getStatusOutput(mod_button.first);
    smooth_mod_values_[mod_button.first] = 0.0f;
    active_mod_values_[mod_button.first] = false;
  }

  num_voices_readout_ = parent->getSynth()->getStatusOutput("num_voices");
}

void ModulationManager::updateModulationMeterLocations() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();

  for (auto& meter : meter_lookup_) {
    SynthSlider* model = slider_model_lookup_[meter.first];
    if (model)
      meter.second->setBounds(getLocalArea(model, model->getModulationMeterBounds()));

    if (parent) {
      int num_modulations = parent->getSynth()->getNumModulations(meter.first);
      meter.second->setModulated(num_modulations);
      meter.second->setVisible(num_modulations);
    }
  }
}

void ModulationManager::modulationAmountChanged(SynthSlider* slider) {
  std::string slider_name = slider->getName().toStdString();
  std::string source_name = current_modulator_->getName().toStdString();
  setModulationValues(source_name, slider_name,
                      slider->getModulationAmount(), slider->isModulationBipolar(),
                      slider->isModulationStereo(), slider->isModulationBypassed());
  modulation_buttons_[source_name]->repaint();
}

void ModulationManager::modulationRemoved(SynthSlider* slider) {
  std::string slider_name = slider->getName().toStdString();
  std::string source_name = current_modulator_->getName().toStdString();

  removeModulation(source_name, slider_name);
  modulation_buttons_[source_name]->repaint();
}

void ModulationManager::modulationDisconnected(vital::ModulationConnection* connection, bool last) {
  if (current_modulator_ == nullptr)
    return;
  
  if (meter_lookup_.count(connection->destination_name)) {
    meter_lookup_[connection->destination_name]->setModulated(!last);
    meter_lookup_[connection->destination_name]->setVisible(!last);
  }
}

void ModulationManager::modulationSelected(ModulationButton* source) {
  for (auto& button : modulation_buttons_)
    button.second->setActiveModulation(button.second == source);

  current_modulator_ = source;
  for (auto& hover_slider : modulation_hover_sliders_)
    hover_slider->makeVisible(false);
  makeCurrentModulatorAmountsVisible();
  setModulationAmounts();
  positionModulationAmountSliders();
}

void ModulationManager::modulationClicked(ModulationButton* source) {
  hideUnusedHoverModulations();
  positionModulationAmountSliders();
}

void ModulationManager::modulationCleared() {
  makeCurrentModulatorAmountsVisible();
}

bool ModulationManager::hasFreeConnection() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  vital::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    vital::ModulationConnection* connection = bank.atIndex(i);
    if (connection->source_name.empty() && connection->destination_name.empty())
      return true;
  }
  return false;
}

void ModulationManager::startModulationMap(ModulationButton* source, const MouseEvent& e) {
  if (!hasFreeConnection())
    return;
  
  mouse_drag_position_ = getLocalPoint(source, e.getPosition());
  current_source_ = source;
  dragging_ = true;
  Rectangle<int> global_bounds = getLocalArea(current_source_, current_source_->getLocalBounds());
  Point<int> global_start = global_bounds.getCentre();
  mouse_drag_start_ = global_start;
  modulation_destinations_->setVisible(true);
  int widget_margin = findValue(Skin::kWidgetMargin);

  std::map<Viewport*, int> rotary_indices;
  std::map<Viewport*, int> linear_indices;
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_indices[rotary_destination_group.first] = 0;

  for (auto& linear_destination_group : linear_destinations_)
    linear_indices[linear_destination_group.first] = 0;

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::string source_name = source->getName().toStdString();
  std::set<std::string> active_destinations;
  std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getSourceConnections(source_name);
  for (vital::ModulationConnection* connection : connections)
    active_destinations.insert(connection->destination_name);

  for (auto& destination : destination_lookup_) {
    SynthSlider* model = slider_model_lookup_[destination.first];
    bool should_show = model->isShowing() && model->getSectionParent()->isActive() &&
                       current_source_->getName() != String(destination.first);
    Viewport* viewport = model->findParentComponentOfClass<Viewport>();
    destination.second->setVisible(should_show);
    destination.second->setActive(active_destinations.count(destination.first));
    destination.second->setMargin(widget_margin);

    Point<int> position = getLocalPoint(model, Point<int>(0, 0));
    Rectangle<int> slider_bounds = model->getLocalBounds() + position;
    destination.second->setBounds(slider_bounds);

    Component* extra_target = model->getExtraModulationTarget();
    if (extra_target) {
      Rectangle<int> bounds = destination.second->getFillBounds().toNearestInt() + position;

      Point<int> top_left = getLocalPoint(extra_target, Point<int>(0, 0));
      Rectangle<int> extra_bounds(top_left.x, top_left.y, extra_target->getWidth(), extra_target->getHeight());
      bounds = bounds.getUnion(extra_bounds);
      destination.second->setBounds(bounds);
    }

    if (should_show) {
      if (destination.second->isRotary()) {
        destination.second->setIndex(rotary_indices[viewport]);
        rotary_indices[viewport] = rotary_indices[viewport] + 1;
      }
      else {
        destination.second->setIndex(linear_indices[viewport]);
        linear_indices[viewport] = linear_indices[viewport] + 1;
      }
      setDestinationQuadBounds(destination.second);
    }
  }

  for (auto& index_count : rotary_indices) {
    rotary_destinations_[index_count.first]->setNumQuads(index_count.second);
    rotary_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
  }

  for (auto& index_count : linear_indices) {
    linear_destinations_[index_count.first]->setNumQuads(index_count.second);
    linear_destinations_[index_count.first]->setAlpha(index_count.second > 0 ? 1.0f : 0.0f);
  }
}

void ModulationManager::setDestinationQuadBounds(ModulationDestination* destination) {
  Point<float> top_left = destination->getBounds().getTopLeft().toFloat();
  Rectangle<float> draw_bounds = destination->getLocalBounds().toFloat() + top_left;
  if (!destination->hasExtraModulationTarget())
    draw_bounds = destination->getFillBounds() + top_left;
  float global_width = getWidth();
  float global_height = getHeight();
  float x = 2.0f * draw_bounds.getX() / global_width - 1.0f;
  float y = 1.0f - 2.0f * draw_bounds.getBottom() / global_height;
  float width = 2.0f * draw_bounds.getWidth() / global_width;
  float height = 2.0f * draw_bounds.getHeight() / global_height;

  float offset = destination->isActive() ? -2.0f : 0.0f;

  Viewport* viewport = destination->getDestinationSlider()->findParentComponentOfClass<Viewport>();
  if (destination->isRotary())
    rotary_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
  else
    linear_destinations_[viewport]->setQuad(destination->getIndex(), x + offset, y, width, height);
}

void ModulationManager::modulationDraggedToHoverSlider(ModulationAmountKnob* hover_slider) {
  if (hover_slider->isCurrentModulator() || hover_slider->hasAux() || current_modulator_ == nullptr)
    return;

  std::string name = hover_slider->getOriginalName().toStdString();
  std::string source_name = current_modulator_->getName().toStdString();
  vital::ModulationConnection* connection = getConnection(source_name, name);
  if (connection == nullptr) {
    float value = hover_slider->getValue() * 0.5f;
    hover_slider->setValue(0.0f, sendNotificationSync);
    temporarily_set_hover_slider_ = hover_slider;

    connectModulation(source_name, name);
    setModulationValues(source_name, name, value, false, false, false);
    connection = getConnection(source_name, name);

    int new_index = connection->modulation_processor->index();
    addAuxConnection(new_index, hover_slider->index());
    setModulationSliderValues(new_index, value);
  }
}

void ModulationManager::modulationDraggedToComponent(Component* component, bool bipolar) {
  if (component && current_modulator_ && destination_lookup_.count(component->getName().toStdString())) {
    std::string name = component->getName().toStdString();

    if (getConnection(current_modulator_->getName().toStdString(), name) == nullptr) {
      ModulationDestination* destination = destination_lookup_[name];
      SynthSlider* slider = destination->getDestinationSlider();

      float percent = slider->valueToProportionOfLength(slider->getValue());
      float modulation_amount = 1.0f - percent;
      if (bipolar)
        modulation_amount = std::min(modulation_amount, percent) * 2.0f;
      modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

      temporarily_set_destination_ = destination;
      temporarily_set_synth_slider_ = slider_model_lookup_[name];

      std::string source_name = current_modulator_->getName().toStdString();
      connectModulation(source_name, name);
      setModulationValues(source_name, name, modulation_amount, bipolar, false, false);
      destination->setActive(true);
      setDestinationQuadBounds(destination);

      SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
      std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getDestinationConnections(name);

      for (vital::ModulationConnection* connection : connections) {
        if (connection->source_name == source_name && connection->destination_name == name) {
          int index = connection->modulation_processor->index();
          showModulationAmountOverlay(selected_modulation_sliders_[index].get());
        }
      }

      setVisibleMeterBounds();
      makeModulationsVisible(slider, true);
    }
    else
      modulationsChanged(name);
  }
}

void ModulationManager::setTemporaryModulationBipolar(Component* component, bool bipolar) {
  if (current_modulator_ == nullptr || component != temporarily_set_destination_ || component == nullptr)
    return;

  std::string source_name = current_modulator_->getName().toStdString();
  std::string name = component->getName().toStdString();
  ModulationDestination* destination = destination_lookup_[name];
  SynthSlider* slider = destination->getDestinationSlider();

  float percent = slider->valueToProportionOfLength(slider->getValue());
  float modulation_amount = 1.0f - percent;
  if (bipolar)
    modulation_amount = std::min(modulation_amount, percent) * 2.0f;
  modulation_amount = std::max(modulation_amount, kDefaultModulationRatio);

  int index = getModulationIndex(source_name, name);
  setModulationValues(source_name, name, modulation_amount, bipolar, false, false);
  temporarily_set_bipolar_ = bipolar;
  showModulationAmountOverlay(selected_modulation_sliders_[index].get());
}

void ModulationManager::clearTemporaryModulation() {
  if (temporarily_set_destination_ && current_modulator_) {
    temporarily_set_destination_->setActive(false);
    setDestinationQuadBounds(temporarily_set_destination_);
    temporarily_set_destination_ = nullptr;
    std::string source_name = current_modulator_->getName().toStdString();
    removeModulation(source_name, temporarily_set_synth_slider_->getName().toStdString());
    temporarily_set_synth_slider_ = nullptr;

    hideModulationAmountOverlay();
  }
}

void ModulationManager::clearTemporaryHoverModulation() {
  if (temporarily_set_hover_slider_ && current_modulator_) {
    std::string name = temporarily_set_hover_slider_->getOriginalName().toStdString();

    std::string source_name = current_modulator_->getName().toStdString();
    removeModulation(source_name, temporarily_set_hover_slider_->getOriginalName().toStdString());
    temporarily_set_hover_slider_ = nullptr;
  }
}

void ModulationManager::modulationDragged(const MouseEvent& e) {
  if (!dragging_)
    return;
  
  mouse_drag_position_ = getLocalPoint(current_source_, e.getPosition());
  Component* component = getComponentAt(mouse_drag_position_.x, mouse_drag_position_.y);
  ModulationAmountKnob* hover_knob = nullptr;
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    if (modulation_amount_sliders_[i].get() == component)
      hover_knob = modulation_amount_sliders_[i].get();
    else if (modulation_hover_sliders_[i].get() == component)
      hover_knob = modulation_hover_sliders_[i].get();
    else if (selected_modulation_sliders_[i].get() == component)
      hover_knob = selected_modulation_sliders_[i].get();
  }

  if (hover_knob && hover_knob->isCurrentModulator())
    return;

  bool bipolar = e.mods.isAnyModifierKeyDown();
  if (temporarily_set_destination_ && temporarily_set_destination_ != component)
    clearTemporaryModulation();
  if (temporarily_set_hover_slider_ && temporarily_set_hover_slider_ != component)
    clearTemporaryHoverModulation();

  else if (temporarily_set_synth_slider_ && temporarily_set_bipolar_ != bipolar)
    setTemporaryModulationBipolar(component, bipolar);

  if (hover_knob)
    modulationDraggedToHoverSlider(hover_knob);
  else
    modulationDraggedToComponent(component, bipolar);
}

void ModulationManager::modulationWheelMoved(const MouseEvent& e, const MouseWheelDetails& wheel) {
  if (!dragging_ || current_modulator_ == nullptr || temporarily_set_destination_ == nullptr)
    return;

  MouseEvent new_event(e.source, e.position, ModifierKeys(), e.pressure, e.orientation, e.rotation,
                       e.tiltX, e.tiltY, e.eventComponent, e.originalComponent, e.eventTime, e.mouseDownPosition,
                       e.mouseDownTime, e.getNumberOfClicks(), e.mouseWasDraggedSinceMouseDown());
  std::string source_name = current_modulator_->getName().toStdString();
  std::string destination_name = temporarily_set_destination_->getName().toStdString();
  int index = getModulationIndex(source_name, destination_name);
  if (index >= 0)
    selected_modulation_sliders_[index]->mouseWheelMove(new_event, wheel);
}

void ModulationManager::endModulationMap() {
  temporarily_set_destination_ = nullptr;
  temporarily_set_synth_slider_ = nullptr;
  temporarily_set_hover_slider_ = nullptr;
  dragging_ = false;

  setModulationAmounts();
  positionModulationAmountSliders();
  current_source_ = nullptr;
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->setAlpha(0.0f);

  for (auto& linear_destination_group : linear_destinations_)
    linear_destination_group.second->setAlpha(0.0f);

  modulation_destinations_->setVisible(false);
  drag_quad_.setThickness(0.0f, true);
  hideModulationAmountOverlay();
}

void ModulationManager::modulationLostFocus(ModulationButton* source) {
  source->setActiveModulation(false);
  clearModulationSource();
}

void ModulationManager::clearModulationSource() {
  if (current_modulator_) {
    for (auto& selected_slider : selected_modulation_sliders_)
      selected_slider->makeVisible(false);
  }
  current_modulator_ = nullptr;
  setModulationAmounts();
}

void ModulationManager::disconnectModulation(ModulationAmountKnob* modulation_knob) {
  vital::ModulationConnection* connection = getConnectionForModulationSlider(modulation_knob);
  while (connection && !connection->source_name.empty() && !connection->destination_name.empty()) {
    removeModulation(connection->source_name, connection->destination_name);
    connection = getConnectionForModulationSlider(modulation_knob);
  }

  setModulationAmounts();
}

void ModulationManager::setModulationSettings(ModulationAmountKnob* modulation_knob) {
  vital::ModulationConnection* connection = getConnectionForModulationSlider(modulation_knob);
  float value = modulation_knob->getValue();
  bool bipolar = modulation_knob->isBipolar();
  bool stereo = modulation_knob->isStereo();
  bool bypass = modulation_knob->isBypass();

  int index = modulation_knob->index();
  modulation_amount_sliders_[index]->setBipolar(bipolar);
  modulation_amount_sliders_[index]->setStereo(stereo);
  modulation_amount_sliders_[index]->setBypass(bypass);
  modulation_hover_sliders_[index]->setBipolar(bipolar);
  modulation_hover_sliders_[index]->setStereo(stereo);
  modulation_hover_sliders_[index]->setBypass(bypass);
  selected_modulation_sliders_[index]->setBipolar(bipolar);
  selected_modulation_sliders_[index]->setStereo(stereo);
  selected_modulation_sliders_[index]->setBypass(bypass);

  setModulationValues(connection->source_name, connection->destination_name, value, bipolar, stereo, bypass);
}

void ModulationManager::setModulationBypass(ModulationAmountKnob* modulation_knob, bool bypass) {
  setModulationSettings(modulation_knob);
}

void ModulationManager::setModulationBipolar(ModulationAmountKnob* modulation_knob, bool bipolar) {
  setModulationSettings(modulation_knob);
}

void ModulationManager::setModulationStereo(ModulationAmountKnob* modulation_knob, bool stereo) {
  setModulationSettings(modulation_knob);
}

void ModulationManager::initOpenGlComponents(OpenGlWrapper& open_gl) {
  drag_quad_.init(open_gl);
  modulation_expansion_box_.init(open_gl);
  modulation_source_meters_->init(open_gl);
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->init(open_gl);

  for (auto& linear_destination_group : linear_destinations_)
    linear_destination_group.second->init(open_gl);

  for (auto& rotary_meter_group : rotary_meters_)
    rotary_meter_group.second->init(open_gl);

  for (auto& linear_meter_group : linear_meters_)
    linear_meter_group.second->init(open_gl);

  SynthSection::initOpenGlComponents(open_gl);
}

void ModulationManager::drawModulationDestinations(OpenGlWrapper& open_gl) {
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->render(open_gl, true);

  for (auto& linear_destination_group : linear_destinations_)
    linear_destination_group.second->render(open_gl, true);
}

void ModulationManager::drawCurrentModulator(OpenGlWrapper& open_gl) {
  Component* component = current_modulator_;
  if (component) {
    current_modulator_quad_.setTargetComponent(component);
    current_modulator_quad_.setAlpha(1.0f);
  }
  else
    current_modulator_quad_.setAlpha(0.0f);

  current_modulator_quad_.setThickness(dragging_ ? 2.6f : 1.0f);
  current_modulator_quad_.render(open_gl, true);
}

void ModulationManager::drawDraggingModulation(OpenGlWrapper& open_gl) {
  static constexpr float kRadiusWidthRatio = 0.022f;
  static constexpr float kThicknessWidthRatio = 0.003f;
  if (current_source_ == nullptr || temporarily_set_destination_ || temporarily_set_hover_slider_)
    return;

  vital::poly_float mod_percent = modulation_source_readouts_[current_source_->getName().toStdString()]->value();
  float draw_radius = kRadiusWidthRatio * getWidth();
  float radius_x = draw_radius / getWidth();
  float radius_y = draw_radius / getHeight();
  float x = mouse_drag_position_.x * 2.0f / getWidth() - 1.0f;
  float y = -mouse_drag_position_.y * 2.0f / getHeight() + 1.0f;

  Colour widget_background = findColour(Skin::kWidgetBackground, true);
  Colour control = findColour(Skin::kModulationMeterControl, true);
  drag_quad_.setAltColor(widget_background.interpolatedWith(control, mod_percent[0]));
  drag_quad_.setQuad(0, x - radius_x, y - radius_y, 2.0f * radius_x, 2.0f * radius_y);
  drag_quad_.setThickness(getWidth() * kThicknessWidthRatio);
  drag_quad_.render(open_gl, true);
}

void ModulationManager::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  if (!animate)
    return;

  drawCurrentModulator(open_gl);
  for (auto& callout_button : modulation_callout_buttons_) {
    if (callout_button.second->isVisible())
      callout_button.second->renderSliderQuads(open_gl, animate);
  }

  OpenGlComponent::setViewPort(this, open_gl);
  drawModulationDestinations(open_gl);

  Colour first_color = findColour(Skin::kWidgetPrimary1, true);
  Colour second_color = findColour(Skin::kWidgetPrimary2, true);

  modulation_source_meters_->setAdditiveBlending(second_color.getBrightness() > 0.5f);
  modulation_source_meters_->setColor(second_color);
  renderSourceMeters(open_gl, 1);
  modulation_source_meters_->setAdditiveBlending(first_color.getBrightness() > 0.5f);
  modulation_source_meters_->setColor(first_color);
  renderSourceMeters(open_gl, 0);
  updateSmoothModValues();

  editing_rotary_amount_quad_.render(open_gl, animate);
  editing_linear_amount_quad_.render(open_gl, animate);

  SynthSection::renderOpenGlComponents(open_gl, animate);

  drawDraggingModulation(open_gl);
}

void ModulationManager::renderMeters(OpenGlWrapper& open_gl, bool animate) {
  if (!animate)
    return;

  int num_voices = 1;
  if (num_voices_readout_)
    num_voices = std::max<float>(0.0f, num_voices_readout_->value()[0]);
  for (auto& meter : meter_lookup_) {
    SynthSlider* slider = slider_model_lookup_[meter.first];
    bool show = meter.second->isModulated() && showingInParents(slider) && slider->isActive();
    meter.second->setActive(show);
    if (show)
      meter.second->updateDrawing(num_voices);
  }

  OpenGlComponent::setViewPort(this, open_gl);
  for (auto& rotary_meter_group : rotary_meters_)
    rotary_meter_group.second->render(open_gl, animate);

  for (auto& linear_meter_group : linear_meters_)
    linear_meter_group.second->render(open_gl, animate);
}

void ModulationManager::renderSourceMeters(OpenGlWrapper& open_gl, int index) {
  int i = 0;
  float width = getWidth();
  float height = getHeight();
  for (auto& mod_readout : modulation_source_readouts_) {
    ModulationButton* button = modulation_buttons_[mod_readout.first];
    float readout_value = mod_readout.second->value()[index];

    float clamped_value = vital::utils::clamp(readout_value, 0.0f, 1.0f);
    if (!active_mod_values_[mod_readout.first] && !mod_readout.second->isClearValue(readout_value))
      smooth_mod_values_[mod_readout.first].set(index, clamped_value);
    float smooth_value = smooth_mod_values_[mod_readout.first][index];

    Rectangle<int> bounds = getLocalArea(button, button->getMeterBounds());
    float left = 2.0f * ((bounds.getX() - 1.0f) / width) - 1.0f + kModSourceMeterBuffer;
    float w = 2.0f * bounds.getWidth() / width - 2.0f * kModSourceMeterBuffer;
    float h = 2.0f * bounds.getHeight() / height - 2.0f * kModSourceMeterBuffer;
    float y = 1.0f - 2.0f * bounds.getY() / height - kModSourceMeterBuffer;
    float y_center = y - h * (1.0f - clamped_value);
    float smooth_y_center = y - h * (1.0f - smooth_value);

    float top = std::min(y_center, smooth_y_center) - kModSourceMinRadius;
    float bottom = std::max(y_center, smooth_y_center) + kModSourceMinRadius;

    bool active = button->isActiveModulation() || button->hasAnyModulation();
    if (w <= 0.0f || mod_readout.second->isClearValue(readout_value) || !showingInParents(button) || !active) {
      left = -2.0f;
      top = -2.0f;
      bottom = -2.0f;
    }

    modulation_source_meters_->positionBar(i, left, top, w, bottom - top);

    i++;
  }

  modulation_source_meters_->render(open_gl, true);
}

void ModulationManager::updateSmoothModValues() {
  static constexpr float kTimeDecayScale = 60.0f;
  long long current_milliseconds = Time::currentTimeMillis();
  long long delta_milliseconds = current_milliseconds - last_milliseconds_;
  last_milliseconds_ = current_milliseconds;

  float seconds = delta_milliseconds / 1000.0f;
  float decay = std::max(std::min(kModSmoothDecay * seconds * kTimeDecayScale, 1.0f), 0.0f);

  for (auto& mod_readout : modulation_source_readouts_) {
    vital::poly_float readout_value = mod_readout.second->value();
    vital::poly_float clamped_value = vital::utils::clamp(readout_value, 0.0f, 1.0f);
    vital::poly_float smooth_value = smooth_mod_values_[mod_readout.first];
    bool active = !mod_readout.second->isClearValue(readout_value);
    active_mod_values_[mod_readout.first] = active;
    if (active)
      smooth_mod_values_[mod_readout.first] = vital::utils::interpolate(smooth_value, clamped_value, decay);
  }
}

void ModulationManager::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  SynthSection::destroyOpenGlComponents(open_gl);

  drag_quad_.destroy(open_gl);
  modulation_expansion_box_.destroy(open_gl);
  modulation_source_meters_->destroy(open_gl);
  for (auto& rotary_destination_group : rotary_destinations_)
    rotary_destination_group.second->destroy(open_gl);

  for (auto& linear_destination_group : linear_destinations_)
    linear_destination_group.second->destroy(open_gl);

  for (auto& rotary_meter_group : rotary_meters_)
    rotary_meter_group.second->destroy(open_gl);

  for (auto& linear_meter_group : linear_meters_)
    linear_meter_group.second->destroy(open_gl);
}

void ModulationManager::showModulationAmountOverlay(ModulationAmountKnob* slider) {
  vital::ModulationConnection* connection = getConnection(slider->index());
  if (connection == nullptr || meter_lookup_.count(connection->destination_name) == 0)
    return;

  ModulationMeter* meter = meter_lookup_[connection->destination_name].get();
  if (!meter->destination()->isShowing())
    return;

  if (meter->isRotary()) {
    editing_rotary_amount_quad_.setTargetComponent(meter);
    editing_rotary_amount_quad_.setAdditive(false);
    meter->setAmountQuadVertices(editing_rotary_amount_quad_);
    meter->setModulationAmountQuad(editing_rotary_amount_quad_, slider->getValue(), slider->isBipolar());

    editing_rotary_amount_quad_.setThickness(2.0f);
    editing_rotary_amount_quad_.setAlpha(1.0f);
    editing_rotary_amount_quad_.setActive(true);
  }
  else {
    editing_linear_amount_quad_.setTargetComponent(meter);
    editing_linear_amount_quad_.setAdditive(false);
    meter->setAmountQuadVertices(editing_linear_amount_quad_);
    meter->setModulationAmountQuad(editing_linear_amount_quad_, slider->getValue(), slider->isBipolar());

    editing_linear_amount_quad_.setAlpha(1.0f);
    editing_linear_amount_quad_.setActive(true);
  }
}

void ModulationManager::hideModulationAmountOverlay() {
  if (changing_hover_modulation_)
    return;

  editing_rotary_amount_quad_.setAlpha(0.0f);
  editing_linear_amount_quad_.setAlpha(0.0f);
}

void ModulationManager::hoverStarted(SynthSlider* slider) {
  if (changing_hover_modulation_)
    return;

  if (!enteringHoverValue())
    makeModulationsVisible(slider, true);

  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob)
    showModulationAmountOverlay(amount_knob);
  else
    hideModulationAmountOverlay();
}

void ModulationManager::hoverEnded(SynthSlider* slider) {
  hideModulationAmountOverlay();
}

void ModulationManager::menuFinished(SynthSlider* slider) {
  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();
}

void ModulationManager::modulationsChanged(const std::string& destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  hideUnusedHoverModulations();
  SynthSlider* slider = slider_model_lookup_[destination];
  if (current_modulator_)
    makeCurrentModulatorAmountsVisible();
  else if (slider)
    makeModulationsVisible(slider, slider->isShowing());

  if (parent == nullptr)
    return;

  if (meter_lookup_.count(destination) == 0)
    return;
  
  int num_modulations = parent->getSynth()->getNumModulations(destination);
  meter_lookup_[destination]->setModulated(num_modulations);
  meter_lookup_[destination]->setVisible(num_modulations);
}

int ModulationManager::getModulationIndex(std::string source, std::string destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getDestinationConnections(destination);

  for (vital::ModulationConnection* connection : connections) {
    if (connection->source_name == source)
      return connection->modulation_processor->index();
  }

  return -1;
}

int ModulationManager::getIndexForModulationSlider(Slider* slider) {
  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob)
    return amount_knob->index();
  return -1;
}

vital::ModulationConnection* ModulationManager::getConnectionForModulationSlider(Slider* slider) {
  int index = getIndexForModulationSlider(slider);
  if (index < 0)
    return nullptr;

  while (aux_connections_to_from_.count(index))
    index = aux_connections_to_from_[index];

  return getConnection(index);
}

vital::ModulationConnection* ModulationManager::getConnection(int index) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return nullptr;

  return parent->getSynth()->getModulationBank().atIndex(index);
}

vital::ModulationConnection* ModulationManager::getConnection(const std::string& source, const std::string& dest) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return nullptr;

  std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getSourceConnections(source);
  for (vital::ModulationConnection* connection : connections) {
    if (connection->destination_name == dest)
      return connection;
  }

  return nullptr;
}

void ModulationManager::mouseDown(SynthSlider* slider) {
  for (auto& amount_knob : modulation_hover_sliders_) {
    if (slider == amount_knob.get())
      return;
  }

  if (modulation_expansion_box_.isVisible())
    return;

  vital::ModulationConnection* connection = getConnectionForModulationSlider(slider);
  if (connection && !connection->source_name.empty() && !connection->destination_name.empty())
    modulationSelected(modulation_buttons_[connection->source_name]);
  else {
    clearModulationSource();
    hideModulationAmountOverlay();
    makeModulationsVisible(slider, true);
  }
}

void ModulationManager::mouseUp(SynthSlider* slider) {
  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();
}

void ModulationManager::doubleClick(SynthSlider* slider) {
  changing_hover_modulation_ = false;
  vital::ModulationConnection* connection = getConnectionForModulationSlider(slider);
  if (connection)
    removeModulation(connection->source_name, connection->destination_name);
  setModulationAmounts();

  if (current_modulator_ && current_modulator_->isVisible())
    current_modulator_->grabKeyboardFocus();
}

void ModulationManager::beginModulationEdit(SynthSlider* slider) {
  changing_hover_modulation_ = true;
}

void ModulationManager::endModulationEdit(SynthSlider* slider) {
  changing_hover_modulation_ = false;
}

void ModulationManager::sliderValueChanged(Slider* slider) {
  ModulationAmountKnob* amount_knob = dynamic_cast<ModulationAmountKnob*>(slider);
  if (amount_knob == nullptr)
    return;

  float value = slider->getValue();
  int index = getIndexForModulationSlider(slider);
  float scale = getAuxMultiplier(index);
  float scaled_value = value * scale;
  while (aux_connections_to_from_.count(index))
    index = aux_connections_to_from_[index];

  vital::ModulationConnection* connection = getConnection(index);
  bool bipolar = connection->modulation_processor->isBipolar();
  bool stereo = connection->modulation_processor->isStereo();
  bool bypass = connection->modulation_processor->isBypassed();

  setModulationValues(connection->source_name, connection->destination_name, scaled_value, bipolar, stereo, bypass);
  showModulationAmountOverlay(amount_knob);

  SynthSection::sliderValueChanged(modulation_amount_sliders_[index].get());
}

void ModulationManager::buttonClicked(Button* button) {
  for (auto& callout_button : modulation_callout_buttons_) {
    if (button == callout_button.second.get()) {
      bool new_button = button != current_expanded_modulation_;
      hideModulationAmountCallout();
      if (new_button)
        showModulationAmountCallout(callout_button.first);
      return;
    }
  }

  SynthSection::buttonClicked(button);
}

void ModulationManager::connectModulation(std::string source, std::string destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  modifying_ = true;
  parent->connectModulation(source, destination);
  modifying_ = false;
}

void ModulationManager::removeModulation(std::string source, std::string destination) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  vital::ModulationConnection* connection = getConnection(source, destination);
  if (connection == nullptr) {
    positionModulationAmountSliders();
    return;
  }
  
  int index = connection->modulation_processor->index();
  if (aux_connections_from_to_.count(index)) {
    float current_value = connection->modulation_processor->currentBaseValue();
    int dest_index = aux_connections_from_to_[index];
    ModulationAmountKnob* modulation_amount = modulation_amount_sliders_[dest_index].get();
    removeAuxSourceConnection(index);
    float reset_value = current_value == 0.0f ? 1.0f : -current_value;
    modulation_amount->setValue(reset_value, dontSendNotification);
    modulation_amount->setValue(current_value * 2.0f, sendNotificationSync);
  }
  else
    removeAuxSourceConnection(index);

  modifying_ = true;
  parent->disconnectModulation(source, destination);
  modulationsChanged(destination);
  modifying_ = false;
  positionModulationAmountSliders();
}

void ModulationManager::setModulationSliderValue(int index, float value) {
  modulation_amount_sliders_[index]->setValue(value, dontSendNotification);
  modulation_hover_sliders_[index]->setValue(value, dontSendNotification);
  selected_modulation_sliders_[index]->setValue(value, dontSendNotification);
  modulation_amount_sliders_[index]->redoImage();
  modulation_hover_sliders_[index]->redoImage();
  selected_modulation_sliders_[index]->redoImage();
}

void ModulationManager::setModulationSliderBipolar(int index, bool bipolar) {
  modulation_amount_sliders_[index]->setBipolar(bipolar);
  modulation_hover_sliders_[index]->setBipolar(bipolar);
  selected_modulation_sliders_[index]->setBipolar(bipolar);
}

void ModulationManager::setModulationSliderValues(int index, float value) {
  setModulationSliderValue(index, value);
  float from_value = value;
  int from_index = index;
  while (aux_connections_from_to_.count(from_index)) {
    from_index = aux_connections_from_to_[from_index];
    from_value *= 2.0f;
    setModulationSliderValue(from_index, from_value);
  }

  float to_value = value;
  int to_index = index;
  while (aux_connections_to_from_.count(to_index)) {
    to_index = aux_connections_to_from_[to_index];
    to_value *= 0.5f;
    setModulationSliderValue(to_index, to_value);
  }

  setModulationSliderScale(index);
}

void ModulationManager::setModulationSliderScale(int index) {
  int end_index = index;
  float scale = 1.0f;
  while (aux_connections_from_to_.count(end_index)) {
    end_index = aux_connections_from_to_[end_index];
    scale *= 2.0f;
  }

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  vital::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  vital::ModulationConnection* connection = bank.atIndex(end_index);
  if (!connection->destination_name.empty()) {
    vital::ValueDetails details = vital::Parameters::getDetails(connection->destination_name);
    if (details.value_scale == vital::ValueDetails::kLinear || details.value_scale == vital::ValueDetails::kIndexed) {
      float display_multiply = scale * (details.max - details.min);
      modulation_amount_sliders_[index]->setDisplayMultiply(display_multiply);
      modulation_hover_sliders_[index]->setDisplayMultiply(display_multiply);
      selected_modulation_sliders_[index]->setDisplayMultiply(display_multiply);
      return;
    }
  }
  modulation_amount_sliders_[index]->setDisplayMultiply(1.0f);
  modulation_hover_sliders_[index]->setDisplayMultiply(1.0f);
  selected_modulation_sliders_[index]->setDisplayMultiply(1.0f);
}

void ModulationManager::setModulationValues(std::string source, std::string destination,
                                            vital::mono_float amount, bool bipolar, bool stereo, bool bypass) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || source.empty() || destination.empty())
    return;

  modifying_ = true;
  parent->setModulationValues(source, destination, amount, bipolar, stereo, bypass);
  int index = getModulationIndex(source, destination);
  parent->notifyModulationValueChanged(index);
  setModulationSliderValues(index, amount);
  setModulationSliderBipolar(index, bipolar);

  modifying_ = false;
}

void ModulationManager::initAuxConnections() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    modulation_amount_sliders_[i]->removeAux();
    modulation_hover_sliders_[i]->removeAux();
    selected_modulation_sliders_[i]->removeAux();
  }

  aux_connections_from_to_.clear();
  aux_connections_to_from_.clear();

  vital::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    vital::ModulationConnection* connection = bank.atIndex(i);
    int index = connection->modulation_processor->index();

    if (modulation_amount_lookup_.count(connection->destination_name)) {
      int modulation_index = modulation_amount_lookup_[connection->destination_name]->index();
      addAuxConnection(index, modulation_index);
    }
  }
}

void ModulationManager::reset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  for (auto& meter : meter_lookup_) {
    int num_modulations = parent->getSynth()->getNumModulations(meter.first);
    meter.second->setModulated(num_modulations);
    meter.second->setVisible(num_modulations);
  }

  for (auto& button : modulation_buttons_)
    button.second->setActiveModulation(button.second->isActiveModulation());

  setModulationAmounts();
  if (getWidth() > 0)
    positionModulationAmountSliders();
  initAuxConnections();
}

void ModulationManager::hideUnusedHoverModulations() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || changing_hover_modulation_)
    return;

  vital::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    vital::ModulationConnection* connection = bank.atIndex(i);
    int index = connection->modulation_processor->index();
    if (connection->source_name.empty() || connection->destination_name.empty())
      modulation_hover_sliders_[index]->hideImmediately();
    else {
      SynthSlider* slider = slider_model_lookup_[connection->destination_name];
      if (slider == nullptr || !slider->isShowing())
        modulation_hover_sliders_[index]->hideImmediately();
    }
  }
}

float ModulationManager::getAuxMultiplier(int index) {
  float mult = 1.0f;
  while (aux_connections_to_from_.count(index)) {
    index = aux_connections_to_from_[index];
    mult *= 0.5f;
  }

  return mult;
}

void ModulationManager::addAuxConnection(int from_index, int to_index) {
  if (from_index == to_index)
    return;

  aux_connections_to_from_[to_index] = from_index;
  aux_connections_from_to_[from_index] = to_index;
  std::string aux_name = "modulation_" + std::to_string(from_index + 1) + "_amount";
  modulation_hover_sliders_[to_index]->setAux(aux_name);
  modulation_amount_sliders_[to_index]->setAux(aux_name);
}

void ModulationManager::removeAuxSourceConnection(int from_index) {
  if (aux_connections_from_to_.count(from_index) == 0)
    return;

  int to_index = aux_connections_from_to_[from_index];
  modulation_hover_sliders_[to_index]->removeAux();
  modulation_amount_sliders_[to_index]->removeAux();
  aux_connections_from_to_.erase(from_index);
  aux_connections_to_from_.erase(to_index);
}

void ModulationManager::removeAuxDestinationConnection(int to_index) {
  if (aux_connections_to_from_.count(to_index) == 0)
    return;

  modulation_hover_sliders_[to_index]->removeAux();
  modulation_amount_sliders_[to_index]->removeAux();
  aux_connections_from_to_.erase(aux_connections_to_from_[to_index]);
  aux_connections_to_from_.erase(to_index);
}

void ModulationManager::makeCurrentModulatorAmountsVisible() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (current_modulator_ == nullptr || parent == nullptr)
    return;

  std::string source_name = current_modulator_->getName().toStdString();
  std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getSourceConnections(source_name);
  std::set<ModulationAmountKnob*> selected_modulation_sliders;

  int width = size_ratio_ * 24.0f;
  for (vital::ModulationConnection* connection : connections) {
    int index = connection->modulation_processor->index();
    ModulationAmountKnob* selected_slider = selected_modulation_sliders_[index].get();
    selected_slider->setCurrentModulator(true);
    selected_modulation_sliders.insert(selected_slider);
    if (!selected_slider->hasAux()) {
      selected_slider->setValue(connection->modulation_processor->currentBaseValue(), dontSendNotification);
      selected_slider->redoImage();
    }
    selected_slider->setSource(connection->source_name);
    selected_slider->setCurrentModulator(connection->source_name == source_name);
    selected_slider->setBipolar(connection->modulation_processor->isBipolar());
    selected_slider->setStereo(connection->modulation_processor->isStereo());
    selected_slider->setBypass(connection->modulation_processor->isBypassed());
    
    if (slider_model_lookup_.count(connection->destination_name) == 0)
      continue;

    SynthSlider* destination_slider = slider_model_lookup_[connection->destination_name];
    if (slider_model_lookup_[connection->destination_name] == nullptr)
      return;
    Rectangle<int> destination_bounds = getLocalArea(destination_slider, destination_slider->getLocalBounds());

    int center_x = destination_bounds.getCentreX();
    int left = destination_bounds.getX();
    int right = destination_bounds.getRight();

    int bottom = destination_bounds.getBottom();
    int top = destination_bounds.getY();
    int center_y = destination_bounds.getCentreY();

    BubbleComponent::BubblePlacement placement = destination_slider->getModulationPlacement();
    selected_slider->setPopupPlacement(placement);
    if (placement == BubbleComponent::below)
      selected_slider->setBounds(center_x - width / 2, bottom, width, width);
    else if (placement == BubbleComponent::above)
      selected_slider->setBounds(center_x - width / 2, top - width, width, width);
    else if (placement == BubbleComponent::left)
      selected_slider->setBounds(left - width, center_y - width / 2, width, width);
    else
      selected_slider->setBounds(right, center_y - width / 2, width, width);

    selected_slider->makeVisible(destination_slider->isShowing());
  }

  for (auto& selected_slider : selected_modulation_sliders_) {
    if (selected_modulation_sliders.count(selected_slider.get()) == 0)
      selected_slider->makeVisible(false);
  }
}

void ModulationManager::makeModulationsVisible(SynthSlider* destination, bool visible) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (destination == nullptr || parent == nullptr || changing_hover_modulation_)
    return;

  std::string name = destination->getName().toStdString();
  if (slider_model_lookup_[name] != destination)
    return;

  std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getDestinationConnections(name);
  std::vector<ModulationAmountKnob*> modulation_hover_sliders;

  bool current_modulation_showing = false;
  for (vital::ModulationConnection* connection : connections) {
    int index = connection->modulation_processor->index();
    ModulationAmountKnob* hover_slider = modulation_hover_sliders_[index].get();
    if (current_modulator_ && current_modulator_->getName() == String(connection->source_name))
      current_modulation_showing = true;
    else
      modulation_hover_sliders.push_back(hover_slider);
    if (!hover_slider->hasAux()) {
      hover_slider->setValue(connection->modulation_processor->currentBaseValue(), dontSendNotification);
      hover_slider->redoImage();
    }
    hover_slider->setSource(connection->source_name);
    hover_slider->setBipolar(connection->modulation_processor->isBipolar());
    hover_slider->setStereo(connection->modulation_processor->isStereo());
    hover_slider->setBypass(connection->modulation_processor->isBypassed());
  }

  int hover_slider_width = size_ratio_ * 24.0f;
  if (current_modulation_showing) {
    auto position = modulation_hover_sliders.begin() + (modulation_hover_sliders.size() + 1) / 2;
    modulation_hover_sliders.insert(position, nullptr);
    if (modulation_hover_sliders.size() % 2 == 0)
      modulation_hover_sliders.insert(modulation_hover_sliders.end(), nullptr);
  }
  int num_sliders = (int)modulation_hover_sliders.size();

  Rectangle<int> destination_bounds = getLocalArea(destination, destination->getLocalBounds());
  int x = destination_bounds.getRight();
  int y = destination_bounds.getBottom();
  int beginning_offset = hover_slider_width * num_sliders / 2;
  int delta_x = 0;
  int delta_y = 0;

  BubbleComponent::BubblePlacement placement = destination->getModulationPlacement();
  if (placement == BubbleComponent::below) {
    x = destination_bounds.getCentreX() - beginning_offset;
    delta_x = hover_slider_width;
  }
  else if (placement == BubbleComponent::above) {
    x = destination_bounds.getCentreX() - beginning_offset;
    y = destination_bounds.getY() - hover_slider_width;
    delta_x = hover_slider_width;
  }
  else if (placement == BubbleComponent::left) {
    x = destination_bounds.getX() - hover_slider_width;
    y = destination_bounds.getCentreY() - beginning_offset;
    delta_y = hover_slider_width;
  }
  else {
    y = destination_bounds.getCentreY() - beginning_offset;
    delta_y = hover_slider_width;
  }

  std::unordered_set<ModulationAmountKnob*> lookup(modulation_hover_sliders.begin(), modulation_hover_sliders.end());
  for (auto& hover_slider : modulation_hover_sliders_) {
    if (lookup.count(hover_slider.get()) == 0)
      hover_slider->makeVisible(false);
  }

  for (ModulationAmountKnob* hover_slider : modulation_hover_sliders) {
    if (hover_slider) {
      hover_slider->setPopupPlacement(placement);
      hover_slider->setBounds(x, y, hover_slider_width, hover_slider_width);
      hover_slider->makeVisible(visible);
      hover_slider->redoImage();
    }
    x += delta_x;
    y += delta_y;
  }
}

void ModulationManager::positionModulationAmountSlidersInside(const std::string& source,
                                                              std::vector<vital::ModulationConnection*> connections) {
  static constexpr float kRightPopupPositionX = 150;
  int total_connections = static_cast<int>(connections.size());
  ModulationButton* modulation_button = modulation_buttons_[source];
  ExpandModulationButton* expand_button = modulation_callout_buttons_[source].get();
  expand_button->setVisible(false);

  if (expand_button == current_expanded_modulation_)
    hideModulationAmountCallout();

  for (int i = 0; i < total_connections; ++i) {
    vital::ModulationConnection* connection = connections[i];
    int index = connection->modulation_processor->index();
    ModulationAmountKnob* slider = modulation_amount_sliders_[index].get();
    slider->setVisible(showingInParents(modulation_button));
    Point<int> point = getLocalPoint(modulation_button, Point<int>(0, 0));
    slider->setBounds(modulation_button->getModulationAmountBounds(i, total_connections) + point);

    BubbleComponent::BubblePlacement popup_position = BubbleComponent::below;
    if (slider->getX() < kRightPopupPositionX)
      popup_position = BubbleComponent::right;
    if (getWidth() - slider->getRight() < kRightPopupPositionX)
      popup_position = BubbleComponent::left;
    slider->setPopupPlacement(popup_position);

    std::string name = connection->destination_name;
    if (slider_model_lookup_.count(name))
      slider->setDestinationComponent(slider_model_lookup_[name], name);
    else
      slider->setDestinationComponent(nullptr, name);

    slider->setMouseClickGrabsKeyboardFocus(true);
    slider->redoImage();
  }
}

void ModulationManager::positionModulationAmountSlidersCallout(const std::string& source,
                                                               std::vector<vital::ModulationConnection*> connections) {  
  ModulationButton* modulation_button = modulation_buttons_[source];
  ExpandModulationButton* expand_button = modulation_callout_buttons_[source].get();
  expand_button->setBounds(getLocalArea(modulation_button, modulation_button->getModulationAreaBounds()));
  expand_button->setVisible(showingInParents(modulation_button));

  std::vector<ModulationAmountKnob*> amount_controls;
  for (vital::ModulationConnection* connection : connections) {
    int index = connection->modulation_processor->index();
    amount_controls.push_back(modulation_amount_sliders_[index].get());
    ModulationAmountKnob* slider = modulation_amount_sliders_[index].get();

    std::string name = connection->destination_name;
    if (slider_model_lookup_.count(name))
      slider->setDestinationComponent(slider_model_lookup_[name], name);
    else
      slider->setDestinationComponent(nullptr, name);
    
    slider->setVisible(false);
  }

  expand_button->setSliders(amount_controls);
  if (expand_button == current_expanded_modulation_)
    showModulationAmountCallout(source);
}

void ModulationManager::showModulationAmountCallout(const std::string& source) {
  static constexpr int kSliderWidth = 30;
  static constexpr int kPadding = 5;

  ModulationButton* modulation_button = modulation_buttons_[source];
  current_expanded_modulation_ = modulation_callout_buttons_[source].get();
  std::vector<ModulationAmountKnob*> amount_controls = current_expanded_modulation_->getSliders();

  int num_sliders = static_cast<int>(amount_controls.size());
  int columns = current_expanded_modulation_->getNumColumns(num_sliders);
  int rows = (num_sliders + columns - 1) / columns;
  int width = kSliderWidth * columns + 2 * kPadding;
  int height = kSliderWidth * rows + 2 * kPadding;
  Rectangle<int> top_level_modulation_bounds = getLocalArea(modulation_button, modulation_button->getLocalBounds());
  int start_x = top_level_modulation_bounds.getX() + (modulation_button->getWidth() - width) / 2;
  start_x = std::min(getWidth() - width, std::max(0, start_x));
  int start_y = top_level_modulation_bounds.getBottom();
  start_y = std::min(getHeight() - height, start_y);

  modulation_expansion_box_.setVisible(true);
  modulation_expansion_box_.setAmountControls(amount_controls);
  modulation_expansion_box_.setBounds(start_x, start_y, width, height);
  modulation_expansion_box_.setRounding(findValue(Skin::kBodyRounding));
  modulation_expansion_box_.grabKeyboardFocus();

  int row = 0;
  int column = 0;
  for (ModulationAmountKnob* slider : amount_controls) {
    int x = column * kSliderWidth + kPadding;
    int y = height - (row + 1) * kSliderWidth - kPadding;
    slider->setBounds(start_x + x, start_y + y, kSliderWidth, kSliderWidth);
    slider->setVisible(true);
    slider->setMouseClickGrabsKeyboardFocus(false);
    slider->redoImage();
    slider->getQuadComponent()->setAlwaysOnTop(true);

    column++;
    if (column >= columns) {
      column = 0;
      row++;
    }
  }
}

void ModulationManager::hideModulationAmountCallout() {
  if (current_expanded_modulation_ == nullptr)
    return;

  std::vector<ModulationAmountKnob*> amount_controls = current_expanded_modulation_->getSliders();
  for (ModulationAmountKnob* slider : amount_controls) {
    slider->setVisible(false);
    slider->getQuadComponent()->setAlwaysOnTop(false);
  }

  modulation_expansion_box_.setVisible(false);
  current_expanded_modulation_ = nullptr;
}

void ModulationManager::positionModulationAmountSliders(const std::string& source) {
  static constexpr int kMaxModulationsAcross = 3;
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  ModulationButton* modulation_button = modulation_buttons_[source];
  Rectangle<int> modulation_area = modulation_button->getModulationAreaBounds();
  int area_width = std::max(1, modulation_area.getWidth());
  int max_modulation_height = (kMaxModulationsAcross * modulation_area.getHeight()) / area_width;
  int max_modulations_inside = kMaxModulationsAcross * max_modulation_height;

  std::vector<vital::ModulationConnection*> connections = parent->getSynth()->getSourceConnections(source);
  int total_connections = static_cast<int>(connections.size());
  if (total_connections) {
    if (total_connections && total_connections > max_modulations_inside)
      positionModulationAmountSlidersCallout(source, connections);
    else
      positionModulationAmountSlidersInside(source, connections);
  }
  else
    modulation_callout_buttons_[source]->setVisible(false);
}

void ModulationManager::positionModulationAmountSliders() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  for (auto& modulation_slider : modulation_amount_sliders_)
    modulation_slider->setVisible(false);

  for (auto& modulation_button : modulation_buttons_) {
    std::string name = modulation_button.second->getName().toStdString();
    positionModulationAmountSliders(name);
  }
}

bool ModulationManager::enteringHoverValue() {
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    if (modulation_amount_sliders_[i] && modulation_amount_sliders_[i]->enteringValue())
      return true;
    if (modulation_hover_sliders_[i] && modulation_hover_sliders_[i]->enteringValue())
      return true;
    if (selected_modulation_sliders_[i] && selected_modulation_sliders_[i]->enteringValue())
      return true;
  }
  return false;
}

void ModulationManager::setModulationAmounts() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr || modifying_)
    return;

  vital::ModulationConnectionBank& bank = parent->getSynth()->getModulationBank();
  for (int i = 0; i < vital::kMaxModulationConnections; ++i) {
    vital::ModulationConnection* connection = bank.atIndex(i);
    if (aux_connections_to_from_.count(i) == 0)
      setModulationSliderValues(i, connection->modulation_processor->currentBaseValue());

    bool bipolar = connection->modulation_processor->isBipolar();
    bool stereo = connection->modulation_processor->isStereo();
    bool bypass = connection->modulation_processor->isBypassed();
    modulation_amount_sliders_[i]->setBipolar(bipolar);
    modulation_amount_sliders_[i]->setStereo(stereo);
    modulation_amount_sliders_[i]->setBypass(bypass);

    modulation_hover_sliders_[i]->setBipolar(bipolar);
    modulation_hover_sliders_[i]->setStereo(stereo);
    modulation_hover_sliders_[i]->setBypass(bypass);
  }
}

void ModulationManager::setVisibleMeterBounds() {
  for (auto& meter : meter_lookup_) {
    SynthSlider* slider = slider_model_lookup_[meter.first];
    if (slider && slider->isShowing()) {
      Rectangle<int> local_bounds = getLocalArea(slider, slider->getModulationMeterBounds());
      meter.second->setBounds(local_bounds);
    }
  }
}
