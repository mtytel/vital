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

#include "equalizer_section.h"

#include "skin.h"
#include "equalizer_response.h"
#include "fonts.h"
#include "oscilloscope.h"
#include "sound_engine.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "tab_selector.h"

namespace {
  Path bufferPath(const Path& path) {
    static constexpr float kBuffer = 0.3f;
    Rectangle<float> bounds = path.getBounds();
    Path result = path;

    float top = bounds.getY() - kBuffer;
    float bottom = bounds.getBottom() + kBuffer;
    result.addLineSegment(Line<float>(bounds.getX(), top, bounds.getX(), top), 0.1f);
    result.addLineSegment(Line<float>(bounds.getX(), bottom, bounds.getX(), bottom), 0.1f);
    return result;
  }
}

EqualizerSection::EqualizerSection(String name, const vital::output_map& mono_modulations) : SynthSection(name) {
  parent_ = nullptr;

  low_mode_ = std::make_unique<OpenGlShapeButton>("eq_low_mode");
  low_mode_->useOnColors(true);
  low_mode_->setClickingTogglesState(true);
  addButton(low_mode_.get());
  low_mode_->addListener(this);
  low_mode_->setShape(bufferPath(Paths::highPass()));

  band_mode_ = std::make_unique<OpenGlShapeButton>("eq_band_mode");
  band_mode_->useOnColors(true);
  band_mode_->setClickingTogglesState(true);
  addButton(band_mode_.get());
  band_mode_->addListener(this);
  band_mode_->setShape(bufferPath(Paths::notch()));

  high_mode_ = std::make_unique<OpenGlShapeButton>("eq_high_mode");
  high_mode_->useOnColors(true);
  high_mode_->setClickingTogglesState(true);
  addButton(high_mode_.get());
  high_mode_->addListener(this);
  high_mode_->setShape(bufferPath(Paths::lowPass()));

  low_cutoff_ = std::make_unique<SynthSlider>("eq_low_cutoff");
  addSlider(low_cutoff_.get());
  low_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(low_cutoff_.get());

  low_resonance_ = std::make_unique<SynthSlider>("eq_low_resonance");
  addSlider(low_resonance_.get());
  low_resonance_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  low_gain_ = std::make_unique<SynthSlider>("eq_low_gain");
  addSlider(low_gain_.get());
  low_gain_->setBipolar();
  low_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  band_cutoff_ = std::make_unique<SynthSlider>("eq_band_cutoff");
  addSlider(band_cutoff_.get());
  band_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(band_cutoff_.get());

  band_resonance_ = std::make_unique<SynthSlider>("eq_band_resonance");
  addSlider(band_resonance_.get());
  band_resonance_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  band_gain_ = std::make_unique<SynthSlider>("eq_band_gain");
  addSlider(band_gain_.get());
  band_gain_->setBipolar();
  band_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  high_cutoff_ = std::make_unique<SynthSlider>("eq_high_cutoff");
  addSlider(high_cutoff_.get());
  high_cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  setSliderHasHzAlternateDisplay(high_cutoff_.get());

  high_resonance_ = std::make_unique<SynthSlider>("eq_high_resonance");
  addSlider(high_resonance_.get());
  high_resonance_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  high_gain_ = std::make_unique<SynthSlider>("eq_high_gain");
  addSlider(high_gain_.get());
  high_gain_->setBipolar();
  high_gain_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  spectrogram_ = std::make_unique<Spectrogram>();
  addOpenGlComponent(spectrogram_.get());
  float min_frequency = vital::utils::midiNoteToFrequency(low_cutoff_->getMinimum());
  spectrogram_->setMinFrequency(min_frequency);
  float max_frequency = vital::utils::midiNoteToFrequency(low_cutoff_->getMaximum());
  spectrogram_->setMaxFrequency(max_frequency);
  spectrogram_->setInterceptsMouseClicks(false, false);
  spectrogram_->setFill(true);

  eq_response_ = std::make_unique<EqualizerResponse>();
  eq_response_->initEq(mono_modulations);
  addOpenGlComponent(eq_response_.get());
  eq_response_->setLowSliders(low_cutoff_.get(), low_resonance_.get(), low_gain_.get());
  eq_response_->setBandSliders(band_cutoff_.get(), band_resonance_.get(), band_gain_.get());
  eq_response_->setHighSliders(high_cutoff_.get(), high_resonance_.get(), high_gain_.get());
  eq_response_->addListener(this);

  on_ = std::make_unique<SynthButton>("eq_on");
  addButton(on_.get());

  selected_band_ = std::make_unique<TabSelector>("selected_band");
  addAndMakeVisible(selected_band_.get());
  addOpenGlComponent(selected_band_->getImageComponent());
  selected_band_->setSliderStyle(Slider::LinearBar);
  selected_band_->setRange(0, 2);
  selected_band_->addListener(this);
  selected_band_->setNames({"LOW", "BAND", "HIGH"});
  selected_band_->setFontHeightPercent(0.4f);
  selected_band_->setScrollWheelEnabled(false);

  setActivator(on_.get());
  lowBandSelected();
  setSkinOverride(Skin::kEqualizer);
}

EqualizerSection::~EqualizerSection() { }

void EqualizerSection::paintBackground(Graphics& g) {
  SynthSection::paintBackground(g);

  Colour lighten = findColour(Skin::kLightenScreen, true);
  Colour background = findColour(Skin::kWidgetBackground, true);
  Colour color = background.overlaidWith(lighten);
  color = color.withAlpha(1.0f);
  spectrogram_->setColour(Skin::kWidgetPrimary1, color);
  spectrogram_->setColour(Skin::kWidgetPrimary2, color);
  spectrogram_->setColour(Skin::kWidgetSecondary1, color);
  spectrogram_->setColour(Skin::kWidgetSecondary2, color);
  spectrogram_->setLineWidth(2.5f);

  setLabelFont(g);
  drawLabelForComponent(g, "GAIN", low_gain_.get());
  drawLabelForComponent(g, "RESONANCE", low_resonance_.get());
  drawLabelForComponent(g, "CUTOFF", low_cutoff_.get());

  g.setColour(findColour(Skin::kTextComponentBackground, true));
  float button_rounding = findValue(Skin::kLabelBackgroundRounding);
  g.fillRoundedRectangle(low_mode_->getBounds().toFloat(), button_rounding);
  g.fillRoundedRectangle(band_mode_->getBounds().toFloat(), button_rounding);
  g.fillRoundedRectangle(high_mode_->getBounds().toFloat(), button_rounding);
}

void EqualizerSection::resized() {
  int title_width = getTitleWidth();
  Rectangle<int> bounds = getLocalBounds().withLeft(title_width);
  int widget_margin = findValue(Skin::kWidgetMargin);
  Rectangle<int> widget_bounds = getDividedAreaUnbuffered(bounds, 2, 1, widget_margin);
  spectrogram_->setBounds(widget_bounds.reduced(0, widget_margin));
  eq_response_->setBounds(spectrogram_->getBounds());
  
  Rectangle<int> knobs_area = getDividedAreaBuffered(bounds, 2, 0, widget_margin);

  int button_height = getTextComponentHeight();
  int section_height = getKnobSectionHeight();
  int knob_y = getHeight() - section_height;
  int button_y = widget_margin;

  placeKnobsInArea(Rectangle<int>(knobs_area.getX(), knob_y, knobs_area.getWidth(), section_height),
                   { low_gain_.get(), low_cutoff_.get(), low_resonance_.get() });

  band_cutoff_->setBounds(low_cutoff_->getBounds());
  band_resonance_->setBounds(low_resonance_->getBounds());
  band_gain_->setBounds(low_gain_->getBounds());

  high_cutoff_->setBounds(low_cutoff_->getBounds());
  high_resonance_->setBounds(low_resonance_->getBounds());
  high_gain_->setBounds(low_gain_->getBounds());

  low_mode_->setBounds(low_gain_->getX(), button_y, low_gain_->getWidth(), button_height);
  band_mode_->setBounds(low_cutoff_->getX(), button_y, low_cutoff_->getWidth(), button_height);
  high_mode_->setBounds(low_resonance_->getX(), button_y, low_resonance_->getWidth(), button_height);

  int selected_y = low_mode_->getBottom() + widget_margin;
  int selected_height = knob_y - selected_y + widget_margin;
  selected_band_->setBounds(title_width + widget_margin, selected_y,
                            knobs_area.getWidth() - 2 * widget_margin, selected_height);

  SynthSection::resized();
}

void EqualizerSection::setActive(bool active) {
  eq_response_->setActive(active);
  selected_band_->setActive(active);
  SynthSection::setActive(active);
  setGainActive();
}

void EqualizerSection::lowBandSelected() {
  selected_band_->setValue(0.0, dontSendNotification);
  selected_band_->redoImage();
  low_cutoff_->setVisible(true);
  low_resonance_->setVisible(true);
  low_gain_->setVisible(true);

  band_cutoff_->setVisible(false);
  band_resonance_->setVisible(false);
  band_gain_->setVisible(false);

  high_cutoff_->setVisible(false);
  high_resonance_->setVisible(false);
  high_gain_->setVisible(false);
}

void EqualizerSection::midBandSelected() {
  selected_band_->setValue(1.0, dontSendNotification);
  selected_band_->redoImage();
  low_cutoff_->setVisible(false);
  low_resonance_->setVisible(false);
  low_gain_->setVisible(false);

  band_cutoff_->setVisible(true);
  band_resonance_->setVisible(true);
  band_gain_->setVisible(true);

  high_cutoff_->setVisible(false);
  high_resonance_->setVisible(false);
  high_gain_->setVisible(false);
}

void EqualizerSection::highBandSelected() {
  selected_band_->setValue(2.0, dontSendNotification);
  selected_band_->redoImage();
  low_cutoff_->setVisible(false);
  low_resonance_->setVisible(false);
  low_gain_->setVisible(false);

  band_cutoff_->setVisible(false);
  band_resonance_->setVisible(false);
  band_gain_->setVisible(false);

  high_cutoff_->setVisible(true);
  high_resonance_->setVisible(true);
  high_gain_->setVisible(true);
}

void EqualizerSection::sliderValueChanged(Slider* slider) {
  if (slider == selected_band_.get()) {
    if (selected_band_->getValue() == 0)
      lowBandSelected();
    else if (selected_band_->getValue() == 1)
      midBandSelected();
    else if (selected_band_->getValue() == 2)
      highBandSelected();

    eq_response_->setSelectedBand(selected_band_->getValue());
  }
  else
    SynthSection::sliderValueChanged(slider);
}

void EqualizerSection::buttonClicked(Button* button) {
  setGainActive();
  if (button == low_mode_.get()) {
    eq_response_->setHighPass(low_mode_->getToggleState());
    selected_band_->setValue(0);
  }
  else if (button == band_mode_.get()) {
    eq_response_->setNotch(band_mode_->getToggleState());
    selected_band_->setValue(1);
  }
  else if (button == high_mode_.get()) {
    eq_response_->setLowPass(high_mode_->getToggleState());
    selected_band_->setValue(2);
  }
  SynthSection::buttonClicked(button);
}

void EqualizerSection::setScrollWheelEnabled(bool enabled) {
  selected_band_->setScrollWheelEnabled(enabled);
  SynthSection::setScrollWheelEnabled(enabled);
}

void EqualizerSection::setGainActive() {
  low_gain_->setActive(isActive() && !low_mode_->getToggleState());
  band_gain_->setActive(isActive() && !band_mode_->getToggleState());
  high_gain_->setActive(isActive() && !high_mode_->getToggleState());
}

void EqualizerSection::parentHierarchyChanged() {
  if (parent_)
    return;

  parent_ = findParentComponentOfClass<SynthGuiInterface>();

  if (parent_)
    spectrogram_->setAudioMemory(parent_->getSynth()->getEqualizerMemory());
}

void EqualizerSection::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  if (parent_) {
    int oversampling_amount = parent_->getSynth()->getEngine()->getOversamplingAmount();
    if (oversampling_amount >= 1)
      spectrogram_->setOversampleAmount(oversampling_amount);
  }
  spectrogram_->setColour(Skin::kWidgetPrimary1, findColour(Skin::kLightenScreen, true));
  SynthSection::renderOpenGlComponents(open_gl, animate);
}
