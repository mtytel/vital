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

#include "phaser.h"

#include "operators.h"
#include "phaser_filter.h"

#include <climits>

namespace vital {

  Phaser::Phaser() : ProcessorRouter(kNumInputs, kNumOutputs), mix_(0.0f),
                     mod_depth_(0.0f), phase_offset_(0.0f), phase_(0) {
    phaser_filter_ = new PhaserFilter(true);
    addIdleProcessor(phaser_filter_);
  }

  void Phaser::init() {
    phaser_filter_->useInput(input(kFeedbackGain), PhaserFilter::kResonance);
    phaser_filter_->useInput(input(kBlend), PhaserFilter::kPassBlend);
    phaser_filter_->plug(&cutoff_, PhaserFilter::kMidiCutoff);

    phaser_filter_->init();
    ProcessorRouter::init();
  }

  void Phaser::hardReset() {
    phaser_filter_->reset(constants::kFullMask);
    mod_depth_ = input(kModDepth)->at(0);
    phase_offset_ = input(kPhaseOffset)->at(0);
  }

  void Phaser::process(int num_samples) {
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void Phaser::processWithInput(const poly_float* audio_in, int num_samples) {
    VITAL_ASSERT(checkInputAndOutputSize(num_samples));

    poly_float tick_delta = input(kRate)->at(0) * (1.0f / getSampleRate());
    poly_int tick_delta_phase = utils::toInt(tick_delta * UINT_MAX);

    mono_float tick_inc = 1.0f / num_samples;
    poly_float phase_spread = phase_offset_ * constants::kStereoSplit;
    poly_int phase_offset = utils::toInt(phase_spread * INT_MAX);
    phase_offset_ = input(kPhaseOffset)->at(0);
    poly_float end_spread = phase_offset_ * constants::kStereoSplit;
    poly_float delta_spread = (end_spread - phase_spread) * tick_inc;
    poly_int delta_phase_offset = utils::toInt(delta_spread * INT_MAX);

    poly_float current_mod_depth = mod_depth_;
    mod_depth_ = input(kModDepth)->at(0);
    poly_float delta_depth = (mod_depth_ - current_mod_depth) * tick_inc;

    const poly_float* center_buffer = input(kCenter)->source->buffer;
    poly_int current_phase = phase_;
    for (int i = 0; i < num_samples; ++i) {
      phase_offset += delta_phase_offset;
      current_mod_depth += delta_depth;
      poly_int shifted_phase = current_phase + phase_offset;
      poly_mask fold_mask = poly_int::greaterThan(shifted_phase, INT_MAX);
      poly_int folded_phase = utils::maskLoad(shifted_phase, -shifted_phase, fold_mask);
      poly_float modulation = utils::toFloat(folded_phase) * (2.0f / INT_MAX) - 1.0f;
      cutoff_.buffer[i] = center_buffer[i] + modulation * current_mod_depth;
    }

    phaser_filter_->processWithInput(audio_in, num_samples);

    phase_ += utils::toInt((tick_delta * num_samples) * UINT_MAX);
    poly_float current_mix = mix_;
    mix_ = utils::clamp(input(kMix)->at(0), 0.0f, 1.0f);
    poly_float delta_mix = (mix_ - current_mix) * (1.0f / num_samples);

    const poly_float* phaser_out = phaser_filter_->output()->buffer;
    poly_float* audio_out = output(kAudioOutput)->buffer;
    for (int i = 0; i < num_samples; ++i) {
      current_mix += delta_mix;
      audio_out[i] = utils::interpolate(audio_in[i], phaser_out[i], current_mix);
    }

    output(kCutoffOutput)->buffer[0] = cutoff_.buffer[num_samples - 1];
  }

  void Phaser::correctToTime(double seconds) {
    poly_float rate = input(kRate)->at(0);
    poly_float offset = utils::getCycleOffsetFromSeconds(seconds, rate);
    phase_ = utils::toInt((offset - 0.5f) * UINT_MAX) + INT_MAX / 2;
  }
} // namespace vital
