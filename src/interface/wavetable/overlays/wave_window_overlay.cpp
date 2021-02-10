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

#include "wave_window_overlay.h"

#include "skin.h"
#include "wave_frame.h"
#include "text_look_and_feel.h"
#include "text_selector.h"

namespace {
  const std::string kWindowShapeLookup[WaveWindowModifier::kNumWindowShapes] = {
    "Raised Cos",
    "Half Sin",
    "Linear",
    "Square",
    "Wiggle"
  };
} // namespace

WaveWindowOverlay::WaveWindowOverlay() : WavetableComponentOverlay("WAVE WINDOW"), wave_window_modifier_(nullptr) {
  editor_ = std::make_unique<WaveWindowEditor>();
  addAndMakeVisible(editor_.get());
  addOpenGlComponent(editor_.get());
  editor_->setAlwaysOnTop(true);
  editor_->setFit(true);
  editor_->setFill(true);
  editor_->addListener(this);
  editor_->setVisible(false);
  current_frame_ = nullptr;

  window_shape_ = std::make_unique<TextSelector>("Window Shape");
  addSlider(window_shape_.get());
  window_shape_->setAlwaysOnTop(true);
  window_shape_->getImageComponent()->setAlwaysOnTop(true);
  window_shape_->setRange(0, WaveWindowModifier::kNumWindowShapes - 1);
  window_shape_->setLongStringLookup(kWindowShapeLookup);
  window_shape_->setStringLookup(kWindowShapeLookup);
  window_shape_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
  window_shape_->setLookAndFeel(TextLookAndFeel::instance());
  window_shape_->addListener(this);

  left_position_ = std::make_unique<SynthSlider>("Left Position");
  addSlider(left_position_.get());
  left_position_->setAlwaysOnTop(true);
  left_position_->getQuadComponent()->setAlwaysOnTop(true);
  left_position_->setRange(0.0, 1.0);
  left_position_->setDoubleClickReturnValue(true, 0.0);
  left_position_->addListener(this);
  left_position_->setSliderStyle(Slider::LinearBar);

  right_position_ = std::make_unique<SynthSlider>("Right Position");
  addSlider(right_position_.get());
  right_position_->setAlwaysOnTop(true);
  right_position_->getQuadComponent()->setAlwaysOnTop(true);
  right_position_->setRange(0.0, 1.0);
  right_position_->setDoubleClickReturnValue(true, 1.0);
  right_position_->addListener(this);
  right_position_->setSliderStyle(Slider::LinearBar);

  controls_background_.clearTitles();
  controls_background_.addTitle("");
  controls_background_.addTitle("LEFT POSITION");
  controls_background_.addTitle("RIGHT POSITION");
}

void WaveWindowOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr) {
    editor_->setVisible(false);
    current_frame_ = nullptr;
  }
  else if (keyframe->owner() == wave_window_modifier_) {
    editor_->setVisible(true);
    current_frame_ = wave_window_modifier_->getKeyframe(keyframe->index());
    float left = current_frame_->getLeft();
    float right = current_frame_->getRight();
    editor_->setPositions(left, right);
    left_position_->setValue(left, dontSendNotification);
    right_position_->setValue(right, dontSendNotification);
    left_position_->redoImage();
    right_position_->redoImage();
  }
}

void WaveWindowOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kWaveShapeWidthHeightRatio = 5.0f;
  static constexpr float kPositionPaddingRatio = 0.5f;
  static constexpr float kPositionWidthHeightRatio = 5.0f;

  Colour line_color = findColour(Skin::kWidgetPrimary1, true);
  Colour fill_color = findColour(Skin::kWidgetSecondary1, true).withMultipliedAlpha(0.5f);
  editor_->setColor(line_color);
  editor_->setFillColors(fill_color.withMultipliedAlpha(1.0f - findValue(Skin::kWidgetFillFade)), fill_color);

  int padding = getPadding();
  int window_shape_width = bounds.getHeight() * kWaveShapeWidthHeightRatio;
  int position_width = bounds.getHeight() * kPositionWidthHeightRatio;
  int position_padding = bounds.getHeight() * kPositionPaddingRatio;
  int total_width = window_shape_width + 2 * position_width + 2 * padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY();
  int y_title = y + title_height;
  int height = bounds.getHeight();
  int height_title = height - title_height;
  window_shape_->setBounds(x, y, window_shape_width, height);
  window_shape_->setTextHeightPercentage(0.4f);
  left_position_->setBounds(window_shape_->getRight() + padding + position_padding, y_title,
                            position_width - 2 * position_padding, height_title);
  right_position_->setBounds(left_position_->getRight() + padding + 2 * position_padding, y_title,
                             position_width - 2 * position_padding, height_title);

  controls_background_.clearLines();
  controls_background_.addLine(window_shape_width);
  controls_background_.addLine(window_shape_width + position_width + padding);

  window_shape_->redoImage();
  left_position_->redoImage();
  right_position_->redoImage();
}

bool WaveWindowOverlay::setTimeDomainBounds(Rectangle<int> bounds) {
  editor_->setBounds(bounds);
  return true;
}

void WaveWindowOverlay::windowChanged(bool left, bool mouse_up) {
  if (current_frame_ == nullptr)
    return;

  current_frame_->setLeft(editor_->getLeftPosition());
  current_frame_->setRight(editor_->getRightPosition());
  left_position_->setValue(editor_->getLeftPosition(), sendNotificationSync);
  right_position_->setValue(editor_->getRightPosition(), sendNotificationSync);
  notifyChanged(mouse_up);
}

void WaveWindowOverlay::sliderValueChanged(Slider* moved_slider) {
  if (wave_window_modifier_ == nullptr || current_frame_ == nullptr)
    return;
  
  if (moved_slider == window_shape_.get()) {
    int value = static_cast<int>(window_shape_->getValue());
    WaveWindowModifier::WindowShape window_shape = static_cast<WaveWindowModifier::WindowShape>(value);
    editor_->setWindowShape(window_shape);
    wave_window_modifier_->setWindowShape(window_shape);
    notifyChanged(true);
  }
  else if (moved_slider == left_position_.get()) {
    float value = std::min(left_position_->getValue(), right_position_->getValue());
    left_position_->setValue(value, dontSendNotification);
    current_frame_->setLeft(value);
    editor_->setPositions(value, right_position_->getValue());
    notifyChanged(false);
  }
  else if (moved_slider == right_position_.get()) {
    float value = std::max(right_position_->getValue(), left_position_->getValue());
    right_position_->setValue(value, dontSendNotification);
    current_frame_->setRight(value);
    editor_->setPositions(left_position_->getValue(), value);
    notifyChanged(false);
  }
}

void WaveWindowOverlay::sliderDragEnded(Slider* moved_slider) {
  notifyChanged(true);
}
