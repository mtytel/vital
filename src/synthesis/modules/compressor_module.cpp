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

#include "compressor_module.h"

#include "compressor.h"

namespace vital {

  CompressorModule::CompressorModule() : SynthModule(0, kNumOutputs), compressor_(nullptr) { }

  CompressorModule::~CompressorModule() {
  }

  void CompressorModule::init() {
    compressor_ = new MultibandCompressor();
    compressor_->useOutput(output(kAudio), MultibandCompressor::kAudio);
    compressor_->useOutput(output(kLowInputMeanSquared), MultibandCompressor::kLowInputMeanSquared);
    compressor_->useOutput(output(kBandInputMeanSquared), MultibandCompressor::kBandInputMeanSquared);
    compressor_->useOutput(output(kHighInputMeanSquared), MultibandCompressor::kHighInputMeanSquared);
    compressor_->useOutput(output(kLowOutputMeanSquared), MultibandCompressor::kLowOutputMeanSquared);
    compressor_->useOutput(output(kBandOutputMeanSquared), MultibandCompressor::kBandOutputMeanSquared);
    compressor_->useOutput(output(kHighOutputMeanSquared), MultibandCompressor::kHighOutputMeanSquared);
    addIdleProcessor(compressor_);

    Output* compressor_attack = createMonoModControl("compressor_attack");
    Output* compressor_release = createMonoModControl("compressor_release");
    Output* compressor_low_gain = createMonoModControl("compressor_low_gain");
    Output* compressor_band_gain = createMonoModControl("compressor_band_gain");
    Output* compressor_high_gain = createMonoModControl("compressor_high_gain");

    Value* compressor_enabled_bands = createBaseControl("compressor_enabled_bands");

    Value* compressor_low_upper_ratio = createBaseControl("compressor_low_upper_ratio");
    Value* compressor_band_upper_ratio = createBaseControl("compressor_band_upper_ratio");
    Value* compressor_high_upper_ratio = createBaseControl("compressor_high_upper_ratio");
    Value* compressor_low_lower_ratio = createBaseControl("compressor_low_lower_ratio");
    Value* compressor_band_lower_ratio = createBaseControl("compressor_band_lower_ratio");
    Value* compressor_high_lower_ratio = createBaseControl("compressor_high_lower_ratio");

    Value* compressor_low_upper_threshold = createBaseControl("compressor_low_upper_threshold");
    Value* compressor_band_upper_threshold = createBaseControl("compressor_band_upper_threshold");
    Value* compressor_high_upper_threshold = createBaseControl("compressor_high_upper_threshold");
    Value* compressor_low_lower_threshold = createBaseControl("compressor_low_lower_threshold");
    Value* compressor_band_lower_threshold = createBaseControl("compressor_band_lower_threshold");
    Value* compressor_high_lower_threshold = createBaseControl("compressor_high_lower_threshold");

    Output* compressor_mix = createMonoModControl("compressor_mix");

    compressor_->plug(compressor_mix, MultibandCompressor::kMix);
    compressor_->plug(compressor_attack, MultibandCompressor::kAttack);
    compressor_->plug(compressor_release, MultibandCompressor::kRelease);
    compressor_->plug(compressor_low_gain, MultibandCompressor::kLowOutputGain);
    compressor_->plug(compressor_band_gain, MultibandCompressor::kBandOutputGain);
    compressor_->plug(compressor_high_gain, MultibandCompressor::kHighOutputGain);
    compressor_->plug(compressor_enabled_bands, MultibandCompressor::kEnabledBands);

    compressor_->plug(compressor_low_upper_ratio, MultibandCompressor::kLowUpperRatio);
    compressor_->plug(compressor_band_upper_ratio, MultibandCompressor::kBandUpperRatio);
    compressor_->plug(compressor_high_upper_ratio, MultibandCompressor::kHighUpperRatio);
    compressor_->plug(compressor_low_lower_ratio, MultibandCompressor::kLowLowerRatio);
    compressor_->plug(compressor_band_lower_ratio, MultibandCompressor::kBandLowerRatio);
    compressor_->plug(compressor_high_lower_ratio, MultibandCompressor::kHighLowerRatio);

    compressor_->plug(compressor_low_upper_threshold, MultibandCompressor::kLowUpperThreshold);
    compressor_->plug(compressor_band_upper_threshold, MultibandCompressor::kBandUpperThreshold);
    compressor_->plug(compressor_high_upper_threshold, MultibandCompressor::kHighUpperThreshold);
    compressor_->plug(compressor_low_lower_threshold, MultibandCompressor::kLowLowerThreshold);
    compressor_->plug(compressor_band_lower_threshold, MultibandCompressor::kBandLowerThreshold);
    compressor_->plug(compressor_high_lower_threshold, MultibandCompressor::kHighLowerThreshold);

    SynthModule::init();
  }

  void CompressorModule::setSampleRate(int sample_rate) {
    SynthModule::setSampleRate(sample_rate);
    compressor_->setSampleRate(sample_rate);
  }

  void CompressorModule::processWithInput(const poly_float* audio_in, int num_samples) {
    SynthModule::process(num_samples);
    compressor_->processWithInput(audio_in, num_samples);
  }

  void CompressorModule::enable(bool enable) {
    SynthModule::enable(enable);
    if (!enable)
      compressor_->reset(constants::kFullMask);
  }

  void CompressorModule::hardReset() {
    compressor_->reset(constants::kFullMask);
  }
} // namespace vital
