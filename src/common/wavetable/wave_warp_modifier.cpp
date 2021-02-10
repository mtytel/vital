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

#include "wave_warp_modifier.h"

#include "futils.h"
#include "wave_frame.h"
#include "utils.h"

namespace {
  inline double highResPowerScale(float value, float power) {
    static constexpr float kMinPower = 0.01f;
    if (fabsf(power) < kMinPower)
      return value;

    double abs_value = fabsf(value);

    double numerator = exp(power * abs_value) - 1.0f;
    double denominator = exp(power) - 1.0f;
    if (value >= 0.0f)
      return numerator / denominator;
    return -numerator / denominator;
  }
}

WaveWarpModifier::WaveWarpModifierKeyframe::WaveWarpModifierKeyframe() {
  horizontal_power_ = 0.0f;
  vertical_power_ = 0.0f;
  horizontal_asymmetric_ = false;
  vertical_asymmetric_ = false;
}

void WaveWarpModifier::WaveWarpModifierKeyframe::copy(const WavetableKeyframe* keyframe) {
  const WaveWarpModifierKeyframe* source = dynamic_cast<const WaveWarpModifierKeyframe*>(keyframe);
  horizontal_power_ = source->horizontal_power_;
  vertical_power_ = source->vertical_power_;
}

void WaveWarpModifier::WaveWarpModifierKeyframe::interpolate(const WavetableKeyframe* from_keyframe,
                                                             const WavetableKeyframe* to_keyframe,
                                                             float t) {
  const WaveWarpModifierKeyframe* from = dynamic_cast<const WaveWarpModifierKeyframe*>(from_keyframe);
  const WaveWarpModifierKeyframe* to = dynamic_cast<const WaveWarpModifierKeyframe*>(to_keyframe);

  horizontal_power_ = linearTween(from->horizontal_power_, to->horizontal_power_, t);
  vertical_power_ = linearTween(from->vertical_power_, to->vertical_power_, t);
}

void WaveWarpModifier::WaveWarpModifierKeyframe::render(vital::WaveFrame* wave_frame) {
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i)
    wave_frame->frequency_domain[i] = wave_frame->time_domain[i];

  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    float horizontal = i / (vital::WaveFrame::kWaveformSize - 1.0f);
    float warped_horizontal = 0.0f;
    if (horizontal_asymmetric_)
      warped_horizontal = highResPowerScale(horizontal, horizontal_power_);
    else
      warped_horizontal = 0.5f * highResPowerScale(2.0f * horizontal - 1.0f, horizontal_power_) + 0.5f;

    float float_index = (vital::WaveFrame::kWaveformSize - 1) * warped_horizontal;
    int index = float_index;
    index = vital::utils::iclamp(index, 0, vital::WaveFrame::kWaveformSize - 2);

    float vertical_from = wave_frame->frequency_domain[index].real();
    float vertical_to = wave_frame->frequency_domain[index + 1].real();
    float vertical = linearTween(vertical_from, vertical_to, float_index - index);
    vertical = vital::utils::clamp(vertical, -1.0f, 1.0f);
    if (vertical_asymmetric_)
      wave_frame->time_domain[i] = 2.0f * highResPowerScale(0.5f * vertical + 0.5f, vertical_power_) - 1.0f;
    else
      wave_frame->time_domain[i] = highResPowerScale(vertical, vertical_power_);
  }
  wave_frame->toFrequencyDomain();
}

json WaveWarpModifier::WaveWarpModifierKeyframe::stateToJson() {
  json data = WavetableKeyframe::stateToJson();
  data["horizontal_power"] = horizontal_power_;
  data["vertical_power"] = vertical_power_;
  return data;
}

void WaveWarpModifier::WaveWarpModifierKeyframe::jsonToState(json data) {
  WavetableKeyframe::jsonToState(data);
  horizontal_power_ = data["horizontal_power"];
  vertical_power_ = data["vertical_power"];
}

WavetableKeyframe* WaveWarpModifier::createKeyframe(int position) {
  WaveWarpModifierKeyframe* keyframe = new WaveWarpModifierKeyframe();
  interpolate(keyframe, position);
  return keyframe;
}

void WaveWarpModifier::render(vital::WaveFrame* wave_frame, float position) {
  interpolate(&compute_frame_, position);
  compute_frame_.setHorizontalAsymmetric(horizontal_asymmetric_);
  compute_frame_.setVerticalAsymmetric(vertical_asymmetric_);
  compute_frame_.render(wave_frame);
}

WavetableComponentFactory::ComponentType WaveWarpModifier::getType() {
  return WavetableComponentFactory::kWaveWarp;
}

json WaveWarpModifier::stateToJson() {
  json data = WavetableComponent::stateToJson();
  data["horizontal_asymmetric"] = horizontal_asymmetric_;
  data["vertical_asymmetric"] = vertical_asymmetric_;
  return data;
}

void WaveWarpModifier::jsonToState(json data) {
  WavetableComponent::jsonToState(data);
  horizontal_asymmetric_ = data["horizontal_asymmetric"];
  vertical_asymmetric_ = data["vertical_asymmetric"];
}

WaveWarpModifier::WaveWarpModifierKeyframe* WaveWarpModifier::getKeyframe(int index) {
  WavetableKeyframe* wavetable_keyframe = keyframes_[index].get();
  return dynamic_cast<WaveWarpModifier::WaveWarpModifierKeyframe*>(wavetable_keyframe);
}
