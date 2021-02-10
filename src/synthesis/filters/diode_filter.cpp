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

#include "diode_filter.h"

#include "futils.h"

namespace vital {
  DiodeFilter::DiodeFilter() : Processor(DiodeFilter::kNumInputs, 1) {
    hardReset();
  }

  void DiodeFilter::reset(poly_mask reset_mask) {
    high_pass_1_.reset(reset_mask);
    high_pass_2_.reset(reset_mask);
    stage1_.reset(reset_mask);
    stage2_.reset(reset_mask);
    stage3_.reset(reset_mask);
    stage4_.reset(reset_mask);
  }

  void DiodeFilter::hardReset() {
    reset(constants::kFullMask);
    resonance_ = 0.0f;
    drive_ = 0.0f;
    post_multiply_ = 0.0f;
  }

  void DiodeFilter::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));

    poly_float current_resonance = resonance_;
    poly_float current_drive = drive_;
    poly_float current_post_multiply = post_multiply_;
    poly_float current_high_pass_ratio = high_pass_ratio_;
    poly_float current_high_pass_amount = high_pass_amount_;

    filter_state_.loadSettings(this);
    setupFilter(filter_state_);

    poly_mask reset_mask = getResetMask(kReset);
    if (reset_mask.anyMask()) {
      reset(reset_mask);

      current_resonance = utils::maskLoad(current_resonance, resonance_, reset_mask);
      current_drive = utils::maskLoad(current_drive, drive_, reset_mask);
      current_post_multiply = utils::maskLoad(current_post_multiply, post_multiply_, reset_mask);
      current_high_pass_ratio = utils::maskLoad(current_high_pass_ratio, high_pass_ratio_, reset_mask);
      current_high_pass_amount = utils::maskLoad(current_high_pass_amount, high_pass_amount_, reset_mask);
    }

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * tick_increment;
    poly_float delta_high_pass_ratio = (high_pass_ratio_ - current_high_pass_ratio) * tick_increment;
    poly_float delta_high_pass_amount = (high_pass_amount_ - current_high_pass_amount) * tick_increment;

    const poly_float* audio_in = input(kAudio)->source->buffer;
    poly_float* audio_out = output()->buffer;
    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());
    poly_float high_pass_frequency_ratio = kHighPassFrequency * (1.0f / getSampleRate());
    poly_float high_pass_feedback_coefficient = coefficient_lookup->cubicLookup(high_pass_frequency_ratio);

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;
      current_high_pass_ratio += delta_high_pass_ratio;
      current_high_pass_amount += delta_high_pass_amount;

      tick(audio_in[i], coefficient, current_high_pass_ratio, current_high_pass_amount,
           high_pass_feedback_coefficient, current_resonance, current_drive);
      audio_out[i] = stage4_.getCurrentState() * current_post_multiply;
    }
  }

  void DiodeFilter::setupFilter(const FilterState& filter_state) {
    static constexpr float kHighPassStart = -9.0f;
    static constexpr float kHighPassEnd = -1.0f;
    static constexpr float kHighPassRange = kHighPassEnd - kHighPassStart;

    poly_float resonance_percent = utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f);
    resonance_percent *= resonance_percent * resonance_percent;
    resonance_ = utils::interpolate(kMinResonance, kMaxResonance, resonance_percent);
    drive_ = (resonance_ * 0.5f + 1.0f) * filter_state.drive;
    post_multiply_ = poly_float(1.0f) / utils::sqrt(filter_state.drive);
    
    poly_float blend_amount = filter_state.pass_blend * 0.5f;

    if (filter_state.style == k12Db) {
      high_pass_ratio_ = futils::exp2(kHighPassEnd);
      high_pass_amount_ = blend_amount * blend_amount;
    }
    else {
      high_pass_ratio_ = futils::exp2(blend_amount * kHighPassRange + kHighPassStart);
      high_pass_amount_ = 1.0f;
    }
  }

  force_inline void DiodeFilter::tick(poly_float audio_in, poly_float coefficient,
                                      poly_float high_pass_ratio, poly_float high_pass_amount,
                                      poly_float high_pass_feedback_coefficient,
                                      poly_float resonance, poly_float drive) {
    poly_float high_pass_coefficient = coefficient * high_pass_ratio;
    poly_float high_pass_coefficient2 = high_pass_coefficient * 2.0f;
    poly_float high_pass_coefficient_squared = high_pass_coefficient * high_pass_coefficient;
    poly_float high_pass_coefficient_diff = high_pass_coefficient_squared - high_pass_coefficient;
    poly_float high_pass_feedback_mult = high_pass_coefficient2 - high_pass_coefficient_squared - 1.0f;
    poly_float high_pass_normalizer = poly_float(1.0f) / (high_pass_coefficient_diff + 1.0f);

    poly_float high_pass_mult_stage2 = -high_pass_coefficient + 1.0f;
    poly_float high_pass_feedback = high_pass_feedback_mult * high_pass_1_.getNextState() +
                                    high_pass_mult_stage2 * high_pass_2_.getNextState();

    poly_float high_pass_input = (audio_in - high_pass_feedback) * high_pass_normalizer;

    poly_float high_pass_1_out = high_pass_1_.tickBasic(high_pass_input, high_pass_coefficient);
    poly_float high_pass_2_out = high_pass_2_.tickBasic(high_pass_1_out, high_pass_coefficient);
    poly_float high_pass_out = high_pass_input - high_pass_1_out * 2.0f + high_pass_2_out;
    high_pass_out = utils::interpolate(audio_in, high_pass_out, high_pass_amount);

    poly_float filter_state = stage4_.getNextSatState();
    poly_float filter_input = (drive * high_pass_out - resonance * filter_state) * 0.5f;
    poly_float sat_input = futils::tanh(filter_input);

    poly_float feedback_input = sat_input + stage2_.getNextSatState();
    poly_float feedback = high_pass_feedback_.tickBasic(feedback_input, high_pass_feedback_coefficient);
    stage1_.tick(feedback_input - feedback, coefficient);
    stage2_.tick((stage1_.getCurrentState() + stage3_.getNextSatState()) * 0.5f, coefficient);
    stage3_.tick((stage2_.getCurrentState() + stage4_.getNextSatState()) * 0.5f, coefficient);
    stage4_.tick(stage3_.getCurrentState(), coefficient);
  }
} // namespace vital
