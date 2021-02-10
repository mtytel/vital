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

#include "frequency_filter_modifier.h"

#include "futils.h"
#include "utils.h"
#include "wave_frame.h"

namespace {
  constexpr float kMinPower = -9.0f;
  constexpr float kMaxPower = 9.0f;
  constexpr int kMaxSlopeReach = 128;

  force_inline double powerScale(double value, double power) {
    static constexpr float kMinPower = 0.01f;
    if (fabs(power) < kMinPower)
      return value;

    double abs_value = fabs(value);

    double numerator = exp(power * abs_value) - 1.0f;
    double denominator = exp(power) - 1.0f;
    if (value >= 0.0f)
      return numerator / denominator;
    return -numerator / denominator;
  }

  force_inline float combWave(float t, float power) {
    float range = t - floorf(t);
    return 2.0f * powerScale(1.0f - fabsf(2.0f * range - 1.0f), power);
  }
} // namespace

FrequencyFilterModifier::FrequencyFilterModifierKeyframe::FrequencyFilterModifierKeyframe() {
  cutoff_ = 4.0f;
  shape_ = 0.5f;
  style_ = kLowPass;
  normalize_ = true;
}

void FrequencyFilterModifier::FrequencyFilterModifierKeyframe::copy(const WavetableKeyframe* keyframe) {
  const FrequencyFilterModifierKeyframe* source = dynamic_cast<const FrequencyFilterModifierKeyframe*>(keyframe);
  shape_ = source->shape_;
  cutoff_ = source->cutoff_;
}

void FrequencyFilterModifier::FrequencyFilterModifierKeyframe::interpolate(const WavetableKeyframe* from_keyframe,
                                                                           const WavetableKeyframe* to_keyframe,
                                                                           float t) {
  const FrequencyFilterModifierKeyframe* from = dynamic_cast<const FrequencyFilterModifierKeyframe*>(from_keyframe);
  const FrequencyFilterModifierKeyframe* to = dynamic_cast<const FrequencyFilterModifierKeyframe*>(to_keyframe);

  shape_ = linearTween(from->shape_, to->shape_, t);
  cutoff_ = linearTween(from->cutoff_, to->cutoff_, t);
}

void FrequencyFilterModifier::FrequencyFilterModifierKeyframe::render(vital::WaveFrame* wave_frame) {
  for (int i = 0; i < vital::WaveFrame::kNumRealComplex; ++i)
    wave_frame->frequency_domain[i] *= getMultiplier(i);

  wave_frame->toTimeDomain();

  if (normalize_) {
    wave_frame->normalize(true);
    wave_frame->toFrequencyDomain();
  }
}

json FrequencyFilterModifier::FrequencyFilterModifierKeyframe::stateToJson() {
  json data = WavetableKeyframe::stateToJson();
  data["cutoff"] = cutoff_;
  data["shape"] = shape_;
  return data;
}

void FrequencyFilterModifier::FrequencyFilterModifierKeyframe::jsonToState(json data) {
  WavetableKeyframe::jsonToState(data);
  cutoff_ = data["cutoff"];
  shape_ = data["shape"];
}

float FrequencyFilterModifier::FrequencyFilterModifierKeyframe::getMultiplier(float index) {
  float cutoff_index = std::pow(2.0f, cutoff_);
  float cutoff_delta = index - cutoff_index;

  float slope = 1.0f / vital::utils::interpolate(1.0f, kMaxSlopeReach, shape_ * shape_);
  float power = vital::utils::interpolate(kMinPower, kMaxPower, shape_);

  if (style_ == kLowPass)
    return vital::utils::clamp(1.0f - slope * cutoff_delta, 0.0f, 1.0f);
  if (style_ == kBandPass)
    return vital::utils::clamp(1.0f - fabsf(slope * cutoff_delta), 0.0f, 1.0f);
  if (style_ == kHighPass)
    return vital::utils::clamp(1.0f + slope * cutoff_delta, 0.0f, 1.0f);
  if (style_ == kComb)
    return combWave(index / (cutoff_index * 2.0f), power);

  return 0.0f;
}

WavetableKeyframe* FrequencyFilterModifier::createKeyframe(int position) {
  FrequencyFilterModifierKeyframe* keyframe = new FrequencyFilterModifierKeyframe();
  interpolate(keyframe, position);
  return keyframe;
}

void FrequencyFilterModifier::render(vital::WaveFrame* wave_frame, float position) {
  interpolate(&compute_frame_, position);
  compute_frame_.setStyle(style_);
  compute_frame_.setNormalize(normalize_);
  compute_frame_.render(wave_frame);
}

WavetableComponentFactory::ComponentType FrequencyFilterModifier::getType() {
  return WavetableComponentFactory::kFrequencyFilter;
}

json FrequencyFilterModifier::stateToJson() {
  json data = WavetableComponent::stateToJson();
  data["style"] = style_;
  data["normalize"] = normalize_;
  return data;
}

void FrequencyFilterModifier::jsonToState(json data) {
  WavetableComponent::jsonToState(data);
  style_ = data["style"];
  normalize_ = data["normalize"];
}

FrequencyFilterModifier::FrequencyFilterModifierKeyframe* FrequencyFilterModifier::getKeyframe(int index) {
  WavetableKeyframe* wavetable_keyframe = keyframes_[index].get();
  return dynamic_cast<FrequencyFilterModifier::FrequencyFilterModifierKeyframe*>(wavetable_keyframe);
}
