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

#include "wavetable_component.h"

WavetableKeyframe* WavetableComponent::insertNewKeyframe(int position) {
  VITAL_ASSERT(position >= 0 && position < vital::kNumOscillatorWaveFrames);

  WavetableKeyframe* keyframe = createKeyframe(position);
  keyframe->setOwner(this);
  keyframe->setPosition(position);

  int index = getIndexFromPosition(position);
  keyframes_.insert(keyframes_.begin() + index, std::unique_ptr<WavetableKeyframe>(keyframe));
  return keyframe;
}

void WavetableComponent::reposition(WavetableKeyframe* keyframe) {
  int start_index = indexOf(keyframe);
  keyframes_[start_index].release();
  keyframes_.erase(keyframes_.begin() + start_index);

  int new_index = getIndexFromPosition(keyframe->position());
  keyframes_.insert(keyframes_.begin() + new_index, std::unique_ptr<WavetableKeyframe>(keyframe));
}

void WavetableComponent::remove(WavetableKeyframe* keyframe) {
  int start_index = indexOf(keyframe);
  keyframes_.erase(keyframes_.begin() + start_index);
}

void WavetableComponent::jsonToState(json data) {
  keyframes_.clear();
  for (json json_keyframe : data["keyframes"]) {
    WavetableKeyframe* keyframe = insertNewKeyframe(json_keyframe["position"]);
    keyframe->jsonToState(json_keyframe);
  }

  if (data.count("interpolation_style"))
    interpolation_style_ = data["interpolation_style"];
}

json WavetableComponent::stateToJson() {
  json keyframes_data;
  for (int i = 0; i < keyframes_.size(); ++i)
    keyframes_data.emplace_back(keyframes_[i]->stateToJson());

  return {
    { "keyframes", keyframes_data },
    { "type", WavetableComponentFactory::getComponentName(getType()) },
    { "interpolation_style", interpolation_style_ }
  };
}

void WavetableComponent::reset() {
  keyframes_.clear();
  insertNewKeyframe(0);
}

void WavetableComponent::interpolate(WavetableKeyframe* dest, float position) {
  if (numFrames() == 0)
    return;

  int index = getIndexFromPosition(position) - 1;
  int clamped_index = std::min(std::max(index, 0), numFrames() - 1);
  WavetableKeyframe* from_frame = keyframes_[clamped_index].get();

  if (index < 0 || index >= numFrames() - 1 || interpolation_style_ == kNone)
    dest->copy(from_frame);
  else if (interpolation_style_ == kLinear) {
    WavetableKeyframe* to_frame = keyframes_[index + 1].get();
    int from_position = keyframes_[index]->position();
    int to_position = keyframes_[index + 1]->position();
    float t = (1.0f * position - from_position) / (to_position - from_position);
    dest->interpolate(from_frame, to_frame, t);
  }
  else if (interpolation_style_ == kCubic) {
    int next_index = index + 2;
    int prev_index = index - 1;
    if (next_index >= numFrames())
      next_index = index;
    if (prev_index < 0)
      prev_index = index + 1;
    
    WavetableKeyframe* to_frame = keyframes_[index + 1].get();
    WavetableKeyframe* next_frame = keyframes_[next_index].get();
    WavetableKeyframe* prev_frame = keyframes_[prev_index].get();
    
    int from_position = keyframes_[index]->position();
    int to_position = keyframes_[index + 1]->position();
    float t = (1.0f * position - from_position) / (to_position - from_position);
    dest->smoothInterpolate(prev_frame, from_frame, to_frame, next_frame, t);
  }
}

int WavetableComponent::getIndexFromPosition(int position) const {
  int index = 0;
  for (auto& keyframe : keyframes_) {
    if (position < keyframe->position())
      break;
    index++;
  }

  return index;
}

WavetableKeyframe* WavetableComponent::getFrameAtPosition(int position) {
  int index = getIndexFromPosition(position);
  if (index < 0 || index >= keyframes_.size())
    return nullptr;

  return keyframes_[index].get();
}

int WavetableComponent::getLastKeyframePosition() {
  if (keyframes_.size() == 0)
    return 0;
  if (!hasKeyframes())
    return vital::kNumOscillatorWaveFrames - 1;
  return keyframes_[keyframes_.size() - 1]->position();
}
