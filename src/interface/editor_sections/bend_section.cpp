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

#include "bend_section.h"

#include "skin.h"
#include "fonts.h"
#include "synth_gui_interface.h"
#include "sound_engine.h"
#include "synth_slider.h"

ControlWheel::ControlWheel(String name) : SynthSlider(std::move(name)), parent_(nullptr) {
  paintToImage(true);
  setValue(0.0f);
  setSliderStyle(LinearBarVertical);
  setSensitivity(0.5f);
}

void ControlWheel::paintLine(Graphics& g, float y_percent, Colour line_color, Colour fill_color) {
  static constexpr float kLineWidthRatio = 0.165f;
  static constexpr float kFadeRatio = 0.12f;

  float buffer = getWidth() * kBufferWidthRatio;
  float width = getWidth() - 2.0f * buffer;
  float height = getHeight() - 4.0f * buffer;
  float end_radians = vital::kPi / 2.0f;
  float radians = 2.0f * end_radians * y_percent - end_radians;

  if (radians > vital::kPi * 0.6f || radians < -vital::kPi * 0.6f)
    return;

  float sin_value = sinf(radians);
  float cos_value = cosf(radians);
  float y = 2.0f * buffer + height * 0.5f + sin_value * height * 0.45f;

  float round_amount = fabsf(sin_value) * getWidth() * kWheelRoundAmountRatio;
  float line_height = cos_value * height * kLineWidthRatio;

  float distance_from_edge = std::min(y - 2.0f * buffer, height + 2.0f * buffer - y);
  float alpha = std::max(0.0f, std::min(1.0f, distance_from_edge / (height * kFadeRatio)));
  g.setColour(fill_color.interpolatedWith(line_color, alpha));
  float offset = (line_height + round_amount) / 2.0f;
  g.fillRoundedRectangle(buffer, y - offset, width, std::max(0.0f, line_height + round_amount), round_amount);

  g.setColour(fill_color);
  if (sin_value > 0.0f)
    y -= round_amount;
  else
    y += line_height;
  g.fillRoundedRectangle(buffer, y - offset, width, 2.0f * round_amount, round_amount);
}

void ControlWheel::paint(Graphics& g) {
  static constexpr int kNumLines = 6;
  static constexpr float kRotatePercent = 0.8f;

  float round_amount = getWidth() * kContainerRoundAmountRatio;

  Colour background = findColour(Skin::kWidgetSecondary1, true);
  Colour line_color = findColour(Skin::kWidgetAccent1, true);
  Colour fill_color = background;
  Colour marker_color = findColour(Skin::kWidgetPrimary1, true);

  float buffer = getWidth() * kBufferWidthRatio;
  g.setColour(background);
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(buffer, buffer), round_amount);

  float t = 1.0f - (getValue() - getMinimum()) / (getMaximum() - getMinimum());
  t = (t - 0.5f) * kRotatePercent + 0.5f;
  float ratio_spacing = 1.0f / kNumLines;
  for (int i = kNumLines; t + ratio_spacing * i >= 0.5f; --i) {
    float ratio = t + ratio_spacing * i;
    if (i == 0)
      paintLine(g, ratio, marker_color, fill_color);
    else
      paintLine(g, ratio, line_color, fill_color);
  }

  for (int i = -kNumLines; t + ratio_spacing * i < 0.5f; ++i) {
    float ratio = t + ratio_spacing * i;
    if (i == 0)
      paintLine(g, ratio, marker_color, fill_color);
    else
      paintLine(g, ratio, line_color, fill_color);
  }

  g.setColour(findColour(Skin::kBody, true));
  Path erase;
  erase.addRectangle(getLocalBounds().toFloat());
  erase.setUsingNonZeroWinding(false);
  erase.addRoundedRectangle(getLocalBounds().toFloat().reduced(buffer, buffer), round_amount);
  g.fillPath(erase);

  g.setColour(findColour(Skin::kShadow, true));
  Path shadow;
  shadow.addRoundedRectangle(getLocalBounds().toFloat(), round_amount);
  shadow.setUsingNonZeroWinding(false);
  shadow.addRoundedRectangle(getLocalBounds().toFloat().reduced(buffer, buffer), round_amount);
  g.fillPath(shadow);
}

void ControlWheel::parentHierarchyChanged() {
  if (parent_ == nullptr)
    parent_ = findParentComponentOfClass<SynthGuiInterface>();

  SynthSlider::parentHierarchyChanged();
}

ModWheel::ModWheel() : ControlWheel("mod_wheel") { }

PitchWheel::PitchWheel() : ControlWheel("pitch_wheel") { }

void PitchWheel::mouseUp(const MouseEvent& e) {
  SynthSlider::mouseUp(e);
  setValue(0.0, sendNotification);
}

BendSection::BendSection(const String& name) : SynthSection(name) {
  pitch_wheel_ = std::make_unique<PitchWheel>();
  addSlider(pitch_wheel_.get());
  pitch_wheel_->setPopupPlacement(BubbleComponent::above);

  mod_wheel_ = std::make_unique<ModWheel>();
  addSlider(mod_wheel_.get());
  mod_wheel_->setPopupPlacement(BubbleComponent::above);

  setSkinOverride(Skin::kKeyboard);
}

BendSection::~BendSection() = default;

void BendSection::paintBackground(Graphics& g) {
  paintBody(g);
  paintBorder(g);
  paintOpenGlChildrenBackgrounds(g);
  paintKnobShadows(g);
}

void BendSection::resized() {
  int widget_margin = getWidgetMargin();
  int x_padding = getWidth() * 0.16f;
  x_padding -= (getWidth() + x_padding) % 2;
  int wheel_height = getHeight() - 2 * widget_margin;

  int right1 = (getWidth() - x_padding) / 2;
  pitch_wheel_->setBounds(x_padding, widget_margin, right1 - x_padding, wheel_height);
  int x2 = right1 + x_padding;
  mod_wheel_->setBounds(x2, widget_margin, getWidth() - x_padding - x2, wheel_height);

  SynthSection::resized();
}

void BendSection::sliderValueChanged(Slider* changed_slider) {
  SynthSection::sliderValueChanged(changed_slider);
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  if (changed_slider == mod_wheel_.get())
    parent->getSynth()->modWheelGuiChanged(changed_slider->getValue());
  else if (changed_slider == pitch_wheel_.get())
    parent->getSynth()->pitchWheelGuiChanged(changed_slider->getValue());
}
