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

#include "wave_source_overlay.h"

#include "skin.h"
#include "incrementer_buttons.h"
#include "synth_strings.h"
#include "text_look_and_feel.h"
#include "text_selector.h"
#include "wave_frame.h"

namespace {
  constexpr int kNumInterpolationTypes = 5;

  const std::string interpolation_types[kNumInterpolationTypes] = {
    "None",
    "Waveform Blend",
    "Spectral Blend",
    "Smooth Waveform Blend",
    "Smooth Spectral Blend",
  };

} // namespace

WaveSourceOverlay::WaveSourceOverlay() : WavetableComponentOverlay("WAVE SOURCE"), wave_source_(nullptr) {
  current_frame_ = nullptr;
  int waveform_size = vital::WaveFrame::kWaveformSize;
  oscillator_ = std::make_unique<WaveSourceEditor>(waveform_size);
  oscillator_->setGrid(kDefaultXGrid, kDefaultYGrid);
  oscillator_->setFill(true);
  oscillator_->setEditable(true);
  oscillator_->addListener(this);
  addOpenGlComponent(oscillator_.get());
  oscillator_->setVisible(false);

  int bar_size = vital::WaveFrame::kNumRealComplex;
  frequency_amplitudes_ = std::make_unique<BarEditor>(bar_size);
  frequency_amplitudes_->setSquareScale(true);
  frequency_amplitudes_->addListener(this);
  addOpenGlComponent(frequency_amplitudes_.get(), true);
  frequency_amplitudes_->setVisible(false);

  frequency_phases_ = std::make_unique<BarEditor>(bar_size);
  frequency_phases_->addListener(this);
  frequency_phases_->setClearValue(kDefaultPhase);
  addOpenGlComponent(frequency_phases_.get(), true);
  frequency_phases_->setVisible(false);

  controls_background_.toFront(false);

  interpolation_type_ = std::make_unique<TextSelector>("Interpolation");
  addSlider(interpolation_type_.get());
  interpolation_type_->setAlwaysOnTop(true);
  interpolation_type_->getImageComponent()->setAlwaysOnTop(true);
  interpolation_type_->setRange(0, kNumInterpolationTypes - 1);
  interpolation_type_->setLongStringLookup(interpolation_types);
  interpolation_type_->setStringLookup(interpolation_types);
  interpolation_type_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  interpolation_type_->setLookAndFeel(TextLookAndFeel::instance());
  interpolation_type_->addListener(this);

  horizontal_grid_ = std::make_unique<SynthSlider>("wave_source_horizontal_grid");
  horizontal_grid_->setValue(kDefaultXGrid, dontSendNotification);
  addSlider(horizontal_grid_.get());
  horizontal_grid_->setAlwaysOnTop(true);
  horizontal_grid_->getImageComponent()->setAlwaysOnTop(true);
  horizontal_grid_->addListener(this);
  horizontal_grid_->setRange(0, WavetableComponentOverlay::kMaxGrid, 1);
  horizontal_grid_->setDoubleClickReturnValue(true, kDefaultXGrid);
  horizontal_grid_->setLookAndFeel(TextLookAndFeel::instance());
  horizontal_grid_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  horizontal_incrementers_ = std::make_unique<IncrementerButtons>(horizontal_grid_.get());
  addAndMakeVisible(horizontal_incrementers_.get());

  vertical_grid_ = std::make_unique<SynthSlider>("wave_source_vertical_grid");
  vertical_grid_->setValue(kDefaultYGrid, dontSendNotification);
  addSlider(vertical_grid_.get());
  vertical_grid_->setAlwaysOnTop(true);
  vertical_grid_->getImageComponent()->setAlwaysOnTop(true);
  vertical_grid_->addListener(this);
  vertical_grid_->setRange(0, WavetableComponentOverlay::kMaxGrid, 1);
  vertical_grid_->setDoubleClickReturnValue(true, kDefaultYGrid);
  vertical_grid_->setLookAndFeel(TextLookAndFeel::instance());
  vertical_grid_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  vertical_incrementers_ = std::make_unique<IncrementerButtons>(vertical_grid_.get());
  addAndMakeVisible(vertical_incrementers_.get());

  controls_background_.clearTitles();
  controls_background_.addTitle("");
  controls_background_.addTitle("GRID X");
  controls_background_.addTitle("GRID Y");
}

void WaveSourceOverlay::resized() {
  WavetableComponentOverlay::resized();
  if (findParentComponentOfClass<SynthGuiInterface>() == nullptr)
    return;

  Colour line_color = findColour(Skin::kWidgetPrimary1, true);
  oscillator_->setColor(line_color);
  Colour fill_color1 = findColour(Skin::kWidgetSecondary1, true);
  Colour fill_color2 = fill_color1.withMultipliedAlpha(1.0f - findValue(Skin::kWidgetFillFade));
  oscillator_->setFillColors(fill_color2, fill_color1);

  Colour bar_color = findColour(Skin::kWidgetSecondary2, true);
  frequency_amplitudes_->setColor(bar_color);
  frequency_phases_->setColor(bar_color);
}

void WaveSourceOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr) {
    oscillator_->setVisible(false);
    frequency_amplitudes_->setVisible(false);
    frequency_phases_->setVisible(false);
    current_frame_ = nullptr;
  }
  else if (keyframe->owner() == wave_source_) {
    oscillator_->setVisible(true);
    frequency_amplitudes_->setVisible(true);
    frequency_phases_->setVisible(true);
    current_frame_ = wave_source_->getWaveFrame(keyframe->index());
    oscillator_->loadWaveform(current_frame_->time_domain);
    updateFrequencyDomain(current_frame_->frequency_domain);
  }
}

void WaveSourceOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kInterpolationWidthHeightRatio = 8.0f;
  static constexpr float kGridWidthHeightRatio = 2.0f;

  int padding = getPadding();
  int interpolation_width = bounds.getHeight() * kInterpolationWidthHeightRatio;
  int grid_width = bounds.getHeight() * kGridWidthHeightRatio;
  int total_width = interpolation_width + 2 * grid_width + 2 * padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY();
  int y_title = y + title_height;
  int height = bounds.getHeight();
  int height_title = height - title_height;
  interpolation_type_->setBounds(x, y, interpolation_width, height);
  interpolation_type_->setTextHeightPercentage(0.4f);
  horizontal_grid_->setBounds(interpolation_type_->getRight() + padding, y_title, grid_width, height_title);
  vertical_grid_->setBounds(horizontal_grid_->getRight() + padding, y_title, grid_width, height_title);

  horizontal_incrementers_->setBounds(horizontal_grid_->getRight() - height_title, y_title, height_title, height_title);
  vertical_incrementers_->setBounds(vertical_grid_->getRight() - height_title, y_title, height_title, height_title);

  controls_background_.clearLines();
  controls_background_.addLine(interpolation_width);
  controls_background_.addLine(interpolation_width + grid_width + padding);

  interpolation_type_->redoImage();
  horizontal_grid_->redoImage();
  vertical_grid_->redoImage();
}

bool WaveSourceOverlay::setTimeDomainBounds(Rectangle<int> bounds) {
  oscillator_->setBounds(bounds);
  return true;
}

bool WaveSourceOverlay::setFrequencyAmplitudeBounds(Rectangle<int> bounds) {
  frequency_amplitudes_->setBounds(bounds);
  return true;
}

bool WaveSourceOverlay::setPhaseBounds(Rectangle<int> bounds) {
  frequency_phases_->setBounds(bounds);
  return true;
}

void WaveSourceOverlay::updateFrequencyDomain(std::complex<float>* frequency_domain) {
  for (int i = 0; i < vital::WaveFrame::kNumRealComplex; ++i) {
    std::complex<float> frequency = frequency_domain[i];
    float amplitude = std::abs(frequency);
    float phase = kDefaultPhase;
    if (amplitude)
      phase = std::arg(frequency) / vital::kPi;

    float adjusted_amplitude = amplitude / vital::WaveFrame::kWaveformSize;
    frequency_amplitudes_->setScaledY(i, adjusted_amplitude);
    frequency_phases_->setY(i, phase);
  }
}

void WaveSourceOverlay::loadFrequencyDomain() {
  for (int i = 0; i < vital::WaveFrame::kNumRealComplex; ++i) {
    float amplitude = frequency_amplitudes_->scaledYAt(i);
    amplitude *= vital::WaveFrame::kWaveformSize;
    float phase = vital::kPi * frequency_phases_->yAt(i);
    std::complex<float> value = std::polar(amplitude, phase);
    current_frame_->frequency_domain[i] = value;
  }

  current_frame_->toTimeDomain();
  current_frame_->normalize();
  current_frame_->toFrequencyDomain();
}

void WaveSourceOverlay::valuesChanged(int start, int end, bool mouse_up) {
  if (current_frame_ == nullptr)
    return;

  for (int i = start; i <= end; ++i)
    current_frame_->time_domain[i] = oscillator_->valueAt(i);

  current_frame_->toFrequencyDomain();
  updateFrequencyDomain(current_frame_->frequency_domain);

  notifyChanged(mouse_up);
}

void WaveSourceOverlay::sliderValueChanged(Slider* moved_slider) {
  if (wave_source_ == nullptr)
    return;
  
  if (moved_slider == horizontal_grid_.get() || moved_slider == vertical_grid_.get())
    oscillator_->setGrid(horizontal_grid_->getValue(), vertical_grid_->getValue());
  else if (moved_slider == interpolation_type_.get()) {
    WaveSource::InterpolationStyle style = WaveSource::kNone;
    WaveSource::InterpolationMode mode = WaveSource::kTime;
    int value = interpolation_type_->getValue();
    if (value) {
      style = static_cast<WaveSource::InterpolationStyle>((value + 1) / 2);
      mode = static_cast<WaveSource::InterpolationMode>((value + 1) % 2);
    }
    
    wave_source_->setInterpolationStyle(style);
    wave_source_->setInterpolationMode(mode);
    
    notifyChanged(true);
  }
}

void WaveSourceOverlay::setInterpolationType(WaveSource::InterpolationStyle style,
                                             WaveSource::InterpolationMode mode) {
  if (style == WaveSource::kNone)
    interpolation_type_->setValue(0, sendNotificationSync);
  else
    interpolation_type_->setValue(2 * style + mode - 1, sendNotificationSync);
}

void WaveSourceOverlay::barsChanged(int start, int end, bool mouse_up) {
  loadFrequencyDomain();

  oscillator_->loadWaveform(current_frame_->time_domain);
  notifyChanged(mouse_up);
}
