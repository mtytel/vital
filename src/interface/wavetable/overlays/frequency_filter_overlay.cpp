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

#include "frequency_filter_overlay.h"

#include "skin.h"
#include "wave_frame.h"
#include "text_look_and_feel.h"
#include "text_selector.h"

namespace {
  const std::string filter_style_lookup[FrequencyFilterModifier::kNumFilterStyles] = {
    "Low Pass",
    "Band Pass",
    "High Pass",
    "Comb",
  };
} // namespace

FrequencyFilterOverlay::FrequencyFilterOverlay() :
    WavetableComponentOverlay("FREQUENCY FILTER"), frequency_modifier_(nullptr) {
  current_frame_ = nullptr;

  style_ = std::make_unique<TextSelector>("Filter Style");
  addSlider(style_.get());
  style_->setAlwaysOnTop(true);
  style_->getImageComponent()->setAlwaysOnTop(true);
  style_->setRange(0, FrequencyFilterModifier::kNumFilterStyles - 1);
  style_->setLongStringLookup(filter_style_lookup);
  style_->setStringLookup(filter_style_lookup);
  style_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  style_->setLookAndFeel(TextLookAndFeel::instance());
  style_->addListener(this);

  normalize_ = std::make_unique<OpenGlToggleButton>("NORMALIZE");
  addAndMakeVisible(normalize_.get());
  addOpenGlComponent(normalize_->getGlComponent());
  normalize_->setAlwaysOnTop(true);
  normalize_->getGlComponent()->setAlwaysOnTop(true);
  normalize_->setNoBackground();
  normalize_->setLookAndFeel(TextLookAndFeel::instance());
  normalize_->addListener(this);

  cutoff_ = std::make_unique<SynthSlider>("Frequency Filter Cutoff");
  addSlider(cutoff_.get());
  cutoff_->setAlwaysOnTop(true);
  cutoff_->getImageComponent()->setAlwaysOnTop(true);
  cutoff_->addListener(this);
  cutoff_->setRange(0.0f, vital::WaveFrame::kWaveformBits - 1);
  cutoff_->setDoubleClickReturnValue(true, 4.0f);
  cutoff_->setLookAndFeel(TextLookAndFeel::instance());
  cutoff_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  shape_ = std::make_unique<SynthSlider>("Frequency Filter Shape");
  addSlider(shape_.get());
  shape_->setAlwaysOnTop(true);
  shape_->getImageComponent()->setAlwaysOnTop(true);
  shape_->addListener(this);
  shape_->setRange(0.0f, 1.0f);
  shape_->setDoubleClickReturnValue(true, 0.5f);
  shape_->setLookAndFeel(TextLookAndFeel::instance());
  shape_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  controls_background_.clearTitles();
  controls_background_.addTitle("STYLE");
  controls_background_.addTitle("CUTOFF");
  controls_background_.addTitle("SHAPE");
}

void FrequencyFilterOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr)
    current_frame_ = nullptr;
  else if (keyframe->owner() == frequency_modifier_) {
    current_frame_ = frequency_modifier_->getKeyframe(keyframe->index());
    cutoff_->setValue(current_frame_->getCutoff(), dontSendNotification);
    shape_->setValue(current_frame_->getShape(), dontSendNotification);
    normalize_->setToggleState(frequency_modifier_->getNormalize(), dontSendNotification);
    style_->setValue(frequency_modifier_->getStyle(), dontSendNotification);

    cutoff_->redoImage();
    shape_->redoImage();
  }
}

void FrequencyFilterOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kSectionWidthHeightRatio = 4.0f;

  int padding = getPadding();
  int section_width = bounds.getHeight() * kSectionWidthHeightRatio;
  int total_width = 4 * section_width + 3 * padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY();
  int y_title = y + title_height;
  int height = bounds.getHeight();
  int height_title = height - title_height;
  style_->setTextHeightPercentage(0.4f);
  style_->setBounds(x, y_title, section_width, height_title);
  cutoff_->setBounds(style_->getRight() + padding, y_title, section_width, height_title);
  shape_->setBounds(cutoff_->getRight() + padding, y_title, section_width, height_title);
  int normalize_padding = height / 6;
  normalize_->setBounds(shape_->getRight(), y + normalize_padding,
                        section_width, height - 2 * normalize_padding);

  controls_background_.clearLines();
  controls_background_.addLine(section_width);
  controls_background_.addLine(2 * section_width + padding);
  controls_background_.addLine(3 * section_width + 2 * padding);

  style_->redoImage();
}

bool FrequencyFilterOverlay::setFrequencyAmplitudeBounds(Rectangle<int> bounds) {
  return true;
}

void FrequencyFilterOverlay::sliderValueChanged(Slider* moved_slider) {
  if (current_frame_ == nullptr || frequency_modifier_ == nullptr)
    return;

  if (moved_slider == style_.get()) {
    int value = style_->getValue();
    frequency_modifier_->setStyle(static_cast<FrequencyFilterModifier::FilterStyle>(value));
  }
  else if (moved_slider == cutoff_.get()) {
    float value = cutoff_->getValue();
    current_frame_->setCutoff(value);
  }
  else if (moved_slider == shape_.get()) {
    float value = shape_->getValue();
    current_frame_->setShape(value);
  }

  notifyChanged(moved_slider == style_.get());
}

void FrequencyFilterOverlay::sliderDragEnded(Slider* moved_slider) {
  notifyChanged(true);
}

void FrequencyFilterOverlay::buttonClicked(Button* clicked_button) {
  if (clicked_button == normalize_.get() && frequency_modifier_) {
    frequency_modifier_->setNormalize(normalize_->getToggleState());
    notifyChanged(true);
  }
}
