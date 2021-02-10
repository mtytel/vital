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

#include "modulation_meter.h"

#include "open_gl_multi_quad.h"
#include "synth_gui_interface.h"
#include "shaders.h"
#include "synth_section.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"

ModulationMeter::ModulationMeter(const vital::Output* mono_total, const vital::Output* poly_total,
                                 const SynthSlider* slider, OpenGlMultiQuad* quads, int index) :
        mono_total_(mono_total), poly_total_(poly_total), destination_(slider),
        quads_(quads), index_(index), current_value_(0.0), mod_percent_(0.0) {

  rotary_ = destination_->isRotary() && !destination_->isTextOrCurve();

  if (destination_->getSliderStyle() == Slider::LinearBarVertical || destination_->isTextOrCurve())
    quads->setRotatedCoordinates(index, -1.0f, -1.0f, 2.0f, 2.0f);

  setInterceptsMouseClicks(false, false);
  updateDrawing(false);
}

ModulationMeter::~ModulationMeter() { }

void ModulationMeter::resized() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent) {
    std::vector<vital::ModulationConnection*> connections;
    connections = parent->getSynth()->getSourceConnections(getName().toStdString());
    setModulated(!connections.empty());
  }

  if (isVisible())
    setVertices();
  else
    collapseVertices();
}

void ModulationMeter::setActive(bool active) {
  if (active)
    setVertices();
  else
    collapseVertices();
}

Rectangle<float> ModulationMeter::getMeterBounds() {
  float width = getWidth();
  float height = getHeight();
  if (!destination_->isRotary() && !destination_->isTextOrCurve()) {
    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    int widget_margin = parent->getWidgetMargin();

    int total_width = destination_->isHorizontal() ? destination_->getHeight() : destination_->getWidth();
    int extra = total_width % 2;
    int slider_width = std::floor(SynthSlider::kLinearWidthPercent * total_width * 0.5f) * 2.0f + extra;

    int inner_area = (total_width - slider_width) / 2;
    int outer_area = inner_area - widget_margin;
    int meter_width = SynthSlider::kLinearModulationPercent * total_width;
    int border = std::max<int>(1, (widget_margin - meter_width) * 0.5f);

    if (destination_->isHorizontal())
      return Rectangle<float>(0.0f, outer_area + border, width, inner_area - outer_area - 2.0f * border);
    return Rectangle<float>(outer_area + border, 0.0f, inner_area - outer_area - 2.0f * border, height);
  }
  else if (!destination_->isTextOrCurve()) {
    float knob_scale = destination_->getKnobSizeScale();
    float meter_width = destination_->findValue(Skin::kKnobModMeterArcSize) * knob_scale;
    meter_width += destination_->findValue(Skin::kKnobModMeterArcThickness) * (1.0f - knob_scale);
    float offset = destination_->findValue(Skin::kKnobOffset);

    float center_x = getWidth() * 0.5f;
    float center_y = getHeight() * 0.5f;
    return Rectangle<float>(center_x - meter_width * 0.5f, center_y - meter_width * 0.5f + offset,
                            meter_width, meter_width);
  }

  return getLocalBounds().toFloat();
}

void ModulationMeter::setVertices() {
  Rectangle<int> parent_bounds = getParentComponent()->getBounds();
  Rectangle<int> bounds = getBounds();
  Rectangle<float> meter_bounds = getMeterBounds();
  float left = bounds.getX() + meter_bounds.getX();
  float right = bounds.getX() + meter_bounds.getRight();
  float top = parent_bounds.getHeight() - (bounds.getY() + meter_bounds.getY());
  float bottom = parent_bounds.getHeight() - (bounds.getY() + meter_bounds.getBottom());

  left_ = 2.0f * left / parent_bounds.getWidth() - 1.0f;
  right_ = 2.0f * right / parent_bounds.getWidth() - 1.0f;
  top_ = 2.0f * top / parent_bounds.getHeight() - 1.0f;
  bottom_ = 2.0f * bottom / parent_bounds.getHeight() - 1.0f;
  quads_->setQuad(index_, left_, bottom_, right_ - left_, top_ - bottom_);
}

void ModulationMeter::collapseVertices() {
  left_ = right_ = top_ = bottom_= 0.0f;

  quads_->setQuad(index_, left_, bottom_, right_ - left_, top_ - bottom_);
  mod_percent_ = 0.0f;
}

void ModulationMeter::setAmountQuadVertices(OpenGlQuad& quad) {
  Rectangle<float> meter_bounds = getMeterBounds();
  if (rotary_)
    meter_bounds.expand(2.0f, 2.0f);

  float width = getWidth();
  float height = getHeight();
  float left = 2.0f * meter_bounds.getX() / width - 1.0f;
  float bottom = 1.0f - 2.0f * meter_bounds.getBottom() / height;

  bool vertical_bar = destination_->getSliderStyle() == Slider::LinearBarVertical || destination_->isTextOrCurve();
  if (vertical_bar)
    quad.setRotatedCoordinates(0, -1.0f, -1.0f, 2.0f, 2.0f);
  else
    quad.setCoordinates(0, -1.0f, -1.0f, 2.0f, 2.0f);

  if (rotary_)
    quad.setQuad(0, left, bottom, 2.0f * meter_bounds.getWidth() / width, 2.0f * meter_bounds.getHeight() / height);
  else if (vertical_bar) {
    float thickness = 2.0f / width;
    quad.setQuad(0, left, bottom, thickness, 2.0f * meter_bounds.getHeight() / height);
  }
  else {
    float thickness = 2.0f / height;
    quad.setQuad(0, left, bottom + 2.0f * meter_bounds.getHeight() / height - thickness,
                 2.0f * meter_bounds.getWidth() / width, thickness);
  }
}

void ModulationMeter::updateDrawing(bool use_poly) {
  if (mono_total_) {
    current_value_ = mono_total_->trigger_value;
    if (poly_total_ && use_poly)
      current_value_ += poly_total_->trigger_value;
  }

  float range = destination_->getMaximum() - destination_->getMinimum();
  vital::poly_float value = (current_value_ - destination_->getMinimum()) * (1.0f / range);
  mod_percent_ = vital::utils::clamp(value, 0.0f, 1.0f);
  float knob_percent = (destination_->getValue() - destination_->getMinimum()) / range;

  vital::poly_float min_percent = vital::utils::min(mod_percent_, knob_percent);
  vital::poly_float max_percent = vital::utils::max(mod_percent_, knob_percent);

  quads_->setQuad(index_, left_, bottom_, right_ - left_, top_ - bottom_);

  if (rotary_) {
    if (&destination_->getLookAndFeel() == TextLookAndFeel::instance()) {
      min_percent = vital::utils::interpolate(-vital::kPi, 0.0f, min_percent);
      max_percent = vital::utils::interpolate(-vital::kPi, 0.0f, max_percent);
    }
    else {
      float angle = SynthSlider::kRotaryAngle;
      min_percent = vital::utils::interpolate(-angle, angle, min_percent);
      max_percent = vital::utils::interpolate(-angle, angle, max_percent);
    }
  }

  quads_->setShaderValue(index_, min_percent[0], 0);
  quads_->setShaderValue(index_, max_percent[0], 1);
  quads_->setShaderValue(index_, min_percent[1], 2);
  quads_->setShaderValue(index_, max_percent[1], 3);
}

void ModulationMeter::setModulationAmountQuad(OpenGlQuad& quad, float amount, bool bipolar) {
  float range = destination_->getMaximum() - destination_->getMinimum();
  float knob_percent = (destination_->getValue() - destination_->getMinimum()) / range;

  float min_percent = std::min(knob_percent + amount, knob_percent);
  float max_percent = std::max(knob_percent + amount, knob_percent);
  if (bipolar) {
    min_percent = std::min(knob_percent + amount * 0.5f, knob_percent - amount * 0.5f);
    max_percent = std::max(knob_percent + amount * 0.5f, knob_percent - amount * 0.5f);
  }

  if (rotary_) {
    if (&destination_->getLookAndFeel() == TextLookAndFeel::instance()) {
      min_percent = vital::utils::interpolate(-vital::kPi, 0.0f, min_percent);
      max_percent = vital::utils::interpolate(-vital::kPi, 0.0f, max_percent);
    }
    else {
      float angle = SynthSlider::kRotaryAngle;
      min_percent = vital::utils::interpolate(-angle, angle, min_percent);
      max_percent = vital::utils::interpolate(-angle, angle, max_percent);
      min_percent = std::max(-angle, min_percent);
      max_percent = std::min(angle, max_percent);
    }
  }

  quad.setShaderValue(0, min_percent, 0);
  quad.setShaderValue(0, max_percent, 1);
  quad.setShaderValue(0, min_percent, 2);
  quad.setShaderValue(0, max_percent, 3);
}
