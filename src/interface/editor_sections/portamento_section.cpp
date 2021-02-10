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

#include "portamento_section.h"

#include "fonts.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"
#include "curve_look_and_feel.h"

PortamentoSection::PortamentoSection(String name) : SynthSection(name) {
  portamento_ = std::make_unique<SynthSlider>("portamento_time");
  addSlider(portamento_.get());
  portamento_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  portamento_slope_ = std::make_unique<SynthSlider>("portamento_slope");
  addSlider(portamento_slope_.get());
  portamento_slope_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  portamento_slope_->setLookAndFeel(CurveLookAndFeel::instance());

  portamento_scale_ = std::make_unique<SynthButton>("portamento_scale");
  addButton(portamento_scale_.get());
  portamento_scale_->setButtonText("OCTAVE SCALE");
  portamento_scale_->setLookAndFeel(TextLookAndFeel::instance());

  portamento_force_ = std::make_unique<SynthButton>("portamento_force");
  addButton(portamento_force_.get());
  portamento_force_->setButtonText("ALWAYS GLIDE");
  portamento_force_->setLookAndFeel(TextLookAndFeel::instance());

  legato_ = std::make_unique<SynthButton>("legato");
  legato_->setButtonText("LEGATO");
  addButton(legato_.get());
  legato_->setLookAndFeel(TextLookAndFeel::instance());

  setSkinOverride(Skin::kKeyboard);
}

PortamentoSection::~PortamentoSection() { }

void PortamentoSection::paintBackground(Graphics& g) {
  paintBody(g);
  paintBorder(g);
  portamento_->drawShadow(g);

  setLabelFont(g);

  drawLabelForComponent(g, TRANS("GLIDE"), portamento_.get());
  Rectangle<int> slope_bounds = portamento_slope_->getBounds().withBottom(getHeight() - getWidgetMargin());
  drawTextComponentBackground(g, slope_bounds, true);
  drawLabel(g, TRANS("SLOPE"), slope_bounds, true);

  paintOpenGlChildrenBackgrounds(g);
}

void PortamentoSection::resized() {
  int height = getHeight();
  int buttons_width = 3 * getWidth() / 8;
  int buttons_x = getWidth() - buttons_width;
  int widget_margin = findValue(Skin::kWidgetMargin);
  int internal_margin = widget_margin / 2;
  float button_height = (height - 2 * (widget_margin + internal_margin)) / 3.0f;
  portamento_force_->setBounds(buttons_x, widget_margin, buttons_width - widget_margin, button_height);
  legato_->setBounds(buttons_x, height - widget_margin - button_height,
                     buttons_width - widget_margin, button_height);
  portamento_scale_->setBounds(buttons_x, portamento_force_->getBottom() + internal_margin,
                               buttons_width - widget_margin,
                               legato_->getY() - portamento_force_->getBottom() - 2 * internal_margin);

  Rectangle<int> knobs_bounds(0, 0, buttons_x, height);
  placeKnobsInArea(knobs_bounds, { portamento_.get(), portamento_slope_.get() });

  Rectangle<int> slope_bounds = portamento_slope_->getBounds().withTop(getWidgetMargin());

  int bottom = getLabelBackgroundBounds(portamento_slope_->getBounds(), true).getY();
  portamento_slope_->setBounds(slope_bounds.withBottom(bottom));
  SynthSection::resized();
}

void PortamentoSection::sliderValueChanged(Slider* changed_slider) {
  if (changed_slider == portamento_.get())
    portamento_slope_->setActive(portamento_->getValue() != portamento_->getMinimum());

  SynthSection::sliderValueChanged(changed_slider);
}

void PortamentoSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  portamento_slope_->setActive(portamento_->getValue() != portamento_->getMinimum());
}
