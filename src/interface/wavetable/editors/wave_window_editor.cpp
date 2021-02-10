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

#include "wave_window_editor.h"

#include "skin.h"
#include "synth_constants.h"
#include "shaders.h"
#include "utils.h"

WaveWindowEditor::WaveWindowEditor() : OpenGlLineRenderer(kTotalPoints), edit_bars_(4, Shaders::kColorFragment) {
  edit_bars_.setTargetComponent(this);

  hovering_ = kNone;
  editing_ = kNone;
  left_position_ = 0.0f;
  right_position_ = 1.0f;
  window_shape_ = WaveWindowModifier::kCos;

  setPoints();
}

WaveWindowEditor::~WaveWindowEditor() { }

void WaveWindowEditor::resized() {
  OpenGlLineRenderer::resized();
  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setPoints();

  edit_bars_.setColor(findColour(Skin::kLightenScreen, true).withMultipliedAlpha(0.5f));
}

void WaveWindowEditor::mouseDown(const MouseEvent& e) {
  OpenGlLineRenderer::mouseDown(e);

  editing_ = getHover(e.getPosition());
  changeValues(e);
}

void WaveWindowEditor::mouseUp(const MouseEvent& e) {
  OpenGlLineRenderer::mouseUp(e);

  if (editing_ != kNone)
    notifyWindowChanged(true);

  editing_ = kNone;
  setEditingQuads();
}

void WaveWindowEditor::mouseMove(const MouseEvent& e) {
  OpenGlLineRenderer::mouseMove(e);

  WaveWindowEditor::ActiveSide hover = getHover(e.getPosition());
  if (hovering_ != hover) {
    hovering_ = hover;
    setEditingQuads();
  }
}

void WaveWindowEditor::mouseExit(const MouseEvent& e) {
  OpenGlLineRenderer::mouseExit(e);
  hovering_ = kNone;
  setEditingQuads();
}

void WaveWindowEditor::mouseDrag(const MouseEvent& e) {
  OpenGlLineRenderer::mouseDrag(e);

  changeValues(e);
}

void WaveWindowEditor::notifyWindowChanged(bool mouse_up) {
  for (Listener* listener : listeners_)
    listener->windowChanged(editing_ == kLeft, mouse_up);
}

void WaveWindowEditor::changeValues(const MouseEvent& e) {
  if (editing_ == kNone)
    return;

  float position = (1.0f * e.getPosition().x) / getWidth();
  if (editing_ == kLeft)
    left_position_ = vital::utils::clamp(position, 0.0f, right_position_);
  else if (editing_ == kRight)
    right_position_ = vital::utils::clamp(position, left_position_, 1.0f);

  notifyWindowChanged(false);
  setPoints();
}

void WaveWindowEditor::setEditingQuads() {
  if (editing_ == kLeft)
    edit_bars_.setQuad(2, edit_bars_.getQuadX(0), -1.0f, 2.0f * kGrabRadius, 2.0f);
  else if (editing_ == kRight)
    edit_bars_.setQuad(2, edit_bars_.getQuadX(1), -1.0f, 2.0f * kGrabRadius, 2.0f);
  else
    edit_bars_.setQuad(2, -2.0f, -2.0f, 0.0f, 0.0f);

  if (hovering_ == kLeft)
    edit_bars_.setQuad(3, edit_bars_.getQuadX(0), -1.0f, 2.0f * kGrabRadius, 2.0f);
  else if (hovering_ == kRight)
    edit_bars_.setQuad(3, edit_bars_.getQuadX(1), -1.0f, 2.0f * kGrabRadius, 2.0f);
  else
    edit_bars_.setQuad(3, -2.0f, -2.0f, 0.0f, 0.0f);
}

void WaveWindowEditor::setPoints() {
  edit_bars_.setQuad(0, left_position_ * 2.0f - 1.0f - kGrabRadius, -1.0f, 2.0f * kGrabRadius, 2.0f);
  edit_bars_.setQuad(1, right_position_ * 2.0f - 1.0f - kGrabRadius, -1.0f, 2.0f * kGrabRadius, 2.0f);
  setEditingQuads();

  float width = getWidth();
  float half_height = 0.5f * getHeight();
  float left_gl_x = left_position_ * width;
  for (int i = 0; i < kPointsPerSection; ++i) {
    float t = i / (kPointsPerSection - 1.0f);
    float x = vital::utils::interpolate(0.0f, left_gl_x, t);
    float y = WaveWindowModifier::applyWindow(window_shape_, t) * half_height;

    setXAt(i, x);
    setYAt(i, half_height + y);
    setXAt(kTotalPoints - i - 1, x);
    setYAt(kTotalPoints - i - 1, half_height - y);
  }

  float right_gl_x = right_position_ * width;
  for (int i = 0; i < kPointsPerSection; ++i) {
    float t = i / (kPointsPerSection - 1.0f);
    float x = vital::utils::interpolate(right_gl_x, width, t);
    float y = WaveWindowModifier::applyWindow(window_shape_, 1.0f - t) * half_height;

    int index = kPointsPerSection + i;
    setXAt(index, x);
    setYAt(index, half_height + y);
    setXAt(kTotalPoints - index - 1, x);
    setYAt(kTotalPoints - index - 1, half_height - y);
  }
}

WaveWindowEditor::ActiveSide WaveWindowEditor::getHover(Point<int> position) {
  float window_left_x = left_position_ * getWidth();
  float delta_left = fabsf(window_left_x - position.x);
  float window_right_x = right_position_ * getWidth();
  float delta_right = fabsf(window_right_x - position.x);

  bool choose_left = delta_left < delta_right || (delta_left == delta_right && position.x < window_left_x);
  if (delta_left < kGrabRadius * getWidth() && choose_left)
    return kLeft;
  if (delta_right < kGrabRadius * getWidth())
    return kRight;
  return kNone;
}
