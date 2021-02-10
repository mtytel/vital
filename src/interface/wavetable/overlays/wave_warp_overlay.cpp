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

#include "wave_warp_overlay.h"

#include "skin.h"
#include "wave_frame.h"
#include "text_look_and_feel.h"

WaveWarpOverlay::WaveWarpOverlay() : WavetableComponentOverlay("WAVE WARPER"), warp_modifier_(nullptr) {
  current_frame_ = nullptr;
  horizontal_warp_ = std::make_unique<SynthSlider>("wave_warp_horizontal");
  addSlider(horizontal_warp_.get());
  horizontal_warp_->getImageComponent()->setAlwaysOnTop(true);
  horizontal_warp_->setAlwaysOnTop(true);
  horizontal_warp_->addListener(this);
  horizontal_warp_->setRange(-20.0f, 20.0f);
  horizontal_warp_->setDoubleClickReturnValue(true, 0.0f);
  horizontal_warp_->setLookAndFeel(TextLookAndFeel::instance());
  horizontal_warp_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  vertical_warp_ = std::make_unique<SynthSlider>("wave_warp_vertical");
  addSlider(vertical_warp_.get());
  vertical_warp_->getImageComponent()->setAlwaysOnTop(true);
  vertical_warp_->setAlwaysOnTop(true);
  vertical_warp_->addListener(this);
  vertical_warp_->setRange(-20.0f, 20.0f);
  vertical_warp_->setDoubleClickReturnValue(true, 0.0f);
  vertical_warp_->setLookAndFeel(TextLookAndFeel::instance());
  vertical_warp_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  horizontal_asymmetric_ = std::make_unique<OpenGlToggleButton>("X Asymmetric");
  addAndMakeVisible(horizontal_asymmetric_.get());
  addOpenGlComponent(horizontal_asymmetric_->getGlComponent());
  horizontal_asymmetric_->getGlComponent()->setAlwaysOnTop(true);
  horizontal_asymmetric_->setAlwaysOnTop(true);
  horizontal_asymmetric_->setNoBackground();
  horizontal_asymmetric_->setLookAndFeel(TextLookAndFeel::instance());
  horizontal_asymmetric_->addListener(this);

  vertical_asymmetric_ = std::make_unique<OpenGlToggleButton>("Y Asymmetric");
  addAndMakeVisible(vertical_asymmetric_.get());
  addOpenGlComponent(vertical_asymmetric_->getGlComponent());
  vertical_asymmetric_->getGlComponent()->setAlwaysOnTop(true);
  vertical_asymmetric_->setAlwaysOnTop(true);
  vertical_asymmetric_->setNoBackground();
  vertical_asymmetric_->setLookAndFeel(TextLookAndFeel::instance());
  vertical_asymmetric_->addListener(this);

  controls_background_.clearTitles();
  controls_background_.addTitle("");
  controls_background_.addTitle("X WARP");
  controls_background_.addTitle("");
  controls_background_.addTitle("Y WARP");
}

void WaveWarpOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr) {
    current_frame_ = nullptr;
  }
  else if (keyframe->owner() == warp_modifier_) {
    current_frame_ = warp_modifier_->getKeyframe(keyframe->index());
    horizontal_warp_->setValue(current_frame_->getHorizontalPower(), dontSendNotification);
    vertical_warp_->setValue(current_frame_->getVerticalPower(), dontSendNotification);
    horizontal_warp_->redoImage();
    vertical_warp_->redoImage();

    horizontal_asymmetric_->setToggleState(warp_modifier_->getHorizontalAsymmetric(), dontSendNotification);
    vertical_asymmetric_->setToggleState(warp_modifier_->getVerticalAsymmetric(), dontSendNotification);
  }
}

void WaveWarpOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kSymmetryWidthHeightRatio = 3.5f;
  static constexpr float kWarpWidthHeightRatio = 5.0f;
  static constexpr float kWarpPaddingRatio = 0.5f;

  int padding = getPadding();
  int symmetry_width = bounds.getHeight() * kSymmetryWidthHeightRatio;
  int warp_width = bounds.getHeight() * kWarpWidthHeightRatio;
  int warp_padding = bounds.getHeight() * kWarpPaddingRatio;
  int total_width = 2 * symmetry_width + 2 * warp_width + 3 * padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY();
  int y_title = y + title_height;
  int height = bounds.getHeight();
  int height_title = height - title_height;
  int symmetry_padding = height / 6.0f;
  horizontal_asymmetric_->setBounds(x, y + symmetry_padding, symmetry_width, height - 2 * symmetry_padding);
  horizontal_warp_->setBounds(horizontal_asymmetric_->getRight() + padding + warp_padding, y_title,
                              warp_width - 2 * warp_padding, height_title);
  vertical_asymmetric_->setBounds(horizontal_warp_->getRight() + padding + warp_padding, y + symmetry_padding,
                                  symmetry_width, height - 2 * symmetry_padding);
  vertical_warp_->setBounds(vertical_asymmetric_->getRight() + padding + warp_padding, y_title,
                            warp_width - 2 * warp_padding, height_title);

  controls_background_.clearLines();
  controls_background_.addLine(symmetry_width);
  controls_background_.addLine(symmetry_width + warp_width + padding);
  controls_background_.addLine(2 * symmetry_width + warp_width + 2 * padding);
  controls_background_.addLine(2 * symmetry_width + 2 * warp_width + 3 * padding);

  horizontal_warp_->redoImage();
  vertical_warp_->redoImage();
}

void WaveWarpOverlay::sliderValueChanged(Slider* moved_slider) {
  if (warp_modifier_ == nullptr || current_frame_ == nullptr)
    return;

  if (moved_slider == horizontal_warp_.get()) {
    float value = horizontal_warp_->getValue();
    current_frame_->setHorizontalPower(value);
  }
  else if (moved_slider == vertical_warp_.get()) {
    float value = vertical_warp_->getValue();
    current_frame_->setVerticalPower(value);
  }

  notifyChanged(false);
}

void WaveWarpOverlay::sliderDragEnded(Slider* moved_slider) {
  notifyChanged(true);
}

void WaveWarpOverlay::buttonClicked(Button* clicked_button) {
  if (warp_modifier_ == nullptr)
    return;
  
  if (clicked_button == horizontal_asymmetric_.get()) {
    warp_modifier_->setHorizontalAsymmetric(horizontal_asymmetric_->getToggleState());
    notifyChanged(true);
  }
  else if (clicked_button == vertical_asymmetric_.get()) {
    warp_modifier_->setVerticalAsymmetric(vertical_asymmetric_->getToggleState());
    notifyChanged(true);
  }
}
