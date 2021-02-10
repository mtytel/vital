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

#include "wavetable_playhead.h"

#include "skin.h"

WavetablePlayhead::WavetablePlayhead(int num_positions) :
    SynthSection("Playhead"), position_quad_(Shaders::kColorFragment),
    padding_(0), num_positions_(num_positions), position_(0), drag_start_x_(0) {
  addOpenGlComponent(&position_quad_);
}

void WavetablePlayhead::setPosition(int position) {
  position_ = position;

  for (Listener* listener : listeners_)
    listener->playheadMoved(position_);

  setPositionQuad();
}

void WavetablePlayhead::setPositionQuad() {
  float active_width = getWidth() - 2 * padding_ + 1;
  int x = (active_width * position_) / (num_positions_ - 1) - 0.5f + padding_;
  position_quad_.setBounds(x, 0, 1, getHeight());
}

void WavetablePlayhead::mouseDown(const MouseEvent& event) {
  mouseEvent(event);
}

void WavetablePlayhead::mouseDrag(const MouseEvent& event) {
  mouseEvent(event);
}

void WavetablePlayhead::mouseEvent(const MouseEvent& event) {
  float active_width = getWidth() - 2 * padding_ + 1;
  int position = std::roundf((num_positions_ - 1) * (event.x - padding_) / active_width);
  setPosition(std::max(std::min(position, num_positions_ - 1), 0));
}

void WavetablePlayhead::paintBackground(Graphics& g) {
  float active_width = getWidth() - 2 * padding_ + 1;
  g.setColour(findColour(Skin::kLightenScreen, true));
  int small_line_height = getHeight() / 3;
  int big_line_height = 2 * small_line_height;
  for (int i = 0; i < num_positions_; i += kLineSkip) {
    int x = padding_ - 0.5f + i * active_width / (num_positions_ - 1);
    if (i % kBigLineSkip == 0)
      g.fillRect(x, getHeight() - big_line_height, 1, big_line_height);
    else
      g.fillRect(x, getHeight() - small_line_height, 1, small_line_height);
  }

  position_quad_.setColor(findColour(Skin::kWidgetPrimary1, true));
}

void WavetablePlayhead::resized() {
  SynthSection::resized();
  setPositionQuad();
}
