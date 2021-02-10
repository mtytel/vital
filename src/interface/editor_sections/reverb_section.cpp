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

#include "reverb_section.h"

#include "skin.h"
#include "fonts.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "tab_selector.h"

ReverbSection::ReverbSection(String name, const vital::output_map& mono_modulations) : SynthSection(name) {
  dry_wet_ = std::make_unique<SynthSlider>("reverb_dry_wet");
  addSlider(dry_wet_.get());
  dry_wet_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  high_pre_cutoff_ = std::make_unique<SynthSlider>("reverb_pre_high_cutoff");
  addSlider(high_pre_cutoff_.get());
  high_pre_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(high_pre_cutoff_.get());

  chorus_frequency_ = std::make_unique<SynthSlider>("reverb_chorus_frequency");
  addSlider(chorus_frequency_.get());
  chorus_frequency_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  low_gain_ = std::make_unique<SynthSlider>("reverb_low_shelf_gain");
  addSlider(low_gain_.get());
  low_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  high_gain_ = std::make_unique<SynthSlider>("reverb_high_shelf_gain");
  addSlider(high_gain_.get());
  high_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  decay_time_ = std::make_unique<SynthSlider>("reverb_decay_time");
  addSlider(decay_time_.get());
  decay_time_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  low_pre_cutoff_ = std::make_unique<SynthSlider>("reverb_pre_low_cutoff");
  addSlider(low_pre_cutoff_.get());
  low_pre_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(low_pre_cutoff_.get());

  low_cutoff_ = std::make_unique<SynthSlider>("reverb_low_shelf_cutoff");
  addSlider(low_cutoff_.get());
  low_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(low_cutoff_.get());

  high_cutoff_ = std::make_unique<SynthSlider>("reverb_high_shelf_cutoff");
  addSlider(high_cutoff_.get());
  high_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(high_cutoff_.get());

  chorus_amount_ = std::make_unique<SynthSlider>("reverb_chorus_amount");
  addSlider(chorus_amount_.get());
  chorus_amount_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  delay_ = std::make_unique<SynthSlider>("reverb_delay");
  addSlider(delay_.get());
  delay_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  size_ = std::make_unique<SynthSlider>("reverb_size");
  addSlider(size_.get());
  size_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  
  feedback_eq_response_ = std::make_unique<EqualizerResponse>();
  feedback_eq_response_->setDbBufferRatio(kFeedbackFilterBuffer);
  feedback_eq_response_->initReverb(mono_modulations);
  feedback_eq_response_->setLowSliders(low_cutoff_.get(), nullptr, low_gain_.get());
  feedback_eq_response_->setHighSliders(high_cutoff_.get(), nullptr, high_gain_.get());
  feedback_eq_response_->setDrawFrequencyLines(false);
  addAndMakeVisible(feedback_eq_response_.get());
  addOpenGlComponent(feedback_eq_response_.get());
  feedback_eq_response_->addListener(this);
  
  selected_eq_band_ = std::make_unique<TabSelector>("selected_band");
  addAndMakeVisible(selected_eq_band_.get());
  addOpenGlComponent(selected_eq_band_->getImageComponent());
  selected_eq_band_->setSliderStyle(Slider::LinearBar);
  selected_eq_band_->setRange(0, 1);
  selected_eq_band_->addListener(this);
  selected_eq_band_->setNames({"LOW", "HIGH"});
  selected_eq_band_->setFontHeightPercent(0.4f);
  selected_eq_band_->setScrollWheelEnabled(false);

  on_ = std::make_unique<SynthButton>("reverb_on");
  addButton(on_.get());
  setActivator(on_.get());
  setSkinOverride(Skin::kReverb);
  lowBandSelected();
}

ReverbSection::~ReverbSection() { }

void ReverbSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  g.setColour(findColour(Skin::kBodyText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 10.0f));
  
  drawLabelForComponent(g, TRANS("TIME"), decay_time_.get());
  drawLabelForComponent(g, TRANS("PRE LOW CUT"), low_pre_cutoff_.get());
  drawLabelForComponent(g, TRANS("PRE HIGH CUT"), high_pre_cutoff_.get());
  drawLabelForComponent(g, TRANS("CUTOFF"), low_cutoff_.get());
  drawLabelForComponent(g, TRANS("GAIN"), low_gain_.get());
  drawLabelForComponent(g, TRANS("CHORUS AMT"), chorus_amount_.get());
  drawLabelForComponent(g, TRANS("CHORUS FRQ"), chorus_frequency_.get());
  drawLabelForComponent(g, TRANS("DELAY"), delay_.get());
  drawLabelForComponent(g, TRANS("SIZE"), size_.get());
  drawLabelForComponent(g, TRANS("MIX"), dry_wet_.get());
}

void ReverbSection::resized() {
  int title_width = getTitleWidth();
  int widget_margin = findValue(Skin::kWidgetMargin);
  int eq_width = getHeight() - 2 * widget_margin;
  float feedback_widget_x = (getWidth() - title_width - eq_width - 2 * widget_margin) / 5.0f +
                            title_width + widget_margin;
  int section_height = getKnobSectionHeight();
  int widget_rounding = findValue(Skin::kWidgetRoundedCorner);
  int band_height = kFeedbackFilterBuffer * 0.5f * eq_width;

  int selected_x = feedback_widget_x + widget_rounding;
  int selected_width = eq_width - 2 * widget_rounding;
  selected_eq_band_->setBounds(selected_x, widget_margin, selected_width, band_height);
  feedback_eq_response_->setBounds(feedback_widget_x, widget_margin, eq_width, eq_width);

  int pre_cutoff_x = title_width + widget_margin;
  int pre_cutoff_width = feedback_widget_x - pre_cutoff_x - widget_margin;
  int knob_y2 = section_height - widget_margin;

  low_pre_cutoff_->setBounds(pre_cutoff_x, 0, pre_cutoff_width, section_height - widget_margin);
  high_pre_cutoff_->setBounds(pre_cutoff_x, knob_y2, pre_cutoff_width, section_height - widget_margin);

  int knobs_x = feedback_widget_x + eq_width;
  int knobs_width = getWidth() - knobs_x;

  placeKnobsInArea(Rectangle<int>(knobs_x, 0, knobs_width, section_height),
                   { low_cutoff_.get(), chorus_amount_.get(), delay_.get(), dry_wet_.get() });

  placeKnobsInArea(Rectangle<int>(knobs_x, knob_y2, knobs_width, section_height),
                   { low_gain_.get(), chorus_frequency_.get(), size_.get(), decay_time_.get() });

  high_cutoff_->setBounds(low_cutoff_->getBounds());
  high_gain_->setBounds(low_gain_->getBounds());

  SynthSection::resized();
}

void ReverbSection::setActive(bool active) {
  feedback_eq_response_->setActive(active);
  selected_eq_band_->setActive(active);
  SynthSection::setActive(active);
}

void ReverbSection::lowBandSelected() {
  selected_eq_band_->setValue(0.0, dontSendNotification);
  selected_eq_band_->redoImage();
  low_cutoff_->setVisible(true);
  low_gain_->setVisible(true);

  high_cutoff_->setVisible(false);
  high_gain_->setVisible(false);
}

void ReverbSection::highBandSelected() {
  selected_eq_band_->setValue(1.0, dontSendNotification);
  selected_eq_band_->redoImage();
  low_cutoff_->setVisible(false);
  low_gain_->setVisible(false);
  
  high_cutoff_->setVisible(true);
  high_gain_->setVisible(true);
}

void ReverbSection::sliderValueChanged(Slider* slider) {
  if (slider == selected_eq_band_.get()) {
    if (selected_eq_band_->getValue() == 0.0f)
      lowBandSelected();
    else
      highBandSelected();
    
    feedback_eq_response_->setSelectedBand(selected_eq_band_->getValue() * 2.0f);
  }
  else
    SynthSection::sliderValueChanged(slider);
}
