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

#include "compressor_section.h"

#include "compressor.h"
#include "compressor_editor.h"
#include "skin.h"
#include "fonts.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"

CompressorSection::CompressorSection(const String& name) : SynthSection(name) {
  release_ = std::make_unique<SynthSlider>("compressor_release");
  addSlider(release_.get());
  release_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  mix_ = std::make_unique<SynthSlider>("compressor_mix");
  addSlider(mix_.get());
  mix_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  attack_ = std::make_unique<SynthSlider>("compressor_attack");
  addSlider(attack_.get());
  attack_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  low_gain_ = std::make_unique<SynthSlider>("compressor_low_gain");
  addSlider(low_gain_.get());
  low_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  low_gain_->setBipolar(true);

  band_gain_ = std::make_unique<SynthSlider>("compressor_band_gain");
  addSlider(band_gain_.get());
  band_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  band_gain_->setBipolar(true);

  high_gain_ = std::make_unique<SynthSlider>("compressor_high_gain");
  addSlider(high_gain_.get());
  high_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  high_gain_->setBipolar(true);

  enabled_bands_ = std::make_unique<TextSelector>("compressor_enabled_bands");
  addSlider(enabled_bands_.get());
  enabled_bands_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  enabled_bands_->setLookAndFeel(TextLookAndFeel::instance());
  enabled_bands_->setLongStringLookup(strings::kCompressorBandNames);

  compressor_editor_ = std::make_unique<CompressorEditor>();
  addAndMakeVisible(compressor_editor_.get());
  addOpenGlComponent(compressor_editor_.get());

  on_ = std::make_unique<SynthButton>("compressor_on");
  addButton(on_.get());
  setActivator(on_.get());

  setSkinOverride(Skin::kCompressor);
}

CompressorSection::~CompressorSection() = default;

void CompressorSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);
  setLabelFont(g);
  
  drawTextComponentBackground(g, enabled_bands_->getBounds(), true);
  drawLabelForComponent(g, TRANS("MODE"), enabled_bands_.get(), true);
  drawLabelForComponent(g, TRANS("ATTACK"), attack_.get());
  drawLabelForComponent(g, TRANS("RELEASE"), release_.get());
  drawLabelForComponent(g, TRANS("MIX"), mix_.get());
  drawLabelForComponent(g, TRANS("LOW"), low_gain_.get());
  drawLabelForComponent(g, TRANS("BAND"), band_gain_.get());
  drawLabelForComponent(g, TRANS("HIGH"), high_gain_.get());
}

void CompressorSection::resized() {
  int widget_margin = findValue(Skin::kWidgetMargin);
  int title_width = getTitleWidth();
  int section_height = getKnobSectionHeight();
  
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 3, 0, widget_margin);
  Rectangle<int> time_area = getDividedAreaUnbuffered(bounds, 6, 5, widget_margin);
  Rectangle<int> settings_area = getDividedAreaUnbuffered(bounds, 3, 0, widget_margin);
  
  int editor_x = knobs_area.getRight();
  int editor_width = time_area.getX() - editor_x - widget_margin;
  int knob_y2 = section_height - widget_margin;

  placeKnobsInArea(Rectangle<int>(knobs_area.getX(), knob_y2, knobs_area.getWidth(), section_height),
                   { low_gain_.get(), band_gain_.get(), high_gain_.get() });

  int bands_width = band_gain_->getRight() - low_gain_->getX();
  enabled_bands_->setBounds(settings_area.getX(), widget_margin, bands_width, section_height - 2 * widget_margin);
  int mix_x = enabled_bands_->getRight();
  placeKnobsInArea(Rectangle<int>(mix_x, 0, knobs_area.getRight() - mix_x, section_height), { mix_.get() });

  attack_->setBounds(time_area.getX(), 0, time_area.getWidth(), section_height - widget_margin);
  release_->setBounds(time_area.getX(), knob_y2, time_area.getWidth(), section_height - widget_margin);

  int editor_height = getHeight() - 2 * widget_margin;
  compressor_editor_->setBounds(editor_x, widget_margin, editor_width, editor_height);

  SynthSection::resized();
}

void CompressorSection::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  compressor_editor_->setAllValues(controls);
  setCompressorActiveBands();
}

void CompressorSection::setActive(bool active) {
  SynthSection::setActive(active);
  compressor_editor_->setActive(active);
}

void CompressorSection::sliderValueChanged(Slider* changed_slider) {
  if (changed_slider == enabled_bands_.get())
    setCompressorActiveBands();
  SynthSection::sliderValueChanged(changed_slider);
}

void CompressorSection::setCompressorActiveBands() {
  int enabled_bands = enabled_bands_->getValue();
  bool low_enabled = enabled_bands == vital::MultibandCompressor::kLowBand ||
                     enabled_bands == vital::MultibandCompressor::kMultiband;
  bool high_enabled = enabled_bands == vital::MultibandCompressor::kHighBand ||
                      enabled_bands == vital::MultibandCompressor::kMultiband;
  compressor_editor_->setLowBandActive(low_enabled);
  compressor_editor_->setHighBandActive(high_enabled);
  low_gain_->setActive(low_enabled);
  high_gain_->setActive(high_enabled);
}
