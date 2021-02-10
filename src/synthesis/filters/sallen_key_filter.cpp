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

#include "sallen_key_filter.h"

#include "futils.h"

namespace vital {

  SallenKeyFilter::SallenKeyFilter() : Processor(SallenKeyFilter::kNumInputs, 1) {
    hardReset();
  }

  void SallenKeyFilter::reset(poly_mask reset_mask) {
    stage1_input_ = utils::maskLoad(stage1_input_, 0.0f, reset_mask);

    pre_stage1_.reset(reset_mask);
    pre_stage2_.reset(reset_mask);
    stage1_.reset(reset_mask);
    stage2_.reset(reset_mask);
  }

  void SallenKeyFilter::hardReset() {
    reset(constants::kFullMask);
    resonance_ = 0.0f;
    drive_ = 0.0f;
    post_multiply_ = 0.0f;
    low_pass_amount_ = 0.0f;
    band_pass_amount_ = 0.0f;
    high_pass_amount_ = 0.0f;
  }

  void SallenKeyFilter::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize(kAudio));
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void SallenKeyFilter::processWithInput(const poly_float* audio_in, int num_samples) {
    poly_float current_resonance = resonance_;
    poly_float current_drive = drive_;
    poly_float current_post_multiply = post_multiply_;
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
      current_post_multiply = utils::maskLoad(current_post_multiply, post_multiply_, reset_mask);
      current_low = utils::maskLoad(current_low, low_pass_amount_, reset_mask);
      current_band = utils::maskLoad(current_band, band_pass_amount_, reset_mask);
      current_high = utils::maskLoad(current_high, high_pass_amount_, reset_mask);
    }

    if (filter_state_.style == k12Db) {
      process12(audio_in, num_samples, current_resonance, current_drive, current_post_multiply,
                current_low, current_band, current_high);
    }
    else if (filter_state_.style == kDualNotchBand) {
      processDual(audio_in, num_samples, current_resonance, current_drive, current_post_multiply,
                  current_low, current_high);
    }
    else {
      process24(audio_in, num_samples, current_resonance, current_drive, current_post_multiply, 
                current_low, current_band, current_high);
    }
  }

  void SallenKeyFilter::process12(const poly_float* audio_in, int num_samples,
                                  poly_float current_resonance,
                                  poly_float current_drive, poly_float current_post_multiply, 
                                  poly_float current_low, poly_float current_band, poly_float current_high) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * tick_increment;
    poly_float delta_low = (low_pass_amount_ - current_low) * tick_increment;
    poly_float delta_band = (band_pass_amount_ - current_band) * tick_increment;
    poly_float delta_high = (high_pass_amount_ - current_high) * tick_increment;

    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    poly_float* audio_out = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);
      poly_float coefficient_squared = coefficient * coefficient;
      poly_float coefficient2 = coefficient * 2.0f;

      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;
      current_low += delta_low;
      current_band += delta_band;
      current_high += delta_high;

      poly_float resonance = tuneResonance(current_resonance, coefficient2);
      poly_float stage1_feedback_mult = coefficient2 - coefficient * coefficient - 1.0f;
      poly_float normalizer = poly_float(1.0f) / (resonance * (coefficient_squared - coefficient) + 1.0f);
      tick(audio_in[i], coefficient, resonance, stage1_feedback_mult, current_drive, normalizer);

      poly_float stage2_input = stage1_.getCurrentState();

      poly_float low_pass = stage2_.getCurrentState();
      poly_float band_pass = stage2_input - low_pass;
      poly_float high_pass = stage1_input_ - stage2_input - band_pass;

      poly_float low = current_low * low_pass;
      poly_float band_low = utils::mulAdd(low, current_band, band_pass);
      audio_out[i] = utils::mulAdd(band_low, current_high, high_pass) * current_post_multiply;

      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void SallenKeyFilter::process24(const poly_float* audio_in, int num_samples,
                                  poly_float current_resonance,
                                  poly_float current_drive, poly_float current_post_multiply, 
                                  poly_float current_low, poly_float current_band, poly_float current_high) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * tick_increment;
    poly_float delta_low = (low_pass_amount_ - current_low) * tick_increment;
    poly_float delta_band = (band_pass_amount_ - current_band) * tick_increment;
    poly_float delta_high = (high_pass_amount_ - current_high) * tick_increment;

    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    poly_float* audio_out = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);
      poly_float coefficient_squared = coefficient * coefficient;
      poly_float coefficient2 = coefficient * 2.0f;

      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;
      current_low += delta_low;
      current_band += delta_band;
      current_high += delta_high;

      poly_float resonance = tuneResonance(current_resonance, coefficient2);
      poly_float stage1_feedback_mult = coefficient2 - coefficient_squared - 1.0f;
      poly_float pre_normalizer = poly_float(1.0f) / ((coefficient_squared - coefficient) + 1.0f);
      poly_float normalizer = poly_float(1.0f) / (resonance * (coefficient_squared - coefficient) + 1.0f);
      tick24(audio_in[i], coefficient, resonance, stage1_feedback_mult, current_drive,
             pre_normalizer, normalizer, current_low, current_band, current_high);

      poly_float stage2_input = stage1_.getCurrentState();

      poly_float low_pass = stage2_.getCurrentState();
      poly_float band_pass = stage2_input - low_pass;
      poly_float high_pass = stage1_input_ - stage2_input - band_pass;

      poly_float low = current_low * low_pass;
      poly_float band_low = utils::mulAdd(low, current_band, band_pass);
      audio_out[i] = utils::mulAdd(band_low, current_high, high_pass) * current_post_multiply;

      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void SallenKeyFilter::processDual(const poly_float* audio_in, int num_samples,
                                    poly_float current_resonance,
                                    poly_float current_drive, poly_float current_post_multiply, 
                                    poly_float current_low, poly_float current_high) {
    mono_float tick_increment = 1.0f / num_samples;
    poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
    poly_float delta_drive = (drive_ - current_drive) * tick_increment;
    poly_float delta_post_multiply = (post_multiply_ - current_post_multiply) * tick_increment;
    poly_float delta_low = (low_pass_amount_ - current_low) * tick_increment;
    poly_float delta_high = (high_pass_amount_ - current_high) * tick_increment;

    const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
    const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
    poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
    poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

    poly_float* audio_out = output()->buffer;

    for (int i = 0; i < num_samples; ++i) {
      poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
      poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
      poly_float coefficient = coefficient_lookup->cubicLookup(frequency);
      poly_float coefficient_squared = coefficient * coefficient;
      poly_float coefficient2 = coefficient * 2.0f;

      current_resonance += delta_resonance;
      current_drive += delta_drive;
      current_post_multiply += delta_post_multiply;
      current_low += delta_low;
      current_high += delta_high;
       
      poly_float resonance = tuneResonance(current_resonance, coefficient2);
      poly_float stage1_feedback_mult = coefficient2 - coefficient_squared - 1.0f;
      poly_float pre_normalizer = poly_float(1.0f) / ((coefficient_squared - coefficient) + 1.0f);
      poly_float normalizer = poly_float(1.0f) / (resonance * (coefficient_squared - coefficient) + 1.0f);
      tick24(audio_in[i], coefficient, resonance, stage1_feedback_mult, current_drive,
             pre_normalizer, normalizer, current_low, 0.0f, current_high);

      poly_float stage2_input = stage1_.getCurrentState();
      poly_float low_pass = stage2_.getCurrentState();
      poly_float high_pass = stage1_input_ - stage2_input - stage2_input + low_pass;

      poly_float low = current_high * low_pass;
      audio_out[i] = utils::mulAdd(low, current_low, high_pass) * current_post_multiply;

      VITAL_ASSERT(utils::isFinite(audio_out[i]));
    }
  }

  void SallenKeyFilter::setupFilter(const FilterState& filter_state) {
    cutoff_ = utils::midiNoteToFrequency(filter_state.midi_cutoff);
    float min_nyquist = getSampleRate() * kMinNyquistMult;
    cutoff_ = utils::clamp(cutoff_, kMinCutoff, min_nyquist);

    poly_float resonance_percent = utils::clamp(filter_state.resonance_percent, 0.0f, 1.0f);
    resonance_percent = utils::sqrt(resonance_percent);
    resonance_ = utils::interpolate(kMinResonance, kMaxResonance, resonance_percent);
    resonance_ += filter_state.drive_percent * filter_state.resonance_percent * kDriveResonanceBoost;

    poly_float blend = utils::clamp(filter_state.pass_blend - 1.0f, -1.0f, 1.0f);

    poly_float resonance_scale = resonance_percent * resonance_percent * 2.0f + 1.0f;
    drive_ = poly_float(filter_state.drive) / resonance_scale;

    if (filter_state.style == kDualNotchBand) {
      poly_float t = blend * 0.5f + 0.5f;
      poly_float drive_t = poly_float::min(-blend + 1.0f, 1.0f);
      poly_float drive_mult = -t + 2.0f;
      drive_ = utils::interpolate(filter_state.drive, drive_ * drive_mult, drive_t);

      low_pass_amount_ = t;
      band_pass_amount_ = 0.0f;
      high_pass_amount_ = 1.0f;
    }
    else if (filter_state.style == kNotchPassSwap) {
      poly_float drive_t = poly_float::abs(blend);
      drive_ = utils::interpolate(filter_state.drive, drive_, drive_t);

      low_pass_amount_ = utils::min(-blend + 1.0f, 1.0f);
      band_pass_amount_ = 0.0f;
      high_pass_amount_ = utils::min(blend + 1.0f, 1.0f);
    }
    else if (filter_state.style == kBandPeakNotch) {
      poly_float drive_t = poly_float::min(-blend + 1.0f, 1.0f);
      drive_ = utils::interpolate(filter_state.drive, drive_, drive_t);

      poly_float drive_inv_t = -drive_t + 1.0f;
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

    post_multiply_ = poly_float(1.0f) / utils::sqrt(resonance_scale * drive_);
  }

  force_inline void SallenKeyFilter::tick24(poly_float audio_in, poly_float coefficient,
                                            poly_float resonance, poly_float stage1_feedback_mult, poly_float drive,
                                            poly_float pre_normalizer, poly_float normalizer,
                                            poly_float low, poly_float band, poly_float high) {
    poly_float mult_stage2 = -coefficient + 1.0f;
    poly_float feedback = utils::mulAdd(stage1_feedback_mult * pre_stage1_.getNextState(),
                                        mult_stage2, pre_stage2_.getNextState());

    poly_float stage1_input = (audio_in - feedback) * pre_normalizer;

    poly_float stage1_out = pre_stage1_.tickBasic(stage1_input, coefficient);
    poly_float stage2_out = pre_stage2_.tickBasic(stage1_out, coefficient);

    poly_float band_pass_out = stage1_out - stage2_out;
    poly_float high_pass_out = stage1_input - stage1_out - band_pass_out;

    poly_float low_out = low * stage2_out;
    poly_float band_low_out = utils::mulAdd(low_out, band, band_pass_out);
    poly_float audio_out = utils::mulAdd(band_low_out, high, high_pass_out);

    tick(audio_out, coefficient, resonance, stage1_feedback_mult, drive, normalizer);
  }

  force_inline void SallenKeyFilter::tick(poly_float audio_in, poly_float coefficient, poly_float resonance,
                                          poly_float stage1_feedback_mult, poly_float drive, poly_float normalizer) {
    poly_float mult_stage2 = -coefficient + 1.0f;
    poly_float feedback = utils::mulAdd(stage1_feedback_mult * stage1_.getNextState(),
                                        mult_stage2, stage2_.getNextState());

    stage1_input_ = futils::tanh((drive * audio_in - resonance * feedback) * normalizer);

    poly_float stage1_out = stage1_.tickBasic(stage1_input_, coefficient);
    stage2_.tickBasic(stage1_out, coefficient);
  }
} // namespace vital
