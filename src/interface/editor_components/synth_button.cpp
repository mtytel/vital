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

#include "synth_button.h"

#include "default_look_and_feel.h"
#include "full_interface.h"
#include "synth_strings.h"
#include "synth_gui_interface.h"
#include "synth_section.h"
#include "text_look_and_feel.h"

void OpenGlShapeButtonComponent::render(OpenGlWrapper& open_gl, bool animate) {
  incrementHover();

  Colour active_color;
  Colour hover_color;
  if (button_->getToggleState() && use_on_colors_) {
    if (down_)
      active_color = on_down_color_;
    else
      active_color = on_normal_color_;

    hover_color = on_hover_color_;
  }
  else {
    if (down_)
      active_color = off_down_color_;
    else
      active_color = off_normal_color_;

    hover_color = off_hover_color_;
  }

  if (!down_)
    active_color = active_color.interpolatedWith(hover_color, hover_amount_);

  shape_.setColor(active_color);
  shape_.render(open_gl, animate);
};

void OpenGlShapeButtonComponent::incrementHover() {
  if (hover_)
    hover_amount_ = std::min(1.0f, hover_amount_ + kHoverInc);
  else
    hover_amount_ = std::max(0.0f, hover_amount_ - kHoverInc);
}

void OpenGlButtonComponent::setColors() {
  if (button_->findParentComponentOfClass<SynthGuiInterface>() == nullptr)
    return;

  body_color_ = button_->findColour(Skin::kBody, true);
  if (style_ == kTextButton || style_ == kJustText) {
    on_color_ = button_->findColour(Skin::kIconButtonOn, true);
    on_pressed_color_ = button_->findColour(Skin::kIconButtonOnPressed, true);
    on_hover_color_ = button_->findColour(Skin::kIconButtonOnHover, true);
    off_color_ = button_->findColour(Skin::kIconButtonOff, true);
    off_pressed_color_ = button_->findColour(Skin::kIconButtonOffPressed, true);
    off_hover_color_ = button_->findColour(Skin::kIconButtonOffHover, true);
    background_color_ = button_->findColour(Skin::kTextComponentBackground, true);
  }
  else if (style_ == kPowerButton) {
    on_color_ = button_->findColour(Skin::kPowerButtonOn, true);
    on_pressed_color_ = button_->findColour(Skin::kOverlayScreen, true);
    on_hover_color_ = button_->findColour(Skin::kLightenScreen, true);
    off_color_ = button_->findColour(Skin::kPowerButtonOff, true);
    off_pressed_color_ = on_pressed_color_;
    off_hover_color_ = on_hover_color_;
    background_color_ = on_color_;
  }
  else if (style_ == kUiButton) {
    if (primary_ui_button_) {
      on_color_ = button_->findColour(Skin::kUiActionButton, true);
      on_pressed_color_ = button_->findColour(Skin::kUiActionButtonPressed, true);
      on_hover_color_ = button_->findColour(Skin::kUiActionButtonHover, true);
    }
    else {
      on_color_ = button_->findColour(Skin::kUiButton, true);
      on_pressed_color_ = button_->findColour(Skin::kUiButtonPressed, true);
      on_hover_color_ = button_->findColour(Skin::kUiButtonHover, true);
    }
    background_color_ = button_->findColour(Skin::kUiButtonText, true);
  }
  else if (style_ == kLightenButton) {
    on_color_ = Colours::transparentWhite;
    on_pressed_color_ = button_->findColour(Skin::kOverlayScreen, true);
    on_hover_color_ = button_->findColour(Skin::kLightenScreen, true);
    off_color_ = on_color_;
    off_pressed_color_ = on_pressed_color_;
    off_hover_color_ = on_hover_color_;
    background_color_ = on_color_;
  }
}

void OpenGlButtonComponent::renderTextButton(OpenGlWrapper& open_gl, bool animate) {
  incrementHover();

  Colour active_color;
  Colour hover_color;
  if (button_->getToggleState() && show_on_colors_) {
    if (down_)
      active_color = on_pressed_color_;
    else
      active_color = on_color_;

    hover_color = on_hover_color_;
  }
  else {
    if (down_)
      active_color = off_pressed_color_;
    else
      active_color = off_color_;

    hover_color = off_hover_color_;
  }

  if (!down_)
    active_color = active_color.interpolatedWith(hover_color, hover_amount_);

  background_.setRounding(findValue(Skin::kLabelBackgroundRounding));
  if (!text_.isActive()) {
    background_.setColor(active_color);
    background_.render(open_gl, animate);
    return;
  }

  if (style_ != kJustText) {
    background_.setColor(background_color_);
    background_.render(open_gl, animate);
  }
  text_.setColor(active_color);
  text_.render(open_gl, animate);
}

void OpenGlButtonComponent::renderPowerButton(OpenGlWrapper& open_gl, bool animate) {
  static constexpr float kPowerRadius = 0.45f;
  static constexpr float kPowerHoverRadius = 0.65f;

  if (button_->getToggleState())
    background_.setColor(on_color_);
  else
    background_.setColor(off_color_);

  background_.setQuad(0, -kPowerRadius, -kPowerRadius, 2.0f * kPowerRadius, 2.0f * kPowerRadius);
  background_.render(open_gl, animate);

  incrementHover();

  background_.setQuad(0, -kPowerHoverRadius, -kPowerHoverRadius, 2.0f * kPowerHoverRadius, 2.0f * kPowerHoverRadius);
  if (down_) {
    background_.setColor(on_pressed_color_);
    background_.render(open_gl, animate);
  }
  else if (hover_amount_) {
    background_.setColor(on_hover_color_.withMultipliedAlpha(hover_amount_));
    background_.render(open_gl, animate);
  }
}

void OpenGlButtonComponent::renderUiButton(OpenGlWrapper& open_gl, bool animate) {
  bool enabled = button_->isEnabled();
  incrementHover();

  Colour active_color;
  if (down_)
    active_color = on_pressed_color_;
  else
    active_color = on_color_;

  if (!down_ && enabled)
    active_color = active_color.interpolatedWith(on_hover_color_, hover_amount_);

  background_.setRounding(findValue(Skin::kLabelBackgroundRounding));
  background_.setColor(active_color);
  background_.render(open_gl, animate);
  
  text_.setColor(background_color_);
  if (!enabled) {
    text_.setColor(on_color_);

    float border_x = 4.0f / button_->getWidth();
    float border_y = 4.0f / button_->getHeight();
    background_.setQuad(0, -1.0f + border_x, -1.0f + border_y, 2.0f - 2.0f * border_x, 2.0f - 2.0f * border_y);
    background_.setColor(body_color_);
    background_.render(open_gl, animate);
    
    background_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);
  }

  text_.render(open_gl, animate);
}

void OpenGlButtonComponent::renderLightenButton(OpenGlWrapper& open_gl, bool animate) {
  bool enabled = button_->isEnabled();
  incrementHover();

  Colour active_color;
  if (down_)
    active_color = on_pressed_color_;
  else
    active_color = on_color_;

  if (!down_ && enabled)
    active_color = active_color.interpolatedWith(on_hover_color_, hover_amount_);

  background_.setRounding(findValue(Skin::kLabelBackgroundRounding));
  background_.setColor(active_color);
  background_.render(open_gl, animate);
}

void OpenGlButtonComponent::incrementHover() {
  if (hover_)
    hover_amount_ = std::min(1.0f, hover_amount_ + kHoverInc);
  else
    hover_amount_ = std::max(0.0f, hover_amount_ - kHoverInc);
}

void OpenGlToggleButton::resized() {
  static constexpr float kUiButtonSizeMult = 0.45f;
 
  ToggleButton::resized();
  SynthSection* section = findParentComponentOfClass<SynthSection>();
  getGlComponent()->setText();
  getGlComponent()->background().markDirty();
  if (section) {
    if (getGlComponent()->style() == OpenGlButtonComponent::kUiButton) {
      getGlComponent()->text().setFontType(PlainTextComponent::kLight);
      getGlComponent()->text().setTextSize(kUiButtonSizeMult * getHeight());
    }
    else
      getGlComponent()->text().setTextSize(section->findValue(Skin::kButtonFontSize));
    button_component_.setColors();
  }
}

SynthButton::SynthButton(String name) : OpenGlToggleButton(name), string_lookup_(nullptr) {
  if (!vital::Parameters::isParameter(name.toStdString()))
    return;
}

void SynthButton::clicked(const ModifierKeys& modifiers) {
  OpenGlToggleButton::clicked(modifiers);

  if (!modifiers.isPopupMenu()) {
    for (SynthButton::ButtonListener* listener : button_listeners_)
      listener->guiChanged(this);
  }
}

String SynthButton::getTextFromValue(bool on) {
  int lookup = on ? 1 : 0;

  if (string_lookup_)
    return string_lookup_[lookup];

  return strings::kOffOnNames[lookup];
}

void SynthButton::handlePopupResult(int result) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();

  if (result == kArmMidiLearn)
    synth->armMidiLearn(getName().toStdString());
  else if (result == kClearMidiLearn)
    synth->clearMidiLearn(getName().toStdString());
}

void SynthButton::mouseDown(const MouseEvent& e) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();

  if (e.mods.isPopupMenu()) {
    OpenGlToggleButton::mouseExit(e);

    PopupItems options;
    options.addItem(kArmMidiLearn, "Learn MIDI Assignment");
    if (parent->getSynth()->isMidiMapped(getName().toStdString()))
      options.addItem(kClearMidiLearn, "Clear MIDI Assignment");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    parent->showPopupSelector(this, e.getPosition(), options, [=](int selection) { handlePopupResult(selection); });
  }
  else {
    OpenGlToggleButton::mouseDown(e);
    synth->beginChangeGesture(getName().toStdString());
  }
}

void SynthButton::mouseUp(const MouseEvent& e) {
  if (!e.mods.isPopupMenu()) {
    OpenGlToggleButton::mouseUp(e);

    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    if (parent)
      parent->getSynth()->endChangeGesture(getName().toStdString());
  }
}

void SynthButton::addButtonListener(SynthButton::ButtonListener* listener) {
  button_listeners_.push_back(listener);
}
