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

#include "synth_lfo.h"

#include "common.h"
#include "line_generator.h"
#include "synth_constants.h"
#include "utils.h"

#include <cmath>

namespace vital {
  SynthLfo::SynthLfo(LineGenerator* source) : Processor(kNumInputs, kNumOutputs), source_(source) {
    was_control_rate_ = true;
    sync_seconds_ = std::make_shared<double>();
    *sync_seconds_ = 0;

    trigger_sample_ = 0;
  }

  force_inline void SynthLfo::processTrigger() {
    int sync_type = input(kSyncType)->at(0)[0];
    poly_mask reset_mask = getResetMask(kNoteTrigger);
    poly_mask release_mask = getReleaseMask();
    held_mask_ = (held_mask_ | reset_mask) & ~release_mask;
    poly_int trigger_offset = input(kNoteTrigger)->source->trigger_offset;
    trigger_sample_ = utils::maskLoad(trigger_sample_, trigger_offset, (reset_mask | release_mask));

    control_rate_state_.delay_time_passed = utils::maskLoad(control_rate_state_.delay_time_passed, 0.0f, reset_mask);
    control_rate_state_.fade_amplitude = utils::maskLoad(control_rate_state_.fade_amplitude, 0.0f, reset_mask);
    control_rate_state_.smooth_value = utils::maskLoad(control_rate_state_.smooth_value, 0.0f, reset_mask);
    audio_rate_state_.delay_time_passed = utils::maskLoad(audio_rate_state_.delay_time_passed, 0.0f, reset_mask);
    audio_rate_state_.fade_amplitude = utils::maskLoad(audio_rate_state_.fade_amplitude, 0.0f, reset_mask);
    audio_rate_state_.smooth_value = utils::maskLoad(audio_rate_state_.smooth_value, 0.0f, reset_mask);

    poly_float trigger_delay = utils::toFloat(trigger_offset) * (1.0f / getSampleRate());
    trigger_delay_ = utils::maskLoad(trigger_delay_, trigger_delay, reset_mask);

    if (reset_mask.anyMask()) {
      poly_float frequency = input(kFrequency)->at(0);

      if (sync_type == kSync) {
        poly_float sync_phase = utils::getCycleOffsetFromSeconds(*sync_seconds_, frequency);
        control_rate_state_.offset = utils::maskLoad(control_rate_state_.offset, sync_phase, reset_mask);
        audio_rate_state_.offset = utils::maskLoad(audio_rate_state_.offset, sync_phase, reset_mask);
      }
      else {
        control_rate_state_.offset = utils::maskLoad(control_rate_state_.offset, 0.0f, reset_mask);

        poly_float sample_offset = utils::toFloat(trigger_offset) & reset_mask;
        poly_float offset_start = frequency * sample_offset * (1.0f / getSampleRate());
        audio_rate_state_.offset = utils::maskLoad(audio_rate_state_.offset, -offset_start, reset_mask);
      }
    }
  }

  void SynthLfo::processControlRate(int num_samples) {
    poly_float delay_time = input(kDelay)->at(0);

    float tick_time = 1.0f / getSampleRate();
    poly_float time_passed = tick_time * num_samples;
    control_rate_state_.delay_time_passed += time_passed;
    time_passed = utils::clamp(control_rate_state_.delay_time_passed - delay_time, 0.0f, time_passed);

    poly_float stereo_phase = input(kStereoPhase)->at(0);
    poly_float phase = input(kPhase)->at(0) + stereo_phase * poly_float(0.5f, -0.5f);
    poly_float frequency = input(kFrequency)->at(0);
    poly_float current_offset = control_rate_state_.offset;
    control_rate_state_.offset += frequency * time_passed;

    poly_float phased_offset;
    int sync_type = input(kSyncType)->at(0)[0];
    if (sync_type == kEnvelope) {
      control_rate_state_.offset = utils::min(control_rate_state_.offset, 1.0f);
      phased_offset = utils::min(current_offset + phase, 1.0f);
    }
    else if (sync_type == kSustainEnvelope) {
      control_rate_state_.offset = utils::min(control_rate_state_.offset, utils::maskLoad(1.0f, phase, held_mask_));
      phased_offset = current_offset;
    }
    else if (sync_type == kTrigger || sync_type == kSync) {
      control_rate_state_.offset = utils::mod(control_rate_state_.offset);
      phased_offset = utils::mod(current_offset + phase);
    }
    else if (sync_type == kLoopPoint) {
      poly_mask over = poly_float::greaterThanOrEqual(control_rate_state_.offset, 1.0f);
      control_rate_state_.offset = utils::maskLoad(control_rate_state_.offset,
                                                   control_rate_state_.offset - 1.0f + phase, over);
      phased_offset = utils::min(current_offset, 1.0f);
    }
    else if (sync_type == kLoopHold) {
      poly_mask over = held_mask_ & poly_float::greaterThanOrEqual(control_rate_state_.offset, phase);
      control_rate_state_.offset = utils::min(utils::maskLoad(control_rate_state_.offset,
                                                              control_rate_state_.offset - phase, over), 1.0f);
      phased_offset = utils::maskLoad(current_offset, utils::min(current_offset, phase), held_mask_);
    }

    poly_float fade_time = input(kFade)->at(0);
    poly_float fade_increase = time_passed / utils::max(utils::max(tick_time, time_passed), fade_time);
    control_rate_state_.fade_amplitude = utils::min(control_rate_state_.fade_amplitude + fade_increase, 1.0f);
    control_rate_state_.fade_amplitude = utils::maskLoad(control_rate_state_.fade_amplitude,
                                                         1.0f, poly_float::equal(fade_time, 0.0f));

    poly_float value = getValueAtPhase(phased_offset);
    poly_float result;
    if (input(kSmoothMode)->at(0)[0]) {
      poly_float half_life = input(kSmoothTime)->at(0) * kHalfLifeRatio;
      poly_mask smooth_mask = poly_float::greaterThan(half_life, kMinHalfLife);
      poly_float exponent = -time_passed / utils::max(half_life, kMinHalfLife);
      poly_float ratio = futils::exp2(exponent);
      ratio = ratio & smooth_mask;
      result = utils::interpolate(value, control_rate_state_.smooth_value, ratio);
      control_rate_state_.smooth_value = result;
    }
    else {
      poly_float start_value = getValueAtPhase(phase);
      result = utils::interpolate(start_value, value, control_rate_state_.fade_amplitude);
    }

    result = utils::clamp(result, -1.0f, 1.0f);
    output(kValue)->trigger_value = result;
    if (isControlRate())
      output(kValue)->buffer[0] = result;
    output(kOscPhase)->buffer[0] = utils::encodePhaseAndVoice(phased_offset, input(kNoteCount)->at(0));
    output(kOscFrequency)->buffer[0] = frequency;
  }

  poly_float SynthLfo::processAudioRateEnvelope(int num_samples, poly_float current_phase,
                                                poly_float current_offset, poly_float delta_offset) {
    int lfo_resolution = source_->resolution();
    poly_float resolution = lfo_resolution;
    poly_int max_index = lfo_resolution - 1;
    mono_float* lfo_buffer = source_->getCubicInterpolationBuffer();
    poly_float delta_phase = (audio_rate_state_.phase - current_phase) * (1.0f / num_samples);

    poly_float fade_time = input(kFade)->at(0);
    poly_float delay_time = input(kDelay)->at(0) + trigger_delay_;
    poly_float delay_time_passed = audio_rate_state_.delay_time_passed;
    poly_float current_amplitude = audio_rate_state_.fade_amplitude;
    poly_float tick_time = 1.0f / getSampleRate();
    poly_float fade_increase = tick_time / utils::max(tick_time, fade_time);

    poly_float smooth_mult = 0.0f;
    if (input(kSmoothMode)->at(0)[0]) {
      poly_float half_life = input(kSmoothTime)->at(0) * kHalfLifeRatio;
      poly_mask smooth_mask = poly_float::greaterThan(half_life, kMinHalfLife);
      poly_float exponent = poly_float(-tick_time) / utils::max(half_life, kMinHalfLife);
      smooth_mult = futils::exp2(exponent);
      smooth_mult = smooth_mult & smooth_mask;
      current_amplitude = 1.0f;
    }

    poly_float* dest = output(kValue)->buffer;
    poly_float phased_offset = 0.0f;
    poly_float current_value = audio_rate_state_.smooth_value;

    for (int i = 0; i < num_samples; ++i) {
      delay_time_passed += tick_time;
      poly_mask past_delay_mask = poly_float::greaterThanOrEqual(delay_time_passed, delay_time);
      current_amplitude = utils::clamp(current_amplitude + (fade_increase & past_delay_mask), 0.0f, 1.0f);
      phased_offset = utils::min(current_offset + current_phase, 1.0f);
      poly_float value = getValueAtPhase(lfo_buffer, resolution, max_index, phased_offset);
      current_value = utils::interpolate(value, current_value, smooth_mult);
      dest[i] = current_amplitude * current_value;

      current_offset = utils::min(current_offset + (delta_offset & past_delay_mask), 1.0f);
      current_phase += delta_phase;
    }

    audio_rate_state_.smooth_value = current_value;
    audio_rate_state_.fade_amplitude = current_amplitude;
    audio_rate_state_.delay_time_passed = delay_time_passed;
    audio_rate_state_.offset = utils::min(current_offset, 1.0f);
    return phased_offset;
  }

  poly_float SynthLfo::processAudioRateSustainEnvelope(int num_samples, poly_float current_phase,
                                                       poly_float current_offset, poly_float delta_offset) {
    int lfo_resolution = source_->resolution();
    poly_float resolution = lfo_resolution;
    poly_int max_index = lfo_resolution - 1;
    mono_float* lfo_buffer = source_->getCubicInterpolationBuffer();
    poly_float delta_phase = (audio_rate_state_.phase - current_phase) * (1.0f / num_samples);

    poly_float fade_time = input(kFade)->at(0);
    poly_float delay_time = input(kDelay)->at(0) + trigger_delay_;
    poly_float delay_time_passed = audio_rate_state_.delay_time_passed;
    poly_float current_amplitude = audio_rate_state_.fade_amplitude;
    poly_float tick_time = 1.0f / getSampleRate();
    poly_float fade_increase = tick_time / utils::max(tick_time, fade_time);

    poly_float smooth_mult = 0.0f;
    if (input(kSmoothMode)->at(0)[0]) {
      poly_float half_life = input(kSmoothTime)->at(0) * kHalfLifeRatio;
      poly_mask smooth_mask = poly_float::greaterThan(half_life, kMinHalfLife);
      poly_float exponent = poly_float(-tick_time) / utils::max(half_life, kMinHalfLife);
      smooth_mult = futils::exp2(exponent);
      smooth_mult = smooth_mult & smooth_mask;
      current_amplitude = 1.0f;
    }

    poly_float* dest = output(kValue)->buffer;

    poly_mask current_hold_mask;
    poly_mask held_mask = held_mask_;
    poly_int trigger_sample = trigger_sample_;
    poly_float current_value = audio_rate_state_.smooth_value;

    for (int i = 0; i < num_samples; ++i) {
      delay_time_passed += tick_time;
      poly_mask past_delay_mask = poly_float::greaterThanOrEqual(delay_time_passed, delay_time);
      current_amplitude = utils::clamp(current_amplitude + (fade_increase & past_delay_mask), 0.0f, 1.0f);

      current_hold_mask = utils::maskLoad(current_hold_mask, held_mask, poly_int::equal(i, trigger_sample));
      poly_float max = utils::maskLoad(1.0f, current_phase, current_hold_mask);
      poly_float value = getValueAtPhase(lfo_buffer, resolution, max_index, current_offset);
      current_value = utils::interpolate(value, current_value, smooth_mult);
      dest[i] = current_amplitude * current_value;

      current_offset = utils::min(current_offset + (delta_offset & past_delay_mask), max);
      current_phase += delta_phase;
    }

    audio_rate_state_.smooth_value = current_value;
    audio_rate_state_.fade_amplitude = current_amplitude;
    audio_rate_state_.delay_time_passed = delay_time_passed;
    poly_float last_max = utils::maskLoad(1.0f, current_phase, current_hold_mask);
    audio_rate_state_.offset = utils::min(current_offset, last_max);
    return current_offset;
  }

  poly_float SynthLfo::processAudioRateLfo(int num_samples, poly_float current_phase,
                                           poly_float current_offset, poly_float delta_offset) {
    int lfo_resolution = source_->resolution();
    poly_float resolution = lfo_resolution;
    poly_int max_index = lfo_resolution - 1;
    mono_float* lfo_buffer = source_->getCubicInterpolationBuffer();
    poly_float delta_phase = (audio_rate_state_.phase - current_phase) * (1.0f / num_samples);

    poly_float fade_time = input(kFade)->at(0);
    poly_float delay_time = input(kDelay)->at(0) + trigger_delay_;
    poly_float delay_time_passed = audio_rate_state_.delay_time_passed;
    poly_float current_amplitude = audio_rate_state_.fade_amplitude;
    poly_float tick_time = 1.0f / getSampleRate();
    poly_float fade_increase = tick_time / utils::max(tick_time, fade_time);

    poly_float smooth_mult = 0.0f;
    if (input(kSmoothMode)->at(0)[0]) {
      poly_float half_life = input(kSmoothTime)->at(0) * kHalfLifeRatio;
      poly_mask smooth_mask = poly_float::greaterThan(half_life, kMinHalfLife);
      poly_float exponent = poly_float(-tick_time) / utils::max(half_life, kMinHalfLife);
      smooth_mult = futils::exp2(exponent);
      smooth_mult = smooth_mult & smooth_mask;
      current_amplitude = 1.0f;
    }

    poly_mask delaying_mask = poly_float::greaterThan(delay_time, delay_time_passed);
    poly_float* dest = output(kValue)->buffer;
    poly_float phased_offset = 0.0f;
    poly_float current_value = audio_rate_state_.smooth_value;

    for (int i = 0; i < num_samples; ++i) {
      delay_time_passed += tick_time;
      poly_mask past_delay_mask = poly_float::greaterThanOrEqual(delay_time_passed, delay_time);
      current_amplitude = utils::clamp(current_amplitude + (fade_increase & past_delay_mask), 0.0f, 1.0f);

      phased_offset = utils::mod(current_offset + current_phase);
      poly_float value = getValueAtPhase(lfo_buffer, resolution, max_index, phased_offset);
      current_value = utils::interpolate(value, current_value, smooth_mult);
      dest[i] = current_amplitude * current_value;

      current_offset = utils::mod(current_offset + (delta_offset & past_delay_mask));
      current_phase += delta_phase;
    }

    audio_rate_state_.smooth_value = current_value;
    audio_rate_state_.fade_amplitude = current_amplitude;
    audio_rate_state_.delay_time_passed = delay_time_passed;
    poly_float undelayed_offset = utils::mod(audio_rate_state_.offset + delta_offset * num_samples);
    audio_rate_state_.offset = utils::maskLoad(undelayed_offset, current_offset, delaying_mask);
    return phased_offset;
  }

  poly_float SynthLfo::processAudioRateLoopPoint(int num_samples, poly_float current_phase,
                                                 poly_float current_offset, poly_float delta_offset) {
    int lfo_resolution = source_->resolution();
    poly_float resolution = lfo_resolution;
    poly_int max_index = lfo_resolution - 1;
    mono_float* lfo_buffer = source_->getCubicInterpolationBuffer();
    poly_float delta_phase = (audio_rate_state_.phase - current_phase) * (1.0f / num_samples);

    poly_float fade_time = input(kFade)->at(0);
    poly_float delay_time = input(kDelay)->at(0) + trigger_delay_;
    poly_float delay_time_passed = audio_rate_state_.delay_time_passed;
    poly_float current_amplitude = audio_rate_state_.fade_amplitude;
    poly_float tick_time = 1.0f / getSampleRate();
    poly_float fade_increase = tick_time / utils::max(tick_time, fade_time);

    poly_float smooth_mult = 0.0f;
    if (input(kSmoothMode)->at(0)[0]) {
      poly_float half_life = input(kSmoothTime)->at(0) * kHalfLifeRatio;
      poly_mask smooth_mask = poly_float::greaterThan(half_life, kMinHalfLife);
      poly_float exponent = poly_float(-tick_time) / utils::max(half_life, kMinHalfLife);
      smooth_mult = futils::exp2(exponent);
      smooth_mult = smooth_mult & smooth_mask;
      current_amplitude = 1.0f;
    }

    poly_float* dest = output(kValue)->buffer;
    poly_float current_value = audio_rate_state_.smooth_value;
    
    for (int i = 0; i < num_samples; ++i) {
      delay_time_passed += tick_time;
      poly_mask past_delay_mask = poly_float::greaterThanOrEqual(delay_time_passed, delay_time);
      current_amplitude = utils::clamp(current_amplitude + (fade_increase & past_delay_mask), 0.0f, 1.0f);

      current_offset += delta_offset & past_delay_mask;
      poly_mask over = poly_float::greaterThanOrEqual(current_offset, 1.0f);
      current_offset = utils::min(utils::maskLoad(current_offset, current_offset - 1.0f + current_phase, over), 1.0f);
      poly_float value = getValueAtPhase(lfo_buffer, resolution, max_index, current_offset);
      current_value = utils::interpolate(value, current_value, smooth_mult);
      dest[i] = current_amplitude * current_value;
      current_phase += delta_phase;
    }

    audio_rate_state_.smooth_value = current_value;
    audio_rate_state_.fade_amplitude = current_amplitude;
    audio_rate_state_.delay_time_passed = delay_time_passed;
    audio_rate_state_.offset = current_offset;
    return current_offset;
  }

  poly_float SynthLfo::processAudioRateLoopHold(int num_samples, poly_float current_phase,
                                                poly_float current_offset, poly_float delta_offset) {
    int lfo_resolution = source_->resolution();
    poly_float resolution = lfo_resolution;
    poly_int max_index = lfo_resolution - 1;
    mono_float* lfo_buffer = source_->getCubicInterpolationBuffer();
    poly_float delta_phase = (audio_rate_state_.phase - current_phase) * (1.0f / num_samples);

    poly_float fade_time = input(kFade)->at(0);
    poly_float delay_time = input(kDelay)->at(0) + trigger_delay_;
    poly_float delay_time_passed = audio_rate_state_.delay_time_passed;
    poly_float current_amplitude = audio_rate_state_.fade_amplitude;
    poly_float tick_time = 1.0f / getSampleRate();
    poly_float fade_increase = tick_time / utils::max(tick_time, fade_time);

    poly_float smooth_mult = 0.0f;
    if (input(kSmoothMode)->at(0)[0]) {
      poly_float half_life = input(kSmoothTime)->at(0) * kHalfLifeRatio;
      poly_mask smooth_mask = poly_float::greaterThan(half_life, kMinHalfLife);
      poly_float exponent = poly_float(-tick_time) / utils::max(half_life, kMinHalfLife);
      smooth_mult = futils::exp2(exponent);
      smooth_mult = smooth_mult & smooth_mask;
      current_amplitude = 1.0f;
    }

    poly_float* dest = output(kValue)->buffer;
    poly_mask held_mask = held_mask_;
    poly_float current_value = audio_rate_state_.smooth_value;
    
    for (int i = 0; i < num_samples; ++i) {
      delay_time_passed += tick_time;
      poly_mask past_delay_mask = poly_float::greaterThanOrEqual(delay_time_passed, delay_time);
      current_amplitude = utils::clamp(current_amplitude + (fade_increase & past_delay_mask), 0.0f, 1.0f);

      current_offset += delta_offset & past_delay_mask;
      poly_mask over = held_mask & poly_float::greaterThanOrEqual(current_offset, current_phase);
      current_offset = utils::min(utils::maskLoad(current_offset, current_offset - current_phase, over), 1.0f);
      poly_float value = getValueAtPhase(lfo_buffer, resolution, max_index, current_offset);
      current_value = utils::interpolate(value, current_value, smooth_mult);
      dest[i] = current_amplitude * current_value;
      current_phase += delta_phase;
    }

    audio_rate_state_.smooth_value = current_value;
    audio_rate_state_.fade_amplitude = current_amplitude;
    audio_rate_state_.delay_time_passed = delay_time_passed;
    audio_rate_state_.offset = current_offset;
    return current_offset;
  }

  void SynthLfo::processAudioRate(int num_samples) {
    poly_float fade_time = input(kFade)->at(0);
    poly_float delay_time = input(kDelay)->at(0);

    poly_float stereo_phase = input(kStereoPhase)->at(0);
    poly_float current_phase = audio_rate_state_.phase;
    audio_rate_state_.phase = input(kPhase)->at(0) + stereo_phase * poly_float(0.5f, -0.5f);

    int sync_type = input(kSyncType)->at(0)[0];
    if (sync_type == kSustainEnvelope)
      current_phase = utils::maskLoad(current_phase, 0.0f, getResetMask(kNoteTrigger));
    else
      current_phase = utils::maskLoad(current_phase, audio_rate_state_.phase, getResetMask(kNoteTrigger));

    poly_float frequency = input(kFrequency)->at(0);
    float tick_time = 1.0f / getSampleRate();
    poly_float delta_offset = frequency * tick_time;

    poly_float output_phase = 0.0f;
    poly_float offset = utils::max(0.0f, audio_rate_state_.offset);
    if (sync_type == kEnvelope)
      output_phase = processAudioRateEnvelope(num_samples, current_phase, offset, delta_offset);
    else if (sync_type == kSustainEnvelope)
      output_phase = processAudioRateSustainEnvelope(num_samples, current_phase, offset, delta_offset);
    else if (sync_type == kTrigger || sync_type == kSync)
      output_phase = processAudioRateLfo(num_samples, current_phase, offset, delta_offset);
    else if (sync_type == kLoopPoint)
      output_phase = processAudioRateLoopPoint(num_samples, current_phase, offset, delta_offset);
    else if (sync_type == kLoopHold)
      output_phase = processAudioRateLoopHold(num_samples, current_phase, offset, delta_offset);

    output(kOscPhase)->buffer[0] = utils::encodePhaseAndVoice(output_phase, input(kNoteCount)->at(0));
    output(kOscFrequency)->buffer[0] = frequency;
  }

  void SynthLfo::process(int num_samples) {
    bool control_rate = isControlRate();
    if (was_control_rate_ && !control_rate)
      audio_rate_state_ = control_rate_state_;
    was_control_rate_ = control_rate;

    processTrigger();

    if (!control_rate)
      processAudioRate(num_samples);
    processControlRate(num_samples);
  }

  void SynthLfo::correctToTime(double seconds) {
    *sync_seconds_ = seconds;
  }
} // namespace vital
