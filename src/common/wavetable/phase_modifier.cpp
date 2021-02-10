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

#include "phase_modifier.h"
#include "wave_frame.h"
#include "wavetable_component_factory.h"

namespace {
  std::complex<float> multiplyAndMix(std::complex<float> value, std::complex<float> mult, float mix) {
    std::complex<float> result = value * mult;
    return mix * result + (1.0f - mix) * value;
  }
} // namespace

PhaseModifier::PhaseModifierKeyframe::PhaseModifierKeyframe() : phase_(0.0f), mix_(1.0f), phase_style_(kNormal) { }

void PhaseModifier::PhaseModifierKeyframe::copy(const WavetableKeyframe* keyframe) {
  const PhaseModifierKeyframe* source = dynamic_cast<const PhaseModifierKeyframe*>(keyframe);
  phase_ = source->phase_;
  mix_ = source->mix_;
}

void PhaseModifier::PhaseModifierKeyframe::interpolate(const WavetableKeyframe* from_keyframe,
                                                       const WavetableKeyframe* to_keyframe,
                                                       float t) {
  const PhaseModifierKeyframe* from = dynamic_cast<const PhaseModifierKeyframe*>(from_keyframe);
  const PhaseModifierKeyframe* to = dynamic_cast<const PhaseModifierKeyframe*>(to_keyframe);

  phase_ = linearTween(from->phase_, to->phase_, t);
  mix_ = linearTween(from->mix_, to->mix_, t);
}

void PhaseModifier::PhaseModifierKeyframe::render(vital::WaveFrame* wave_frame) {
  std::complex<float> phase_shift = std::polar(1.0f, -phase_);
  if (phase_style_ == kHarmonic) {
    for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i)
      wave_frame->frequency_domain[i] = multiplyAndMix(wave_frame->frequency_domain[i], phase_shift, mix_);
  }
  else if (phase_style_ == kHarmonicEvenOdd) {
    std::complex<float> odd_shift = 1.0f / phase_shift;
    for (int i = 0; i < vital::WaveFrame::kWaveformSize; i += 2) {
      wave_frame->frequency_domain[i] = multiplyAndMix(wave_frame->frequency_domain[i], phase_shift, mix_);
      wave_frame->frequency_domain[i + 1] = multiplyAndMix(wave_frame->frequency_domain[i + 1], odd_shift, mix_);
    }
  }
  else if (phase_style_ == kNormal) {
    std::complex<float> current_phase_shift = 1.0f;

    for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
      wave_frame->frequency_domain[i] = multiplyAndMix(wave_frame->frequency_domain[i], current_phase_shift, mix_);
      current_phase_shift *= phase_shift;
    }
  }
  else if (phase_style_ == kEvenOdd) {
    std::complex<float> current_phase_shift = 1.0f;

    for (int i = 0; i < vital::WaveFrame::kWaveformSize; i += 2) {
      wave_frame->frequency_domain[i] = multiplyAndMix(wave_frame->frequency_domain[i], current_phase_shift, mix_);
      std::complex<float> odd_shift = 1.0f / (current_phase_shift * phase_shift);
      wave_frame->frequency_domain[i + 1] = multiplyAndMix(wave_frame->frequency_domain[i + 1], odd_shift, mix_);
      current_phase_shift *= phase_shift * phase_shift;
    }
  }
  else if (phase_style_ == kClear) {
    for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i)
      wave_frame->frequency_domain[i] = std::abs(wave_frame->frequency_domain[i]);
  }
  wave_frame->toTimeDomain();
}

json PhaseModifier::PhaseModifierKeyframe::stateToJson() {
  json data = WavetableKeyframe::stateToJson();
  data["phase"] = phase_;
  data["mix"] = mix_;
  return data;
}

void PhaseModifier::PhaseModifierKeyframe::jsonToState(json data) {
  WavetableKeyframe::jsonToState(data);
  phase_ = data["phase"];
  mix_ = data["mix"];
}

WavetableKeyframe* PhaseModifier::createKeyframe(int position) {
  PhaseModifierKeyframe* keyframe = new PhaseModifierKeyframe();
  interpolate(keyframe, position);
  return keyframe;
}

void PhaseModifier::render(vital::WaveFrame* wave_frame, float position) {
  compute_frame_.setPhaseStyle(phase_style_);
  interpolate(&compute_frame_, position);
  compute_frame_.render(wave_frame);
}

WavetableComponentFactory::ComponentType PhaseModifier::getType() {
  return WavetableComponentFactory::kPhaseModifier;
}

json PhaseModifier::stateToJson() {
  json data = WavetableComponent::stateToJson();
  data["style"] = phase_style_;
  return data;
}

void PhaseModifier::jsonToState(json data) {
  WavetableComponent::jsonToState(data);
  phase_style_ = data["style"];
}

PhaseModifier::PhaseModifierKeyframe* PhaseModifier::getKeyframe(int index) {
  WavetableKeyframe* wavetable_keyframe = keyframes_[index].get();
  return dynamic_cast<PhaseModifier::PhaseModifierKeyframe*>(wavetable_keyframe);
}
