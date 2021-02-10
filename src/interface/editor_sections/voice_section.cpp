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

#include "voice_section.h"

#include "fonts.h"
#include "operators.h"
#include "paths.h"
#include "synth_strings.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"

VoiceSection::VoiceSection(String name) : SynthSection(name) {
  static constexpr double kKnobSensitivity = 0.2;

  polyphony_ = std::make_unique<SynthSlider>("polyphony");
  addSlider(polyphony_.get());
  polyphony_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  polyphony_->setSensitivity(kKnobSensitivity);
  polyphony_->setLookAndFeel(TextLookAndFeel::instance());
  polyphony_->useSuffix(false);

  velocity_track_ = std::make_unique<SynthSlider>("velocity_track");
  addSlider(velocity_track_.get());
  velocity_track_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  velocity_track_->setBipolar(true);

  pitch_bend_range_ = std::make_unique<SynthSlider>("pitch_bend_range");
  addSlider(pitch_bend_range_.get());
  pitch_bend_range_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  pitch_bend_range_->setSensitivity(kKnobSensitivity);
  pitch_bend_range_->setLookAndFeel(TextLookAndFeel::instance());
  pitch_bend_range_->useSuffix(false);

  stereo_routing_ = std::make_unique<SynthSlider>("stereo_routing");
  stereo_routing_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  addSlider(stereo_routing_.get());

  stereo_mode_text_ = std::make_unique<PlainTextComponent>("Stereo Mode Text", "---");
  addOpenGlComponent(stereo_mode_text_.get());
  stereo_mode_text_->setText(strings::kStereoModeNames[0]);

  stereo_mode_type_selector_ = std::make_unique<ShapeButton>("Stereo Mode", Colours::black,
                                                             Colours::black, Colours::black);
  addAndMakeVisible(stereo_mode_type_selector_.get());
  stereo_mode_type_selector_->addListener(this);
  stereo_mode_type_selector_->setTriggeredOnMouseDown(true);

  setSkinOverride(Skin::kKeyboard);
}

VoiceSection::~VoiceSection() { }

void VoiceSection::paintBackground(Graphics& g) {
  paintBody(g);
  paintBorder(g);
  paintKnobShadows(g);
  paintOpenGlChildrenBackgrounds(g);

  drawTextComponentBackground(g, polyphony_->getBounds(), true);
  setLabelFont(g);
  drawLabelForComponent(g, TRANS("VOICES"), polyphony_.get(), true);
  drawLabelForComponent(g, TRANS("VEL TRK"), velocity_track_.get());
  drawLabelForComponent(g, TRANS(""), stereo_routing_.get());

  drawTextComponentBackground(g, pitch_bend_range_->getBounds(), true);
  drawLabelForComponent(g, TRANS("BEND"), pitch_bend_range_.get(), true);
}

void VoiceSection::resized() {
  stereo_mode_text_->setColor(findColour(Skin::kBodyText, true)); 
  int widget_margin = findValue(Skin::kWidgetMargin);
  int text_width = findValue(Skin::kModulationButtonWidth) - widget_margin;
  int text_height = getHeight() - 2 * widget_margin;
  polyphony_->setBounds(widget_margin, widget_margin, text_width, text_height);
  pitch_bend_range_->setBounds(polyphony_->getRight() + widget_margin, widget_margin, text_width, text_height);

  int knobs_x = pitch_bend_range_->getRight();
  Rectangle<int> knob_bounds(knobs_x, 0, getWidth() - knobs_x, getHeight());
  placeKnobsInArea(knob_bounds, { velocity_track_.get(), stereo_routing_.get() });

  Rectangle<int> stereo_label_bounds = getLabelBackgroundBounds(stereo_routing_->getBounds());
  stereo_mode_text_->setBounds(stereo_label_bounds);
  stereo_mode_text_->setTextSize(findValue(Skin::kLabelHeight));
  stereo_mode_type_selector_->setBounds(stereo_label_bounds);

  SynthSection::resized();
}

void VoiceSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  int stereo_mode = controls["stereo_mode"]->value();
  stereo_mode_text_->setText(strings::kStereoModeNames[stereo_mode]);
}

void VoiceSection::buttonClicked(Button* clicked_button) {
  int num_modes = vital::StereoEncoder::kNumStereoModes;
  if (clicked_button == stereo_mode_type_selector_.get()) {
    PopupItems options;

    for (int i = 0; i < num_modes; ++i)
      options.addItem(i, strings::kStereoModeNames[i]);
    
    showPopupSelector(this, Point<int>(clicked_button->getX(), clicked_button->getBottom()), options,
                      [=](int selection) { setStereoModeSelected(selection); });
  }
  else
    SynthSection::buttonClicked(clicked_button);
}

void VoiceSection::setStereoModeSelected(int selection) {
  stereo_mode_text_->setText(strings::kStereoModeNames[selection]);

  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal("stereo_mode", selection);
}
