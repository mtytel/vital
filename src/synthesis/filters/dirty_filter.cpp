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

#include "dirty_filter.h"

#include "futils.h"

namespace vital {

  DirtyFilter::DirtyFilter() : Processor(DirtyFilter::kNumInputs, 1) {
    hardReset();
  }

  void DirtyFilter::reset(poly_mask reset_mask) {
    pre_stage1_.reset(reset_mask);
    pre_stage2_.reset(reset_mask);
    stage1_.reset(reset_mask);
    stage2_.reset(reset_mask);
    stage3_.reset(reset_mask);
    stage4_.reset(reset_mask);
  }

  void DirtyFilter::hardReset() {
    reset(constants::kFullMask);
    coefficient_ = 0.1f;
    resonance_ = 0.0f;
    drive_ = 0.0f;
    drive_boost_ = 0.0f;
    drive_blend_ = 0.0f;
    drive_mult_ = 0.0f;
    low_pass_amount_ = 0.0f;
    band_pass_amount_ = 0.0f;
    high_pass_amount_ = 0.0f;
  }

  void DirtyFilter::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));

    poly_float current_resonance = resonance_;
    poly_float current_drive = drive_;
    poly_float current_drive_boost = drive_boost_;
    poly_float current_drive_blend = drive_blend_;
    poly_float current_drive_mult = drive_mult_;
    poly_float current_low = low_pass_amount_;
    poly_float current_band = band_pass_amount_;
    poly_float current_high = high_pass_amount_;

    filter_state_.loadSettings(this);
    setupFilter(filter_state_);

    poly_mask reset_mask = getResetMask(kReset);
    
    if (reset_mask.anyMask()) {
      reset(reset_mask);

      current_resonance = utils::maskLoad(current_resonance, resonance_, reset_mask);
      current_drive = utils::maskLoad(current_drive, drive_, reset_mask);
      current_drive_boost = utils::maskLoad(current_drive_boost, drive_boost_, reset_mask);
      current_drive_blend = utils::maskLoad(current_drive_blend, drive_blend_, reset_mask);
      current_drive_mult = utils::maskLoad(current_drive_mult, drive_mult_, reset_mask);
      current_low = utils::maskLoad(current_low, low_pass_amount_, reset_mask);
      current_band = utils::maskLoad(current_band, band_pass_amount_, reset_mask);
      current_high = utils::maskLoad(current_high, high_pass_amount_, reset_mask);
    }

    if (filter_state_.style == k12Db) {
      process12(num_samples, current_resonance,
                current_drive, current_drive_boost, current_drive_blend,
                current_low, current_band, current_high);
    }
    else if (filter_state_.style == kDualNotchBand) {
      processDual(num_samples, current_resonance,
                  current_drive, current_drive_boost, current_drive_blend, current_drive_mult,
                  current_low, current_high);
    }
    else {
      process24(num_samples, current_resonance,
                current_drive, current_drive_boost, current_drive_blend,
                current_low, current_band, current_high);
    }
  }

  void DirtyFilter::process12(int num_samples, poly_float current_resonance,
                              poly_float current_drive, poly_float current_drive_boost, poly_float current_drive_blend,
                              poly_float current_low, poly_float current_band, poly_float current_high) {
    const poly_float* audio_in = input(kAudio)->source->buffer;

    poly_float* audio_out = output()->buffer;

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_drive_boost = (drive_boost_ - current_drive_boost) * tick_increment;
    poly_float delta_drive_blend = (drive_blend_ - current_drive_blend) * tick_increment;
    poly_float delta_low = (low_pass_amount_ - current_low) * tick_increment;
    poly_float delta_band = (band_pass_amount_ - current_band) * tick_increment;
    poly_float delta_high = (high_pass_amount_ - current_high) * tick_increment;

    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      current_drive_boost += delta_drive_boost;
      current_resonance += delta_resonance;

      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      poly_float coefficient_squared = coefficient * coefficient;
      poly_float coefficient2 = coefficient * 2.0f;
      poly_float resonance_in = utils::clamp(tuneResonance(current_resonance, coefficient2), 0.0f, 1.0f);
      poly_float resonance = utils::interpolate(kMinResonance, kMaxResonance, resonance_in) + current_drive_boost;
      poly_float resonance_squared = resonance * resonance;

      poly_float normalizer = poly_float(kSaturationBoost) / (resonance_squared + 1.0f);
      poly_float compute = -resonance * (coefficient - coefficient_squared) + 1.0f;
      poly_float feed_mult = poly_float(1.0f) / (compute * (coefficient + 1.0f));

      current_drive += delta_drive;
      current_drive_blend += delta_drive_blend;

      poly_float scaled_drive = utils::max(poly_float(kMinDrive), current_drive) / (resonance_squared * 0.5f + 1.0f);
      poly_float drive = utils::interpolate(current_drive, scaled_drive, current_drive_blend);

      current_low += delta_low;
      current_band += delta_band;
      current_high += delta_high;

      audio_out[i] = tick(audio_in[i], coefficient, resonance, drive, feed_mult, normalizer,
                          current_low, current_band, current_high);
    }
  }

  void DirtyFilter::process24(int num_samples, poly_float current_resonance,
                              poly_float current_drive, poly_float current_drive_boost, poly_float current_drive_blend,
                              poly_float current_low, poly_float current_band, poly_float current_high) {
    const poly_float* audio_in = input(kAudio)->source->buffer;

    poly_float* audio_out = output()->buffer;

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_drive_boost = (drive_boost_ - current_drive_boost) * tick_increment;
    poly_float delta_drive_blend = (drive_blend_ - current_drive_blend) * tick_increment;
    poly_float delta_low = (low_pass_amount_ - current_low) * tick_increment;
    poly_float delta_band = (band_pass_amount_ - current_band) * tick_increment;
    poly_float delta_high = (high_pass_amount_ - current_high) * tick_increment;

    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      current_drive_boost += delta_drive_boost;
      current_resonance += delta_resonance;

      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      poly_float coefficient_squared = coefficient * coefficient;
      poly_float coefficient2 = coefficient * 2.0f;
      poly_float resonance_in = utils::clamp(tuneResonance(current_resonance, coefficient2), 0.0f, 1.0f);
      poly_float resonance = utils::interpolate(kMinResonance, kMaxResonance, resonance_in) + current_drive_boost;
      poly_float resonance_squared = resonance * resonance;

      poly_float normalizer = poly_float(kSaturationBoost) / (resonance_squared + 1.0f);
      poly_float coefficient_diff = coefficient_squared - coefficient;
      poly_float compute = resonance * coefficient_diff + 1.0f;
      poly_float feed_mult = poly_float(1.0f) / (compute * (coefficient + 1.0f));
      poly_float pre_feedback = coefficient2 - coefficient_squared - 1.0f;
      poly_float pre_normalizer = poly_float(1.0f) / (coefficient_diff * kFlatResonance + 1.0f);

      current_drive += delta_drive;
      current_drive_blend += delta_drive_blend;

      poly_float scaled_drive = utils::max(poly_float(kMinDrive), current_drive) / (resonance_squared * 0.5f + 1.0f);
      poly_float drive = utils::interpolate(current_drive, scaled_drive, current_drive_blend);

      current_low += delta_low;
      current_band += delta_band;
      current_high += delta_high;

      audio_out[i] = tick24(audio_in[i], coefficient, resonance, drive, feed_mult, normalizer,
                            pre_feedback, pre_normalizer,
                            current_low, current_band, current_high);
    }
  }

  void DirtyFilter::processDual(int num_samples, poly_float current_resonance,
                                poly_float current_drive, poly_float current_drive_boost,
                                poly_float current_drive_blend, poly_float current_drive_mult,
                                poly_float current_low, poly_float current_high) {
    const poly_float* audio_in = input(kAudio)->source->buffer;

    poly_float* audio_out = output()->buffer;

    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_drive_boost = (drive_boost_ - current_drive_boost) * tick_increment;
    poly_float delta_drive_blend = (drive_blend_ - current_drive_blend) * tick_increment;
    poly_float delta_drive_mult = (drive_mult_ - current_drive_mult) * tick_increment;
    poly_float delta_low = (low_pass_amount_ - current_low) * tick_increment;
    poly_float delta_high = (high_pass_amount_ - current_high) * tick_increment;

    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    for (int i = 0; i < num_samples; ++i) {
      current_drive_boost += delta_drive_boost;
      current_resonance += delta_resonance;

      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

      poly_float coefficient_squared = coefficient * coefficient;
      poly_float coefficient2 = coefficient * 2.0f;
      poly_float resonance_in = utils::clamp(tuneResonance(current_resonance, coefficient2), 0.0f, 1.0f);
      poly_float resonance = utils::interpolate(kMinResonance, kMaxResonance, resonance_in) + current_drive_boost;
      poly_float resonance_squared = resonance * resonance;

      poly_float normalizer = poly_float(kSaturationBoost) / (resonance_squared + 1.0f);
      poly_float coefficient_diff = coefficient_squared - coefficient;
      poly_float compute = resonance * coefficient_diff + 1.0f;
      poly_float feed_mult = poly_float(1.0f) / (compute * (coefficient + 1.0f));
      poly_float pre_feedback = coefficient2 - coefficient_squared - 1.0f;
      poly_float pre_normalizer = poly_float(1.0f) / (coefficient_diff * kFlatResonance + 1.0f);

      current_drive += delta_drive;
      current_drive_blend += delta_drive_blend;

      poly_float scaled_drive = utils::max(poly_float(kMinDrive), current_drive) / (resonance_squared * 0.5f + 1.0f);
      poly_float drive = utils::interpolate(current_drive, scaled_drive * current_drive_mult, current_drive_blend);

      current_low += delta_low;
      current_high += delta_high;

      audio_out[i] = tickDual(audio_in[i], coefficient, resonance, drive, feed_mult, normalizer,
                              pre_feedback, pre_normalizer, current_low, current_high);
    }
  }

  void DirtyFilter::setupFilter(const FilterState& filter_state) {
    static constexpr float kMaxMidi = 150.0f;
    poly_float cutoff = utils::clamp(filter_state.midi_cutoff, 0.0f, kMaxMidi);
    poly_float base_frequency = utils::midiNoteToFrequency(cutoff) * (1.0f / getSampleRate());
    coefficient_ = getCoefficientLookup()->cubicLookup(base_frequency);

    resonance_ = utils::sqrt(utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f));
    drive_ = (filter_state.drive - 1.0f) * 2.0f + 1.0f;
    drive_boost_ = filter_state.drive_percent * kDriveResonanceBoost;

    drive_blend_ = 1.0f;
    drive_mult_ = 1.0f;

    poly_float blend = utils::clamp(filter_state.pass_blend - 1.0f, -1.0f, 1.0f);
    if (filter_state.style == kDualNotchBand) {
      poly_float t = blend * 0.5f + 0.5f;
      drive_blend_ = poly_float::min(-blend + 1.0f, 1.0f);
      drive_mult_ = -t + 2.0f;

      low_pass_amount_ = t;
      band_pass_amount_ = 0.0f;
      high_pass_amount_ = 1.0f;
    }
    else if (filter_state.style == kNotchPassSwap) {
      drive_blend_ = poly_float::abs(blend);

      low_pass_amount_ = utils::min(-blend + 1.0f, 1.0f);
      band_pass_amount_ = 0.0f;
      high_pass_amount_ = utils::min(blend + 1.0f, 1.0f);
    }
    else if (filter_state.style == kBandPeakNotch) {
      drive_blend_ = poly_float::min(-blend + 1.0f, 1.0f);

      poly_float drive_inv_t = -drive_blend_ + 1.0f;
      poly_float mult = utils::sqrt((drive_inv_t * drive_inv_t) * 0.5f + 0.5f);
      poly_float peak_band_value = -utils::max(-blend, 0.0f);
      low_pass_amount_ = mult * (peak_band_value + 1.0f);
      band_pass_amount_ = mult * (peak_band_value - blend + 1.0f) * 2.0f;
      high_pass_amount_ = low_pass_amount_;
    }
    else {
      band_pass_amount_ = utils::sqrt(-blend * blend + 1.0f);
      poly_mask blend_mask = poly_float::lessThan(blend, 0.0f);
      low_pass_amount_ = (-blend) & blend_mask;
      high_pass_amount_ = blend & ~blend_mask;
    }
  }

  force_inline poly_float DirtyFilter::tick24(poly_float audio_in,
                                              poly_float coefficient, poly_float resonance,
                                              poly_float drive, poly_float feed_mult, poly_float normalizer,
                                              poly_float pre_feedback_mult, poly_float pre_normalizer,
                                              poly_float low, poly_float band, poly_float high) {
    poly_float mult_stage2 = -coefficient + 1.0f;
    poly_float feedback = pre_feedback_mult * pre_stage1_.getNextSatState() +
                          mult_stage2 * pre_stage2_.getNextSatState();

    feedback *= kFlatResonance;
    poly_float stage1_input = (audio_in - feedback) * pre_normalizer;

    poly_float stage1_out = pre_stage1_.tickBasic(stage1_input, coefficient);
    poly_float stage2_out = pre_stage2_.tickBasic(stage1_out, coefficient);

    poly_float band_pass = stage1_out - stage2_out;
    poly_float high_pass = stage1_input - stage1_out - band_pass;
    poly_float pre_out = band * band_pass + high * high_pass + low * stage2_out;

    return tick(pre_out, coefficient, resonance, drive, feed_mult, normalizer, low, band, high);
  }

  force_inline poly_float DirtyFilter::tickDual(poly_float audio_in,
                                                poly_float coefficient, poly_float resonance,
                                                poly_float drive, poly_float feed_mult, poly_float normalizer,
                                                poly_float pre_feedback_mult, poly_float pre_normalizer,
                                                poly_float low, poly_float high) {
    poly_float mult_stage2 = -coefficient + 1.0f;
    poly_float feedback = pre_feedback_mult * pre_stage1_.getNextSatState() +
                          mult_stage2 * pre_stage2_.getNextSatState();

    feedback *= kFlatResonance;
    poly_float stage1_input = (audio_in - feedback) * pre_normalizer;

    poly_float stage1_out = pre_stage1_.tickBasic(stage1_input, coefficient);
    poly_float stage2_out = pre_stage2_.tickBasic(stage1_out, coefficient);

    poly_float band_pass = stage1_out - stage2_out;
    poly_float high_pass = stage1_input - stage1_out - band_pass;

    poly_float pre_out = low * high_pass + high * stage2_out;

    return tick(pre_out, coefficient, resonance, drive, feed_mult, normalizer, low, 0.0f, high);
  }

  force_inline poly_float DirtyFilter::tick(poly_float audio_in, poly_float coefficient, poly_float resonance,
                                            poly_float drive, poly_float feed_mult, poly_float normalizer,
                                            poly_float low, poly_float band, poly_float high) {
    poly_float stage1_in = normalizer * audio_in;
    poly_float stage1_out = stage1_.tickBasic(stage1_in, coefficient);
    poly_float stage2_out = stage2_.tickBasic(stage1_out, coefficient);

    poly_float band_pass = stage1_out - stage2_out;
    poly_float high_pass = stage1_in - stage1_out - band_pass;
    poly_float pass_output = utils::mulAdd(utils::mulAdd(low * stage2_out, band, band_pass), high, high_pass);

    poly_float feedback = stage4_.getNextSatState() +
                          utils::mulAdd(pass_output, coefficient, pass_output - stage3_.getNextSatState());

    poly_float loop_input = futils::tanh(utils::mulAdd(drive * pass_output, resonance, feed_mult * feedback));

    poly_float stage3_out = stage3_.tick(loop_input, coefficient);

    poly_float stage4_in = loop_input - stage3_out;
    stage4_.tick(stage4_in, coefficient);

    return loop_input * (1.0f / kSaturationBoost);
  }
} // namespace vital
