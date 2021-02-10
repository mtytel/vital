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

#include "reverb_module.h"

#include "reverb.h"

namespace vital {

  ReverbModule::ReverbModule() : SynthModule(0, 1), reverb_(nullptr) { }

  ReverbModule::~ReverbModule() {
  }

  void ReverbModule::init() {
    reverb_ = new Reverb();
    reverb_->useOutput(output());
    addIdleProcessor(reverb_);

    Output* reverb_decay_time = createMonoModControl("reverb_decay_time");
    Output* reverb_pre_low_cutoff = createMonoModControl("reverb_pre_low_cutoff");
    Output* reverb_pre_high_cutoff = createMonoModControl("reverb_pre_high_cutoff");
    Output* reverb_low_shelf_cutoff = createMonoModControl("reverb_low_shelf_cutoff");
    Output* reverb_low_shelf_gain = createMonoModControl("reverb_low_shelf_gain");
    Output* reverb_high_shelf_cutoff = createMonoModControl("reverb_high_shelf_cutoff");
    Output* reverb_high_shelf_gain = createMonoModControl("reverb_high_shelf_gain");
    Output* reverb_chorus_amount = createMonoModControl("reverb_chorus_amount");
    Output* reverb_chorus_frequency = createMonoModControl("reverb_chorus_frequency");
    Output* reverb_size = createMonoModControl("reverb_size");
    Output* reverb_delay = createMonoModControl("reverb_delay");
    Output* reverb_wet = createMonoModControl("reverb_dry_wet");

    reverb_->plug(reverb_decay_time, Reverb::kDecayTime);
    reverb_->plug(reverb_pre_low_cutoff, Reverb::kPreLowCutoff);
    reverb_->plug(reverb_pre_high_cutoff, Reverb::kPreHighCutoff);
    reverb_->plug(reverb_low_shelf_cutoff, Reverb::kLowCutoff);
    reverb_->plug(reverb_low_shelf_gain, Reverb::kLowGain);
    reverb_->plug(reverb_high_shelf_cutoff, Reverb::kHighCutoff);
    reverb_->plug(reverb_high_shelf_gain, Reverb::kHighGain);
    reverb_->plug(reverb_chorus_amount, Reverb::kChorusAmount);
    reverb_->plug(reverb_chorus_frequency, Reverb::kChorusFrequency);
    reverb_->plug(reverb_delay, Reverb::kDelay);
    reverb_->plug(reverb_size, Reverb::kSize);
    reverb_->plug(reverb_wet, Reverb::kWet);

    SynthModule::init();
  }

  void ReverbModule::hardReset() {
    reverb_->hardReset();
  }

  void ReverbModule::enable(bool enable) {
    SynthModule::enable(enable);
    process(1);
    if (!enable)
      reverb_->hardReset();
  }

  void ReverbModule::setSampleRate(int sample_rate) {
    SynthModule::setSampleRate(sample_rate);
    reverb_->setSampleRate(sample_rate);
  }

  void ReverbModule::processWithInput(const poly_float* audio_in, int num_samples) {
    SynthModule::process(num_samples);
    reverb_->processWithInput(audio_in, num_samples);
  }
} // namespace vital
