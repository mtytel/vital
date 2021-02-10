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

#include "slew_limiter_overlay.h"

#include "skin.h"
#include "wave_frame.h"
#include "synth_slider.h"
#include "text_look_and_feel.h"

SlewLimiterOverlay::SlewLimiterOverlay() : WavetableComponentOverlay("SLEW LIMITER"), slew_modifier_(nullptr) {
  current_frame_ = nullptr;

  up_slew_limit_ = std::make_unique<SynthSlider>("up_slew_limit");
  addSlider(up_slew_limit_.get());
  up_slew_limit_->setAlwaysOnTop(true);
  up_slew_limit_->getImageComponent()->setAlwaysOnTop(true);
  up_slew_limit_->addListener(this);
  up_slew_limit_->setRange(0.0f, 1.0f);
  up_slew_limit_->setDoubleClickReturnValue(true, 0.0f);
  up_slew_limit_->setLookAndFeel(TextLookAndFeel::instance());
  up_slew_limit_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  down_slew_limit_ = std::make_unique<SynthSlider>("down_slew_limit");
  addSlider(down_slew_limit_.get());
  down_slew_limit_->setAlwaysOnTop(true);
  down_slew_limit_->getImageComponent()->setAlwaysOnTop(true);
  down_slew_limit_->addListener(this);
  down_slew_limit_->setRange(0.0f, 1.0f);
  down_slew_limit_->setDoubleClickReturnValue(true, 0.0f);
  down_slew_limit_->setLookAndFeel(TextLookAndFeel::instance());
  down_slew_limit_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  controls_background_.clearTitles();
  controls_background_.addTitle("DOWN LIMIT");
  controls_background_.addTitle("UP LIMIT");
}

void SlewLimiterOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr)
    current_frame_ = nullptr;
  else if (keyframe->owner() == slew_modifier_) {
    current_frame_ = slew_modifier_->getKeyframe(keyframe->index());
    up_slew_limit_->setValue(current_frame_->getSlewUpLimit(), dontSendNotification);
    down_slew_limit_->setValue(current_frame_->getSlewDownLimit(), dontSendNotification);
    up_slew_limit_->redoImage();
    down_slew_limit_->redoImage();
  }
}

void SlewLimiterOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kLimitWidthHeightRatio = 4.0f;

  int padding = getPadding();
  int limit_width = bounds.getHeight() * kLimitWidthHeightRatio;
  int total_width = 2 * limit_width + padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY() + title_height;
  int height = bounds.getHeight() - title_height;
  up_slew_limit_->setBounds(x, y, limit_width, height);
  down_slew_limit_->setBounds(up_slew_limit_->getRight() + padding, y, limit_width, height);

  controls_background_.clearLines();
  controls_background_.addLine(limit_width);

  up_slew_limit_->redoImage();
  down_slew_limit_->redoImage();
}

void SlewLimiterOverlay::sliderValueChanged(Slider* moved_slider) {
  if (current_frame_ == nullptr)
    return;

  if (moved_slider == up_slew_limit_.get()) {
    float value = up_slew_limit_->getValue();
    current_frame_->setSlewUpLimit(value);
  }
  else if (moved_slider == down_slew_limit_.get()) {
    float value = down_slew_limit_->getValue();
    current_frame_->setSlewDownLimit(value);
  }

  notifyChanged(false);
}

void SlewLimiterOverlay::sliderDragEnded(Slider* moved_slider) {
  notifyChanged(true);
}
