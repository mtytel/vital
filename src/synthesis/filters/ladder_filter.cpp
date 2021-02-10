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

#include "ladder_filter.h"

#include "futils.h"

namespace vital {
  LadderFilter::LadderFilter() : Processor(LadderFilter::kNumInputs, 1) {
    hardReset();
  }

  void LadderFilter::reset(poly_mask reset_mask) {
    filter_input_ = utils::maskLoad(filter_input_, 0.0f, reset_mask);

    for (int i = 0; i < kNumStages; ++i)
      stages_[i].reset(reset_mask);
  }

  void LadderFilter::hardReset() {
    reset(constants::kFullMask);
    resonance_ = 0.0f;
    drive_ = 0.0f;
    post_multiply_ = 0.0f;
  }

  void LadderFilter::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));

    poly_float current_resonance = resonance_;
    poly_float current_drive = drive_;
    poly_float current_post_multiply = post_multiply_;

    poly_float current_stage_scales[kNumStages + 1];
    for (int i = 0; i <= kNumStages; ++i)
      current_stage_scales[i] = stage_scales_[i];

    filter_state_.loadSettings(this);
    setupFilter(filter_state_);

    poly_mask reset_mask = getResetMask(kReset);
    if (reset_mask.anyMask()) {
      reset(reset_mask);

      current_resonance = utils::maskLoad(current_resonance, resonance_, reset_mask);
      current_drive = utils::maskLoad(current_drive, drive_, reset_mask);
      current_post_multiply = utils::maskLoad(current_post_multiply, post_multiply_, reset_mask);

      for (int i = 0; i <= kNumStages; ++i)
        current_stage_scales[i] = utils::maskLoad(current_stage_scales[i], stage_scales_[i], reset_mask);
    }

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * tick_increment;
    poly_float delta_stage_scales[kNumStages + 1];
    for (int i = 0; i <= kNumStages; ++i)
      delta_stage_scales[i] = (stage_scales_[i] - current_stage_scales[i]) * tick_increment;

    const poly_float* audio_in = input(kAudio)->source->buffer;
    poly_float* audio_out = output()->buffer;
    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());
    poly_float max_frequency = kMaxCutoff / getSampleRate();

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), max_frequency);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;
      for (int stage = 0; stage <= kNumStages; ++stage)
        current_stage_scales[stage] += delta_stage_scales[stage];

      tick(audio_in[i], coefficient, current_resonance, current_drive);
      poly_float total = current_stage_scales[0] * filter_input_;
      
      for (int stage = 0; stage < kNumStages; ++stage)
        total += current_stage_scales[stage + 1] * stages_[stage].getCurrentState();

      audio_out[i] = total * current_post_multiply;
    }
  }

  void LadderFilter::setupFilter(const FilterState& filter_state) {
    poly_float resonance_percent = utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f);
    poly_float resonance_adjust = resonance_percent;
    if (filter_state.style)
      resonance_adjust = utils::sin(resonance_percent * (0.5f * kPi));

    resonance_ = utils::interpolate(kMinResonance, kMaxResonance, resonance_adjust);
    resonance_ += filter_state.drive_percent * filter_state.resonance_percent * kDriveResonanceBoost;

    setStageScales(filter_state);
  }

  void LadderFilter::setStageScales(const FilterState& filter_state) {
    static const mono_float low_pass24[kNumStages + 1] = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    static const mono_float band_pass24[kNumStages + 1] = { 0.0f, 0.0f, -1.0f, 2.0f, -1.0f };
    static const mono_float high_pass24[kNumStages + 1] = { 1.0f, -4.0f, 6.0f, -4.0f, 1.0f };
    static const mono_float low_pass12[kNumStages + 1] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f };
    static const mono_float band_pass12[kNumStages + 1] = { 0.0f, 1.0f, -1.0f, 0.0f, 0.0f };
    static const mono_float high_pass12[kNumStages + 1] = { 1.0f, -2.0f, 1.0f, 0.0f, 0.0f };

    poly_float blend = utils::clamp(filter_state.pass_blend - 1.0f, -1.0f, 1.0f);
    poly_float band_pass = utils::sqrt(-blend * blend + 1.0f);

    poly_mask blend_mask = poly_float::lessThan(blend, 0.0f);
    poly_float low_pass = (-blend) & blend_mask;
    poly_float high_pass = blend & ~blend_mask;

    poly_float resonance_percent = utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f);
    poly_float drive_mult = resonance_percent + 1.0f;
    if (filter_state.style)
      drive_mult = utils::sin(resonance_percent) + 1.0f;

    poly_float resonance_scale = utils::interpolate(drive_mult, 1.0f, high_pass);
    drive_ = filter_state.drive * resonance_scale;
    post_multiply_ = poly_float(1.0f) / utils::sqrt((filter_state.drive - 1.0f) * 0.5f + 1.0f);

    if (filter_state.style == k12Db) {
      for (int i = 0; i <= kNumStages; ++i)
        stage_scales_[i] = low_pass * low_pass12[i] + band_pass * band_pass12[i] + high_pass * high_pass12[i];
    }
    else if (filter_state.style == k24Db) {
      band_pass = -poly_float::abs(blend) + 1.0f;
      post_multiply_ = poly_float(1.0f) / utils::sqrt((filter_state.drive - 1.0f) * 0.25f + 1.0f);

      for (int i = 0; i <= kNumStages; ++i)
        stage_scales_[i] = (low_pass * low_pass24[i] + band_pass * band_pass24[i] + high_pass * high_pass24[i]);
    }
    else if (filter_state.style == kDualNotchBand) {
      drive_ = filter_state.drive;
      poly_float low_pass_fade = utils::min(blend + 1.0f, 1.0f);
      poly_float high_pass_fade = utils::min(-blend + 1.0f, 1.0f);

      stage_scales_[0] = low_pass_fade;
      stage_scales_[1] = low_pass_fade * -4.0f;
      stage_scales_[2] = high_pass_fade * 4.0f + low_pass_fade * 8.0f;
      stage_scales_[3] = high_pass_fade * -8.0f - low_pass_fade * 8.0f;
      stage_scales_[4] = high_pass_fade * 4.0f + low_pass_fade * 4.0f;
    }
    else if (filter_state.style == kNotchPassSwap) {
      post_multiply_ = poly_float(1.0f) / utils::sqrt((filter_state.drive - 1.0f) * 0.5f + 1.0f);

      poly_float low_pass_fade = utils::min(blend + 1.0f, 1.0f);
      poly_float low_pass_fade2 = low_pass_fade * low_pass_fade;
      poly_float high_pass_fade = utils::min(-blend + 1.0f, 1.0f);
      poly_float high_pass_fade2 = high_pass_fade * high_pass_fade;
      poly_float low_high_pass_fade = low_pass_fade * high_pass_fade;

      stage_scales_[0] = low_pass_fade2;
      stage_scales_[1] = low_pass_fade2 * -4.0f;
      stage_scales_[2] = low_pass_fade2 * 6.0f + low_high_pass_fade * 2.0f;
      stage_scales_[3] = low_pass_fade2 * -4.0f - low_high_pass_fade * 4.0f;
      stage_scales_[4] = low_pass_fade2 + high_pass_fade2 + low_high_pass_fade * 2.0f;
    }
    else if (filter_state.style == kBandPeakNotch) {
      poly_float drive_t = poly_float::min(-blend + 1.0f, 1.0f);
      drive_ = utils::interpolate(filter_state.drive, drive_, drive_t);

      poly_float drive_inv_t = -drive_t + 1.0f;
      poly_float mult = utils::sqrt((drive_inv_t * drive_inv_t) * 0.5f + 0.5f);
      poly_float peak_band_value = -utils::max(-blend, 0.0f);
      poly_float low_high = mult * (peak_band_value + 1.0f);
      poly_float band = mult * (peak_band_value - blend + 1.0f) * 2.0f;

      for (int i = 0; i <= kNumStages; ++i)
        stage_scales_[i] = low_high * low_pass12[i] + band * band_pass12[i] + low_high * high_pass12[i];
    }
  }

  force_inline void LadderFilter::tick(poly_float audio_in, poly_float coefficient,
                                       poly_float resonance, poly_float drive) {
    poly_float g1 = coefficient * kResonanceTuning;
    poly_float g2 = g1 * g1;
    poly_float g3 = g1 * g2;

    poly_float filter_state1 = utils::mulAdd(stages_[3].getNextSatState(), g1, stages_[2].getNextSatState());
    poly_float filter_state2 = utils::mulAdd(filter_state1, g2, stages_[1].getNextSatState());
    poly_float filter_state = utils::mulAdd(filter_state2, g3, stages_[0].getNextSatState());

    poly_float filter_input = (audio_in * drive - resonance * filter_state);
    filter_input_ = futils::tanh(filter_input);

    poly_float stage_out = stages_[0].tick(filter_input_, coefficient);
    stage_out = stages_[1].tick(stage_out, coefficient);
    stage_out = stages_[2].tick(stage_out, coefficient);
    stages_[3].tick(stage_out, coefficient);
  }
} // namespace vital
