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

#include "wave_line_source_overlay.h"

#include "skin.h"
#include "incrementer_buttons.h"
#include "wave_frame.h"
#include "text_look_and_feel.h"

WaveLineSourceOverlay::WaveLineSourceOverlay() : WavetableComponentOverlay("LINE SOURCE"), line_source_(nullptr) {
  static constexpr int kWaveformSize = vital::WaveFrame::kWaveformSize;
  
  current_frame_ = nullptr;
  default_line_generator_ = std::make_unique<LineGenerator>(kWaveformSize);
  editor_ = std::make_unique<LineEditor>(default_line_generator_.get());
  editor_->setGridSizeX(kDefaultXGrid);
  editor_->setGridSizeY(kDefaultYGrid);
  editor_->addListener(this);
  addOpenGlComponent(editor_.get());
  addOpenGlComponent(editor_->getTextEditorComponent());
  editor_->setVisible(false);
  editor_->setFill(true);
  editor_->setFillCenter(0.0f);
  editor_->setAllowFileLoading(false);

  pull_power_ = std::make_unique<SynthSlider>("wave_line_source_pull_power");
  pull_power_->setValue(0.0f, dontSendNotification);
  addSlider(pull_power_.get());
  pull_power_->setAlwaysOnTop(true);
  pull_power_->getImageComponent()->setAlwaysOnTop(true);
  pull_power_->addListener(this);
  pull_power_->setRange(0, 5.0f);
  pull_power_->setDoubleClickReturnValue(true, 0.0f);
  pull_power_->setLookAndFeel(TextLookAndFeel::instance());
  pull_power_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  horizontal_grid_ = std::make_unique<SynthSlider>("wave_line_source_horizontal_grid");
  horizontal_grid_->setValue(kDefaultXGrid, dontSendNotification);
  addSlider(horizontal_grid_.get());
  horizontal_grid_->setAlwaysOnTop(true);
  horizontal_grid_->getImageComponent()->setAlwaysOnTop(true);
  horizontal_grid_->addListener(this);
  horizontal_grid_->setRange(0, kMaxGrid, 1);
  horizontal_grid_->setDoubleClickReturnValue(true, kDefaultXGrid);
  horizontal_grid_->setLookAndFeel(TextLookAndFeel::instance());
  horizontal_grid_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  horizontal_incrementers_ = std::make_unique<IncrementerButtons>(horizontal_grid_.get());
  addAndMakeVisible(horizontal_incrementers_.get());

  vertical_grid_ = std::make_unique<SynthSlider>("wave_line_source_vertical_grid");
  vertical_grid_->setValue(kDefaultYGrid, dontSendNotification);
  addSlider(vertical_grid_.get());
  vertical_grid_->setAlwaysOnTop(true);
  vertical_grid_->getImageComponent()->setAlwaysOnTop(true);
  vertical_grid_->addListener(this);
  vertical_grid_->setRange(0, kMaxGrid, 1);
  vertical_grid_->setDoubleClickReturnValue(true, kDefaultYGrid);
  vertical_grid_->setLookAndFeel(TextLookAndFeel::instance());
  vertical_grid_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);

  vertical_incrementers_ = std::make_unique<IncrementerButtons>(vertical_grid_.get());
  addAndMakeVisible(vertical_incrementers_.get());

  controls_background_.clearTitles();
  controls_background_.addTitle("PULL POWER");
  controls_background_.addTitle("GRID X");
  controls_background_.addTitle("GRID Y");
}

WaveLineSourceOverlay::~WaveLineSourceOverlay() { }

void WaveLineSourceOverlay::resized() {
  editor_->setColor(findColour(Skin::kWidgetPrimary1, true));
  Colour fill_color = findColour(Skin::kWidgetSecondary1, true);
  Colour fill_color2 = fill_color.withMultipliedAlpha(1.0f - findValue(Skin::kWidgetFillFade));
  editor_->setFillColors(fill_color2, fill_color);
  editor_->setLineWidth(4.0f);
}

void WaveLineSourceOverlay::frameSelected(WavetableKeyframe* keyframe) {
  if (keyframe == nullptr) {
    editor_->setVisible(false);
    editor_->setModel(default_line_generator_.get());
    current_frame_ = nullptr;
    pull_power_->setValue(0.0f, dontSendNotification);
    pull_power_->setEnabled(false);
    pull_power_->redoImage();
  }
  else if (keyframe->owner() == line_source_) {
    editor_->setVisible(true);
    current_frame_ = line_source_->getKeyframe(keyframe->index());
    editor_->setModel(current_frame_->getLineGenerator());
    pull_power_->setValue(current_frame_->getPullPower(), dontSendNotification);
    pull_power_->setEnabled(true);
    pull_power_->redoImage();
  }
}

void WaveLineSourceOverlay::setEditBounds(Rectangle<int> bounds) {
  static constexpr float kPullPowerWidthHeightRatio = 2.0f;
  static constexpr float kGridWidthHeightRatio = 2.0f;

  int padding = getPadding();
  int pull_power_width = bounds.getHeight() * kPullPowerWidthHeightRatio;
  int grid_width = bounds.getHeight() * kGridWidthHeightRatio;
  int total_width = pull_power_width + 2 * grid_width + 2 * padding;
  setControlsWidth(total_width);
  WavetableComponentOverlay::setEditBounds(bounds);

  int x = bounds.getX() + (bounds.getWidth() - total_width) / 2;
  int title_height = WavetableComponentOverlay::kTitleHeightRatio * bounds.getHeight();
  int y = bounds.getY() + title_height;
  int height = bounds.getHeight() - title_height;
  pull_power_->setBounds(x, y, pull_power_width, height);
  horizontal_grid_->setBounds(pull_power_->getRight() + padding, y, grid_width, height);
  vertical_grid_->setBounds(horizontal_grid_->getRight() + padding, y, grid_width, height);

  horizontal_incrementers_->setBounds(horizontal_grid_->getRight() - height, y, height, height);
  vertical_incrementers_->setBounds(vertical_grid_->getRight() - height, y, height, height);

  controls_background_.clearLines();
  controls_background_.addLine(pull_power_width);
  controls_background_.addLine(pull_power_width + grid_width + padding);
  controls_background_.addLine(pull_power_width + 2 * (grid_width + padding));

  pull_power_->redoImage();
  vertical_grid_->redoImage();
  horizontal_grid_->redoImage();
}

bool WaveLineSourceOverlay::setTimeDomainBounds(Rectangle<int> bounds) {
  editor_->setBounds(bounds);
  return true;
}

void WaveLineSourceOverlay::lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) {
  if (wheel.deltaY > 0)
    horizontal_grid_->setValue(horizontal_grid_->getValue() + 1);
  else
    horizontal_grid_->setValue(horizontal_grid_->getValue() - 1);
}

void WaveLineSourceOverlay::fileLoaded() {

}

void WaveLineSourceOverlay::pointChanged(int index, Point<float> position, bool mouse_up) {
  if (current_frame_ == nullptr)
    return;

  notifyChanged(mouse_up);
}

void WaveLineSourceOverlay::powersChanged(bool mouse_up) {
  if (current_frame_ == nullptr)
    return;

  notifyChanged(mouse_up);
}

void WaveLineSourceOverlay::pointAdded(int index, Point<float> position) {
  if (line_source_ == nullptr || current_frame_ == nullptr)
    return;

  int num_points = current_frame_->getNumPoints();
  line_source_->setNumPoints(num_points);
  int num_keyframes = line_source_->numFrames();
  for (int i = 0; i < num_keyframes; ++i) {
    WaveLineSource::WaveLineSourceKeyframe* keyframe = line_source_->getKeyframe(i);
    if (keyframe != current_frame_)
      keyframe->addMiddlePoint(std::min(index, keyframe->getNumPoints() - 1));

    VITAL_ASSERT(keyframe->getNumPoints() == num_points);
  }

  notifyChanged(true);
}

void WaveLineSourceOverlay::pointsAdded(int index, int num_points_added) {
  if (line_source_ == nullptr)
    return;

  int num_points = current_frame_->getNumPoints();
  line_source_->setNumPoints(num_points);
  int num_keyframes = line_source_->numFrames();
  for (int i = 0; i < num_keyframes; ++i) {
    WaveLineSource::WaveLineSourceKeyframe* keyframe = line_source_->getKeyframe(i);

    if (keyframe != current_frame_) {
      for (int p = 0; p < num_points_added; ++p)
        keyframe->addMiddlePoint(index + p);
    }

    VITAL_ASSERT(keyframe->getNumPoints() == num_points);
  }
  notifyChanged(true);
}

void WaveLineSourceOverlay::pointRemoved(int index) {
  if (line_source_ == nullptr)
    return;

  int num_points = current_frame_->getNumPoints();
  line_source_->setNumPoints(num_points);
  int num_keyframes = line_source_->numFrames();
  for (int i = 0; i < num_keyframes; ++i) {
    WaveLineSource::WaveLineSourceKeyframe* keyframe = line_source_->getKeyframe(i);
    if (keyframe != current_frame_)
      keyframe->removePoint(index);

    VITAL_ASSERT(keyframe->getNumPoints() == num_points);
  }
  notifyChanged(true);
}

void WaveLineSourceOverlay::pointsRemoved(int index, int num_points_removed) {
  if (line_source_ == nullptr)
    return;

  int num_points = current_frame_->getNumPoints();
  line_source_->setNumPoints(num_points);
  int num_keyframes = line_source_->numFrames();
  for (int i = 0; i < num_keyframes; ++i) {
    WaveLineSource::WaveLineSourceKeyframe* keyframe = line_source_->getKeyframe(i);

    if (keyframe != current_frame_) {
      for (int p = 0; p < num_points_removed; ++p)
        keyframe->removePoint(index);
    }

    VITAL_ASSERT(keyframe->getNumPoints() == num_points);
  }
  notifyChanged(true);
}

void WaveLineSourceOverlay::sliderValueChanged(Slider* moved_slider) {
  if (line_source_ == nullptr)
    return;
  
  if (moved_slider == horizontal_grid_.get())
    editor_->setGridSizeX(horizontal_grid_->getValue());
  else if (moved_slider == vertical_grid_.get())
    editor_->setGridSizeY(vertical_grid_->getValue());
  else if (moved_slider == pull_power_.get()) {
    if (current_frame_)
      current_frame_->setPullPower(pull_power_->getValue());
  }

  notifyChanged(false);
}

void WaveLineSourceOverlay::sliderDragEnded(Slider* moved_slider) {
  if (moved_slider != horizontal_grid_.get() && moved_slider != vertical_grid_.get())
    notifyChanged(true);
}
