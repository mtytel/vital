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

#include "wave_frame.h"

#include "wave_window_modifier.h"

float WaveWindowModifier::applyWindow(WindowShape window_shape, float t) {
  if (window_shape == kCos)
    return 0.5f - 0.5f * cosf(vital::kPi * t);
  if (window_shape == kHalfSin)
    return sinf(vital::kPi * t / 2.0f);
  if (window_shape == kSquare)
    return t < 1.0f ? 0.0f : 1.0f;
  if (window_shape == kWiggle)
    return t * cosf(vital::kPi * (t * 1.5f + 0.5f));
  return t;
}

WaveWindowModifier::WaveWindowModifierKeyframe::WaveWindowModifierKeyframe() {
  static constexpr float kDefaultOffset = 0.25f;
  left_position_ = kDefaultOffset;
  right_position_ = 1.0f - kDefaultOffset;
  window_shape_ = kCos;
}

void WaveWindowModifier::WaveWindowModifierKeyframe::copy(const WavetableKeyframe* keyframe) {
  const WaveWindowModifierKeyframe* source = dynamic_cast<const WaveWindowModifierKeyframe*>(keyframe);

  left_position_ = source->left_position_;
  right_position_ = source->right_position_;
}

void WaveWindowModifier::WaveWindowModifierKeyframe::interpolate(const WavetableKeyframe* from_keyframe,
                                                                 const WavetableKeyframe* to_keyframe,
                                                                 float t) {
  const WaveWindowModifierKeyframe* from = dynamic_cast<const WaveWindowModifierKeyframe*>(from_keyframe);
  const WaveWindowModifierKeyframe* to = dynamic_cast<const WaveWindowModifierKeyframe*>(to_keyframe);

  left_position_ = linearTween(from->left_position_, to->left_position_, t);
  right_position_ = linearTween(from->right_position_, to->right_position_, t);
}

void WaveWindowModifier::WaveWindowModifierKeyframe::render(vital::WaveFrame* wave_frame) {
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    float t = i / (vital::WaveFrame::kWaveformSize - 1.0f);
    if (t >= left_position_)
      break;

    wave_frame->time_domain[i] *= applyWindow(t / left_position_);
  }

  for (int i = vital::WaveFrame::kWaveformSize; i >= 0; --i) {
    float t = i / (vital::WaveFrame::kWaveformSize - 1.0f);
    if (t <= right_position_)
      break;

    wave_frame->time_domain[i] *= applyWindow((1.0f - t) / (1.0f - right_position_));
  }

  wave_frame->toFrequencyDomain();
}

json WaveWindowModifier::WaveWindowModifierKeyframe::stateToJson() {
  json data = WavetableKeyframe::stateToJson();
  data["left_position"] = left_position_;
  data["right_position"] = right_position_;
  return data;
}

void WaveWindowModifier::WaveWindowModifierKeyframe::jsonToState(json data) {
  WavetableKeyframe::jsonToState(data);
  left_position_ = data["left_position"];
  right_position_ = data["right_position"];
}

WavetableKeyframe* WaveWindowModifier::createKeyframe(int position) {
  WaveWindowModifierKeyframe* keyframe = new WaveWindowModifierKeyframe();
  interpolate(keyframe, position);
  return keyframe;
}

void WaveWindowModifier::render(vital::WaveFrame* wave_frame, float position) {
  interpolate(&compute_frame_, position);
  compute_frame_.setWindowShape(window_shape_);
  compute_frame_.render(wave_frame);
}

WavetableComponentFactory::ComponentType WaveWindowModifier::getType() {
  return WavetableComponentFactory::kWaveWindow;
}

json WaveWindowModifier::stateToJson() {
  json data = WavetableComponent::stateToJson();
  data["window_shape"] = window_shape_;
  return data;
}

void WaveWindowModifier::jsonToState(json data) {
  WavetableComponent::jsonToState(data);
  window_shape_ = data["window_shape"];
}

WaveWindowModifier::WaveWindowModifierKeyframe* WaveWindowModifier::getKeyframe(int index) {
  WavetableKeyframe* wavetable_keyframe = keyframes_[index].get();
  return dynamic_cast<WaveWindowModifier::WaveWindowModifierKeyframe*>(wavetable_keyframe);
}
