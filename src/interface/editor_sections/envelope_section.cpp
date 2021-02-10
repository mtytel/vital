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

#include "envelope_section.h"

#include "skin.h"
#include "envelope_editor.h"
#include "fonts.h"
#include "paths.h"
#include "modulation_button.h"
#include "synth_slider.h"

DragMagnifyingGlass::DragMagnifyingGlass() : OpenGlShapeButton("Magnifying Glass") {
  setShape(Paths::magnifyingGlass());
}

void DragMagnifyingGlass::mouseDown(const MouseEvent& e) {
  OpenGlShapeButton::mouseDown(e);
  last_position_ = e.position;

  MouseInputSource source = e.source;
  if (source.isMouse() && source.canDoUnboundedMovement()) {
    source.hideCursor();
    source.enableUnboundedMouseMovement(true);
    mouse_down_position_ = e.getScreenPosition();
  }
}

void DragMagnifyingGlass::mouseUp(const MouseEvent& e) {
  OpenGlShapeButton::mouseUp(e);

  MouseInputSource source = e.source;
  if (source.isMouse() && source.canDoUnboundedMovement()) {
    source.showMouseCursor(MouseCursor::NormalCursor);
    source.enableUnboundedMouseMovement(false);
    source.setScreenPosition(mouse_down_position_.toFloat());
  }
}

void DragMagnifyingGlass::mouseDrag(const MouseEvent& e) {
  Point<float> position = e.position;
  Point<float> delta_position = position - last_position_;
  last_position_ = position;

  for (Listener* listener : listeners_)
    listener->magnifyDragged(delta_position);

  OpenGlShapeButton::mouseDrag(e);
}

void DragMagnifyingGlass::mouseDoubleClick(const MouseEvent& e) {
  for (Listener* listener : listeners_)
    listener->magnifyDoubleClicked();
  
  OpenGlShapeButton::mouseDoubleClick(e);
}

EnvelopeSection::EnvelopeSection(String name, std::string value_prepend,
                                 const vital::output_map& mono_modulations,
                                 const vital::output_map& poly_modulations) : SynthSection(name) {
  delay_ = std::make_unique<SynthSlider>(value_prepend + "_delay");
  addSlider(delay_.get());
  delay_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  delay_->setPopupPlacement(BubbleComponent::below);
  
  attack_ = std::make_unique<SynthSlider>(value_prepend + "_attack");
  addSlider(attack_.get());
  attack_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  attack_->setPopupPlacement(BubbleComponent::below);

  attack_power_ = std::make_unique<SynthSlider>(value_prepend + "_attack_power");
  addSlider(attack_power_.get());
  attack_power_->setVisible(false);

  hold_ = std::make_unique<SynthSlider>(value_prepend + "_hold");
  addSlider(hold_.get());
  hold_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  hold_->setPopupPlacement(BubbleComponent::below);

  decay_ = std::make_unique<SynthSlider>(value_prepend + "_decay");
  addSlider(decay_.get());
  decay_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  decay_->setPopupPlacement(BubbleComponent::below);
  
  decay_power_ = std::make_unique<SynthSlider>(value_prepend + "_decay_power");
  addSlider(decay_power_.get());
  decay_power_->setVisible(false);

  release_ = std::make_unique<SynthSlider>(value_prepend + "_release");
  addSlider(release_.get());
  release_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  release_->setPopupPlacement(BubbleComponent::below);

  release_power_ = std::make_unique<SynthSlider>(value_prepend + "_release_power");
  addSlider(release_power_.get());
  release_power_->setVisible(false);

  sustain_ = std::make_unique<SynthSlider>(value_prepend + "_sustain");
  addSlider(sustain_.get());
  sustain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  sustain_->setPopupPlacement(BubbleComponent::below);

  envelope_ = std::make_unique<EnvelopeEditor>(value_prepend, mono_modulations, poly_modulations);
  addOpenGlComponent(envelope_.get());
  envelope_->setName(value_prepend);
  envelope_->setDelaySlider(delay_.get());
  envelope_->setAttackSlider(attack_.get());
  envelope_->setAttackPowerSlider(attack_power_.get());
  envelope_->setHoldSlider(hold_.get());
  envelope_->setDecaySlider(decay_.get());
  envelope_->setDecayPowerSlider(decay_power_.get());
  envelope_->setSustainSlider(sustain_.get());
  envelope_->setReleaseSlider(release_.get());
  envelope_->setReleasePowerSlider(release_power_.get());
  envelope_->resetEnvelopeLine(-1);

  drag_magnifying_glass_ = std::make_unique<DragMagnifyingGlass>();
  drag_magnifying_glass_->addListener(this);
  addAndMakeVisible(drag_magnifying_glass_.get());
  addOpenGlComponent(drag_magnifying_glass_->getGlComponent());
  setSkinOverride(Skin::kEnvelope);
}

EnvelopeSection::~EnvelopeSection() { }

void EnvelopeSection::paintBackground(Graphics& g) {
  setLabelFont(g);
  drawLabelForComponent(g, TRANS("DELAY"), delay_.get());
  drawLabelForComponent(g, TRANS("ATTACK"), attack_.get());
  drawLabelForComponent(g, TRANS("HOLD"), hold_.get());
  drawLabelForComponent(g, TRANS("DECAY"), decay_.get());
  drawLabelForComponent(g, TRANS("SUSTAIN"), sustain_.get());
  drawLabelForComponent(g, TRANS("RELEASE"), release_.get());

  paintKnobShadows(g);
  paintChildrenBackgrounds(g);
}

void EnvelopeSection::resized() {
  static constexpr float kMagnifyingHeightRatio = 0.2f;
  int knob_section_height = getKnobSectionHeight();
  int knob_y = getHeight() - knob_section_height;

  int widget_margin = findValue(Skin::kWidgetMargin);
  int envelope_height = knob_y - widget_margin;
  envelope_->setBounds(widget_margin, widget_margin, getWidth() - 2 * widget_margin, envelope_height);

  Rectangle<int> knobs_area(0, knob_y, getWidth(), knob_section_height);
  placeKnobsInArea(knobs_area, { delay_.get(), attack_.get(), hold_.get(),
                                 decay_.get(), sustain_.get(), release_.get() });
  SynthSection::resized();
  envelope_->setSizeRatio(getSizeRatio());

  int magnify_height = envelope_->getHeight() * kMagnifyingHeightRatio;
  drag_magnifying_glass_->setBounds(envelope_->getRight() - magnify_height, envelope_->getY(),
                                    magnify_height, magnify_height);
}

void EnvelopeSection::reset() {
  envelope_->resetPositions();
  SynthSection::reset();
}

void EnvelopeSection::magnifyDragged(Point<float> delta) {
  envelope_->magnifyZoom(delta);
}

void EnvelopeSection::magnifyDoubleClicked() {
  envelope_->magnifyReset();
}
