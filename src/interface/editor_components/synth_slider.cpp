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

#include "synth_slider.h"

#include <cmath>

#include "skin.h"
#include "fonts.h"
#include "curve_look_and_feel.h"
#include "full_interface.h"
#include "modulation_matrix.h"
#include "synth_gui_interface.h"
#include "synth_section.h"

void OpenGlSliderQuad::init(OpenGlWrapper& open_gl) {
  if (slider_->isModulationKnob())
    setFragmentShader(Shaders::kModulationKnobFragment);
  else if (slider_->isRotaryQuad())
    setFragmentShader(Shaders::kRotarySliderFragment);
  else if (slider_->isHorizontalQuad())
    setFragmentShader(Shaders::kHorizontalSliderFragment);
  else
    setFragmentShader(Shaders::kVerticalSliderFragment);

  OpenGlQuad::init(open_gl);
}

void OpenGlSliderQuad::paintBackground(Graphics& g) {
  slider_->redoImage(false);
}

int OpenGlSlider::getLinearSliderWidth() {
  int total_width = isHorizontal() ? getHeight() : getWidth();
  int extra = total_width % 2;
  return std::floor(SynthSlider::kLinearWidthPercent * total_width * 0.5f) * 2.0f + extra;
}

void OpenGlSlider::setSliderDisplayValues() {
  if (isModulationKnob()) {
    float radius = 1.0f - 1.0f / getWidth();
    slider_quad_.setQuad(0, -radius, -radius, 2.0f * radius, 2.0f * radius);
  }
  else if (isRotaryQuad()) {
    float thickness = findValue(Skin::kKnobArcThickness);
    float size = findValue(Skin::kKnobArcSize) * getKnobSizeScale() + thickness;
    float offset = findValue(Skin::kKnobOffset);
    float radius_x = (size + 0.5f) / getWidth();
    float center_y = 2.0f * offset / getHeight();
    float radius_y = (size + 0.5f) / getHeight();
    slider_quad_.setQuad(0, -radius_x, -center_y - radius_y, 2.0f * radius_x, 2.0f * radius_y);
    slider_quad_.setThumbAmount(findValue(Skin::kKnobHandleLength));
  }
  else if (isHorizontalQuad()) {
    float margin = 2.0f * (findValue(Skin::kWidgetMargin) - 0.5f) / getWidth();
    slider_quad_.setQuad(0, -1.0f + margin, -1.0f, 2.0f - 2.0f * margin, 2.0f);
  }
  else if (isVerticalQuad()) {
    float margin = 2.0f * (findValue(Skin::kWidgetMargin) - 0.5f) / getHeight();
    slider_quad_.setQuad(0, -1.0f, -1.0f + margin, 2.0f, 2.0f - 2.0f * margin);
  }
}

void OpenGlSlider::redoImage(bool skip_image) {
  static constexpr float kRoundingMult = 0.4f;
  static constexpr float kRotaryHoverBoost = 1.4f;
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  bool horizontal = isHorizontalQuad();
  bool vertical = isVerticalQuad();
  if (modulation_amount_) {
    slider_quad_.setModColor(mod_color_);
    slider_quad_.setBackgroundColor(background_color_);
  }
  else {
    slider_quad_.setModColor(Colours::transparentBlack);
    slider_quad_.setBackgroundColor(Colours::transparentBlack);
  }

  if (isModulationKnob()) {
    slider_quad_.setActive(true);
    float t = getValue();
    slider_quad_.setThumbColor(thumb_color_);

    if (t > 0.0f) {
      slider_quad_.setShaderValue(0, vital::utils::interpolate(vital::kPi, -vital::kPi, t));
      slider_quad_.setColor(unselected_color_);
      slider_quad_.setAltColor(selected_color_);
    }
    else {
      slider_quad_.setShaderValue(0, vital::utils::interpolate(-vital::kPi, vital::kPi, -t));
      slider_quad_.setColor(selected_color_);
      slider_quad_.setAltColor(unselected_color_);
    }

    if (isMouseOverOrDragging())
      slider_quad_.setThickness(1.8f);
    else
      slider_quad_.setThickness(1.0f);
  }
  else if (isRotaryQuad()) {
    slider_quad_.setActive(true);
    float arc = slider_quad_.getMaxArc();
    float t = valueToProportionOfLength(getValue());
    slider_quad_.setShaderValue(0, vital::utils::interpolate(-arc, arc, t));
    slider_quad_.setColor(selected_color_);
    slider_quad_.setAltColor(unselected_color_);
    slider_quad_.setThumbColor(thumb_color_);
    slider_quad_.setStartPos(bipolar_ ? 0.0f : -vital::kPi);

    float thickness = findValue(Skin::kKnobArcThickness);
    if (isMouseOverOrDragging())
      thickness *= kRotaryHoverBoost;
    slider_quad_.setThickness(thickness);
  }
  else if (horizontal || vertical) {
    slider_quad_.setActive(true);
    float t = valueToProportionOfLength(getValue());
    slider_quad_.setShaderValue(0, t);
    slider_quad_.setColor(selected_color_);
    slider_quad_.setAltColor(unselected_color_);
    slider_quad_.setThumbColor(thumb_color_);
    slider_quad_.setStartPos(bipolar_ ? 0.0f : -1.0f);

    int total_width = horizontal ? getHeight() : getWidth();
    float slider_width = getLinearSliderWidth();
    float handle_width = SynthSlider::kLinearHandlePercent * total_width;
    if (isMouseOverOrDragging()) {
      int boost = std::round(slider_width / 8.0f) + 1.0f;
      slider_quad_.setThickness(slider_width + 2 * boost);
    }
    else
      slider_quad_.setThickness(slider_width);
    slider_quad_.setRounding(slider_width * kRoundingMult);
    slider_quad_.setThumbAmount(handle_width);
  }
  else if (!skip_image) {
    image_component_.setActive(true);
    image_component_.redrawImage(true);
  }
}

void OpenGlSlider::setColors() {
  if (getWidth() <= 0)
    return;

  thumb_color_ = getThumbColor();
  selected_color_ = getSelectedColor();
  unselected_color_ = getUnselectedColor();
  background_color_ = getBackgroundColor();
  mod_color_ = getModColor();
}

SynthSlider::SynthSlider(String name) : OpenGlSlider(name), show_popup_on_hover_(false), scroll_enabled_(true),
                                        bipolar_modulation_(false), stereo_modulation_(false),
                                        bypass_modulation_(false), modulation_bar_right_(true),
                                        snap_to_value_(false), hovering_(false),
                                        has_parameter_assignment_(false),
                                        use_suffix_(true), snap_value_(0.0),
                                        text_height_percentage_(0.0f), knob_size_scale_(1.0f),
                                        sensitivity_(kDefaultSensitivity),
                                        popup_placement_(BubbleComponent::below),
                                        modulation_control_placement_(BubbleComponent::below),
                                        max_display_characters_(kDefaultFormatLength),
                                        max_decimal_places_(kDefaultFormatDecimalPlaces), shift_index_amount_(0),
                                        shift_is_multiplicative_(false), mouse_wheel_index_movement_(1.0),
                                        text_entry_width_percent_(kDefaultTextEntryWidthPercent),
                                        text_entry_height_percent_(kDefaultTextEntryHeightPercent),
                                        display_multiply_(0.0f), display_exponential_base_(2.0f),
                                        string_lookup_(nullptr), extra_modulation_target_(nullptr),
                                        synth_interface_(nullptr) {
  text_entry_ = std::make_unique<OpenGlTextEditor>("text_entry");
  text_entry_->setMonospace();
  text_entry_->setMultiLine(false);
  text_entry_->setScrollToShowCursor(false);
  text_entry_->addListener(this);
  text_entry_->setSelectAllWhenFocused(true);
  text_entry_->setKeyboardType(TextEditor::numericKeyboard);
  text_entry_->setJustification(Justification::centred);
  text_entry_->setAlwaysOnTop(true);
  text_entry_->getImageComponent()->setAlwaysOnTop(true);
  addChildComponent(text_entry_.get());

  setWantsKeyboardFocus(true);
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);

  has_parameter_assignment_ = vital::Parameters::isParameter(name.toStdString());
  if (!has_parameter_assignment_)
    return;

  setRotaryParameters(2.0f * vital::kPi - kRotaryAngle, 2.0f * vital::kPi + kRotaryAngle, true);

  details_ = vital::Parameters::getDetails(name.toStdString());
  setStringLookup(details_.string_lookup);

  VITAL_ASSERT(details_.value_scale != vital::ValueDetails::kIndexed || details_.max - details_.min >= 1.0f);
  setDefaultRange();
  setDoubleClickReturnValue(true, details_.default_value);
  setVelocityBasedMode(false);
  setVelocityModeParameters(1.0, 0, 0.0, false, ModifierKeys::ctrlAltCommandModifiers);
}

PopupItems SynthSlider::createPopupMenu() {
  PopupItems options;

  if (isDoubleClickReturnEnabled())
    options.addItem(kDefaultValue, "Set to Default Value");

  if (has_parameter_assignment_)
    options.addItem(kArmMidiLearn, "Learn MIDI Assignment");

  if (has_parameter_assignment_ && synth_interface_->getSynth()->isMidiMapped(getName().toStdString()))
    options.addItem(kClearMidiLearn, "Clear MIDI Assignment");

  options.addItem(kManualEntry, "Enter Value");

  std::vector<vital::ModulationConnection*> connections = getConnections();
  if (!connections.empty())
    options.addItem(-1, "");

  std::string disconnect = "Disconnect from ";
  for (int i = 0; i < connections.size(); ++i) {
    std::string name = ModulationMatrix::getMenuSourceDisplayName(connections[i]->source_name).toStdString();
    options.addItem(kModulationList + i, disconnect + name);
  }

  if (connections.size() > 1)
    options.addItem(kClearModulations, "Disconnect all modulations");

  return options;
}

void SynthSlider::mouseDown(const MouseEvent& e) {
  SynthBase* synth = synth_interface_->getSynth();

  if (e.mods.isAltDown()) {
    showTextEntry();
    return;
  }

  if (e.mods.isPopupMenu()) {
    PopupItems options = createPopupMenu();
    parent_->showPopupSelector(this, e.getPosition(), options, [=](int selection) { handlePopupResult(selection); });
  }
  else {
    if (isRotary())
      setMouseDragSensitivity(kDefaultRotaryDragLength / sensitivity_);
    else {
      setSliderSnapsToMousePosition(false);
      setMouseDragSensitivity(std::max(getWidth(), getHeight()) / sensitivity_);
    }

    OpenGlSlider::mouseDown(e);
    synth->beginChangeGesture(getName().toStdString());

    for (SliderListener* listener : slider_listeners_)
      listener->mouseDown(this);

    showPopup(true);
  }
}

void SynthSlider::mouseDrag(const MouseEvent& e) {
  if (e.mods.isAltDown())
    return;
  
  float multiply = 1.0f;
  if (e.mods.isShiftDown() && shift_index_amount_) {
    double value = getValue();
    int value_from_min = value - details_.min;
    int shift = value_from_min % shift_index_amount_;
    int min = details_.min + shift;
    double max = details_.max;
    if (shift)
      max = std::max<double>(details_.max + shift - shift_index_amount_, value);
    if (value < min || value > max)
      setValue(vital::utils::clamp(value, min, max));
    setRange(min, max, shift_index_amount_);
    multiply = std::max(1, shift_index_amount_ / 2);
  }
  else
    setDefaultRange();
    
  sensitive_mode_ = e.mods.isCommandDown();
  if (sensitive_mode_)
    multiply *= kSlowDragMultiplier;

  if (isRotary())
    setMouseDragSensitivity(kDefaultRotaryDragLength / (sensitivity_ * multiply));
  else {
    setSliderSnapsToMousePosition(false);
    setMouseDragSensitivity(std::max(getWidth(), getHeight()) / (sensitivity_ * multiply));
  }

  OpenGlSlider::mouseDrag(e);

  if (!e.mods.isPopupMenu())
    showPopup(true);
}

void SynthSlider::mouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu() || e.mods.isAltDown())
    return;

  setDefaultRange();
  OpenGlSlider::mouseUp(e);

  for (SliderListener* listener : slider_listeners_)
    listener->mouseUp(this);

  synth_interface_->getSynth()->endChangeGesture(getName().toStdString());
}

void SynthSlider::mouseEnter(const MouseEvent &e) {
  OpenGlSlider::mouseEnter(e);
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->hoverStarted(this);

  if (show_popup_on_hover_)
    showPopup(true);

  hovering_ = true;
  redoImage();
}

void SynthSlider::mouseExit(const MouseEvent &e) {
  OpenGlSlider::mouseExit(e);
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->hoverEnded(this);

  hidePopup(true);
  hovering_ = false;
  redoImage();
}

void SynthSlider::mouseDoubleClick(const MouseEvent& e) {
  OpenGlSlider::mouseDoubleClick(e);
  if (!e.mods.isPopupMenu()) {
    for (SliderListener* listener : slider_listeners_)
      listener->doubleClick(this);
  }
  showPopup(true);
}

void SynthSlider::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  double interval = getInterval();
  if (scroll_enabled_ && !wheel.isSmooth && getInterval() > 0) {
    if (shift_index_amount_ && e.mods.isShiftDown()) {
      interval = shift_index_amount_;
      if (shift_is_multiplicative_) {
        if (wheel.deltaY > 0.0f)
          setValue(getValue() * interval * mouse_wheel_index_movement_);
        else
          setValue(getValue() / std::max(1.0, (interval * mouse_wheel_index_movement_)));
      }
    }

    if (wheel.deltaY > 0.0f)
      setValue(getValue() + interval * mouse_wheel_index_movement_);
    else
      setValue(getValue() - interval * mouse_wheel_index_movement_);
  }
  else
    OpenGlSlider::mouseWheelMove(e, wheel);

  showPopup(true);
}

void SynthSlider::focusLost(FocusChangeType cause) {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->focusLost(this);
}

void SynthSlider::valueChanged() {
  OpenGlSlider::valueChanged();
  notifyGuis();
}

String SynthSlider::getRawTextFromValue(double value) {
  if (!has_parameter_assignment_)
    return OpenGlSlider::getTextFromValue(value);

  return String(getAdjustedValue(value));
}

String SynthSlider::getSliderTextFromValue(double value) {
  if (string_lookup_) {
    int lookup = vital::utils::iclamp(value, 0, getMaximum());
    return string_lookup_[lookup];
  }

  if (!has_parameter_assignment_)
    return OpenGlSlider::getTextFromValue(value);

  double adjusted_value = getAdjustedValue(value);
  return popup_prefix_ + formatValue(adjusted_value);
}

String SynthSlider::getTextFromValue(double value) {
  if (isText() && has_parameter_assignment_ && popup_prefix_.isEmpty()) {
    if (details_.local_description.empty())
      return details_.display_name;
    return details_.local_description;
  }
  if (isText() && !popup_prefix_.isEmpty())
    return popup_prefix_;
  return getSliderTextFromValue(value);
}

double SynthSlider::getValueFromText(const String& text) {
  String cleaned = text.removeCharacters(" ").toLowerCase();
  if (string_lookup_) {
    for (int i = 0; i <= getMaximum(); ++i) {
      if (cleaned == String(string_lookup_[i]).toLowerCase())
        return i;
    }
  }
  if (text.endsWithChar('%') && getDisplayDetails()->display_units != "%") {
    float t = 0.01f * cleaned.removeCharacters("%").getDoubleValue();
    return (getMaximum() - getMinimum()) * t + getMinimum();
  }
  return getValueFromAdjusted(Slider::getValueFromText(text));
}

double SynthSlider::getAdjustedValue(double value) {
  vital::ValueDetails* details = getDisplayDetails();

  double adjusted_value = value;
  switch (details->value_scale) {
    case vital::ValueDetails::kQuadratic:
      adjusted_value *= adjusted_value;
      break;
    case vital::ValueDetails::kCubic:
      adjusted_value *= adjusted_value * adjusted_value;
      break;
    case vital::ValueDetails::kQuartic:
      adjusted_value *= adjusted_value;
      adjusted_value *= adjusted_value;
      break;
    case vital::ValueDetails::kExponential:
      adjusted_value = powf(display_exponential_base_, adjusted_value);
      break;
    case vital::ValueDetails::kSquareRoot:
      adjusted_value = sqrtf(std::max(adjusted_value, 0.0));
      break;
    default:
      break;
  }

  adjusted_value += details->post_offset;
  if (details->display_invert)
    adjusted_value = 1.0 / adjusted_value;
  if (display_multiply_)
    adjusted_value *= display_multiply_;
  else
    adjusted_value *= details->display_multiply;

  return adjusted_value;
}

double SynthSlider::getValueFromAdjusted(double value) {
  vital::ValueDetails* details = getDisplayDetails();

  double readjusted_value = value;
  if (display_multiply_)
    readjusted_value /= display_multiply_;
  else
    readjusted_value /= details->display_multiply;

  if (details->display_invert)
    readjusted_value = 1.0 / readjusted_value;
  readjusted_value -= details->post_offset;

  switch (details->value_scale) {
    case vital::ValueDetails::kQuadratic:
      return sqrtf(std::max(readjusted_value, 0.0));
    case vital::ValueDetails::kCubic:
      return powf(std::max(readjusted_value, 0.0), 1.0f / 3.0f);
    case vital::ValueDetails::kQuartic:
      return sqrtf(sqrtf(std::max(readjusted_value, 0.0)));
    case vital::ValueDetails::kExponential:
      return log(readjusted_value) / std::log(display_exponential_base_);
    case vital::ValueDetails::kSquareRoot:
      return readjusted_value * readjusted_value;
    default:
      return readjusted_value;
  }
}

void SynthSlider::setValueFromAdjusted(double value) {
  setValue(getValueFromAdjusted(value));
}

void SynthSlider::parentHierarchyChanged() {
  synth_interface_ = findParentComponentOfClass<SynthGuiInterface>();
  OpenGlSlider::parentHierarchyChanged();
}

double SynthSlider::snapValue(double attempted_value, DragMode drag_mode) {
  const double percent = 0.05;
  if (!snap_to_value_ || sensitive_mode_ || drag_mode != DragMode::absoluteDrag)
    return attempted_value;

  double range = getMaximum() - getMinimum();
  double radius = percent * range;
  if (attempted_value - snap_value_ <= radius && attempted_value - snap_value_ >= -radius)
    return snap_value_;
  return attempted_value;
}

void SynthSlider::textEditorReturnKeyPressed(TextEditor& editor) {
  setSliderPositionFromText();
}

void SynthSlider::textEditorFocusLost(TextEditor& editor) {
  setSliderPositionFromText();
}

void SynthSlider::setSliderPositionFromText() {
  if (text_entry_ && !text_entry_->getText().isEmpty())
    setValue(getValueFromText(text_entry_->getText()));
  text_entry_->setVisible(false);

  for (SliderListener* listener : slider_listeners_)
    listener->menuFinished(this);
}

void SynthSlider::showTextEntry() {
#if !defined(NO_TEXT_ENTRY)
  text_entry_->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
  text_entry_->setColour(TextEditor::textColourId, findColour(Skin::kBodyText, true));
  text_entry_->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
  text_entry_->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
  if (isRotary())
    setRotaryTextEntryBounds();
  else
    setLinearTextEntryBounds();
  text_entry_->setVisible(true);

  text_entry_->redoImage();
  text_entry_->setText(getRawTextFromValue(getValue()));
  text_entry_->selectAll();
  if (text_entry_->isShowing())
    text_entry_->grabKeyboardFocus();
#endif
}

void SynthSlider::drawShadow(Graphics &g) {
  if (isRotary() && !isTextOrCurve())
    drawRotaryShadow(g);
  else if (&getLookAndFeel() == CurveLookAndFeel::instance()) {
    g.setColour(findColour(Skin::kWidgetBackground, true));
    float rounding = findValue(Skin::kWidgetRoundedCorner);
    g.fillRoundedRectangle(getBounds().toFloat(), rounding);
  }
}

void SynthSlider::drawRotaryShadow(Graphics &g) {
  Colour shadow_color = findColour(Skin::kShadow, true);

  float center_x = getWidth() / 2.0f;
  float center_y = getHeight() / 2.0f;
  float stroke_width = findValue(Skin::kKnobArcThickness);
  float radius = knob_size_scale_ * findValue(Skin::kKnobArcSize) / 2.0f;
  center_y += findValue(Skin::kKnobOffset);
  float shadow_width = findValue(Skin::kKnobShadowWidth);
  float shadow_offset = findValue(Skin::kKnobShadowOffset);

  PathStrokeType outer_stroke(stroke_width, PathStrokeType::beveled, PathStrokeType::rounded);
  PathStrokeType shadow_stroke(stroke_width + 1, PathStrokeType::beveled, PathStrokeType::rounded);

  g.saveState();
  g.setOrigin(getX(), getY());

  Colour body = findColour(Skin::kRotaryBody, true);
  float body_radius = knob_size_scale_ * findValue(Skin::kKnobBodySize) / 2.0f;
  if (body_radius >= 0.0f && body_radius < getWidth()) {

    if (shadow_width > 0.0f) {
      Colour transparent_shadow = shadow_color.withAlpha(0.0f);
      float shadow_radius = body_radius + shadow_width;
      ColourGradient shadow_gradient(shadow_color, center_x, center_y + shadow_offset,
                                     transparent_shadow, center_x - shadow_radius, center_y + shadow_offset, true);
      float shadow_start = std::max(0.0f, (body_radius - std::abs(shadow_offset))) / shadow_radius;
      shadow_gradient.addColour(shadow_start, shadow_color);
      shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.75f, shadow_color.withMultipliedAlpha(0.5625f));
      shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.5f, shadow_color.withMultipliedAlpha(0.25f));
      shadow_gradient.addColour(1.0f - (1.0f - shadow_start) * 0.25f, shadow_color.withMultipliedAlpha(0.0625f));
      g.setGradientFill(shadow_gradient);
      g.fillRect(getLocalBounds());
    }

    g.setColour(body);
    Rectangle<float> ellipse(center_x - body_radius, center_y - body_radius, 2.0f * body_radius, 2.0f * body_radius);
    g.fillEllipse(ellipse);

    g.setColour(findColour(Skin::kRotaryBodyBorder, true));
    g.drawEllipse(ellipse.reduced(0.5f), 1.0f);
  }

  Path shadow_outline;
  Path shadow_path;

  shadow_outline.addCentredArc(center_x, center_y, radius, radius,
                               0, -kRotaryAngle, kRotaryAngle, true);
  shadow_stroke.createStrokedPath(shadow_path, shadow_outline);
  if ((!findColour(Skin::kRotaryArcUnselected, true).isTransparent() && isActive()) ||
      (!findColour(Skin::kRotaryArcUnselectedDisabled, true).isTransparent() && !isActive())) {
    g.setColour(shadow_color); 
    g.fillPath(shadow_path);
  }

  g.restoreState();
}

void SynthSlider::setDefaultRange() {
  if (!has_parameter_assignment_)
    return;

  if (details_.value_scale == vital::ValueDetails::kIndexed)
    setRange(details_.min, details_.max, 1.0f);
  else
    setRange(details_.min, details_.max);
}

void SynthSlider::addSliderListener(SynthSlider::SliderListener* listener) {
  slider_listeners_.push_back(listener);
}

void SynthSlider::showPopup(bool primary) {
  if (shouldShowPopup())
    parent_->showPopupDisplay(this, getTextFromValue(getValue()).toStdString(), popup_placement_, primary);
}

void SynthSlider::hidePopup(bool primary) {
  parent_->hidePopupDisplay(primary);
}

String SynthSlider::formatValue(float value) {
  String format;
  if (details_.value_scale == vital::ValueDetails::kIndexed)
    format = String(value);
  else {
    if (max_decimal_places_ == 0)
      format = String(std::roundf(value));
    else
      format = String(value, max_decimal_places_);

    int display_characters = max_display_characters_;
    if (format[0] == '-')
      display_characters += 1;

    format = format.substring(0, display_characters);
    if (format.getLastCharacter() == '.')
      format = format.removeCharacters(".");
  }
  
  if (use_suffix_)
    return format + getDisplayDetails()->display_units;
  return format;
}

void SynthSlider::notifyGuis() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->guiChanged(this);
}

Rectangle<int> SynthSlider::getModulationMeterBounds() const {
  static constexpr int kTextBarSize = 2;

  Rectangle<int> mod_bounds = getModulationArea();
  if (isTextOrCurve()) {
    if (modulation_bar_right_)
      return mod_bounds.removeFromRight(kTextBarSize);

    return mod_bounds.removeFromLeft(kTextBarSize);
  }
  else if (isRotary())
    return getLocalBounds();

  int buffer = findValue(Skin::kWidgetMargin);
  if (getSliderStyle() == LinearBar) {
    return Rectangle<int>(mod_bounds.getX() + buffer, mod_bounds.getY(), 
                          mod_bounds.getWidth() - 2 * buffer, mod_bounds.getHeight());
  }
  return Rectangle<int>(mod_bounds.getX(), mod_bounds.getY() + buffer,
                        mod_bounds.getWidth(), mod_bounds.getHeight() - 2 * buffer);
}

std::vector<vital::ModulationConnection*> SynthSlider::getConnections() {
  if (synth_interface_ == nullptr)
    return std::vector<vital::ModulationConnection*>();

  SynthBase* synth = synth_interface_->getSynth();
  return synth->getDestinationConnections(getName().toStdString());
}

void SynthSlider::handlePopupResult(int result) {
  SynthBase* synth = synth_interface_->getSynth();
  std::vector<vital::ModulationConnection*> connections = getConnections();

  if (result == kArmMidiLearn)
    synth->armMidiLearn(getName().toStdString());
  else if (result == kClearMidiLearn)
    synth->clearMidiLearn(getName().toStdString());
  else if (result == kDefaultValue)
    setValue(getDoubleClickReturnValue());
  else if (result == kManualEntry)
    showTextEntry();
  else if (result == kClearModulations) {
    for (vital::ModulationConnection* connection : connections) {
      std::string source = connection->source_name;
      synth_interface_->disconnectModulation(connection);
    }
    notifyModulationsChanged();
  }
  else if (result >= kModulationList) {
    int connection_index = result - kModulationList;
    std::string source = connections[connection_index]->source_name;
    synth_interface_->disconnectModulation(connections[connection_index]);
    notifyModulationsChanged();
  }
}

void SynthSlider::setRotaryTextEntryBounds() {
  int text_width = getWidth() * text_entry_width_percent_;
  float font_size = findValue(Skin::kTextComponentFontSize);
  int text_height = font_size / kTextEntryHeightPercent;
  float y_offset = 0.0f;
  if (isText())
    y_offset = findValue(Skin::kTextComponentOffset);

  text_entry_->setBounds((getWidth() - text_width) / 2, (getHeight() - text_height + 1) / 2 + y_offset,
                         text_width, text_height);
}

void SynthSlider::setLinearTextEntryBounds() {
  static constexpr float kTextEntryWidthRatio = 3.0f;

  float font_size = findValue(Skin::kTextComponentFontSize);
  int text_height = font_size / kTextEntryHeightPercent;
  int text_width = text_height * kTextEntryWidthRatio;

  text_entry_->setBounds((getWidth() - text_width) / 2, (getHeight() - text_height) / 2, text_width, text_height);
}

void SynthSlider::notifyModulationAmountChanged() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->modulationAmountChanged(this);
}

void SynthSlider::notifyModulationRemoved() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->modulationRemoved(this);
}

void SynthSlider::notifyModulationsChanged() {
  for (SynthSlider::SliderListener* listener : slider_listeners_)
    listener->modulationsChanged(getName().toStdString());
}

vital::ValueDetails* SynthSlider::getDisplayDetails() {
  if (alternate_display_setting_.first == 0 || parent_ == nullptr)
    return &details_;

  if (parent_->findValue(alternate_display_setting_.first) == alternate_display_setting_.second)
    return &alternate_details_;
  return &details_;
}
