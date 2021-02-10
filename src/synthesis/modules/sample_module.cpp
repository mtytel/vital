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

#include "sample_module.h"

#include "synth_constants.h"

namespace vital {

  SampleModule::SampleModule() : SynthModule(kNumInputs, kNumOutputs), on_(nullptr) {
    sampler_ = new SampleSource();
    was_on_ = std::make_shared<bool>(true);
  }

  void SampleModule::init() {
    on_ = createBaseControl("sample_on");
    Value* random_phase = createBaseControl("sample_random_phase");
    Value* loop = createBaseControl("sample_loop");
    Value* bounce = createBaseControl("sample_bounce");
    Value* keytrack = createBaseControl("sample_keytrack");
    Value* transpose_quantize = createBaseControl("sample_transpose_quantize");
    Output* transpose = createPolyModControl("sample_transpose");
    Output* tune = createPolyModControl("sample_tune");
    Output* level = createPolyModControl("sample_level", true, true);
    Output* pan = createPolyModControl("sample_pan");

    sampler_->useInput(input(kReset), SampleSource::kReset);
    sampler_->useInput(input(kMidi), SampleSource::kMidi);
    sampler_->useInput(input(kNoteCount), SampleSource::kNoteCount);
    sampler_->plug(random_phase, SampleSource::kRandomPhase);
    sampler_->plug(keytrack, SampleSource::kKeytrack);
    sampler_->plug(loop, SampleSource::kLoop);
    sampler_->plug(bounce, SampleSource::kBounce);
    sampler_->plug(transpose, SampleSource::kTranspose);
    sampler_->plug(transpose_quantize, SampleSource::kTransposeQuantize);
    sampler_->plug(tune, SampleSource::kTune);
    sampler_->plug(level, SampleSource::kLevel);
    sampler_->plug(pan, SampleSource::kPan);
    sampler_->useOutput(output(kRaw), SampleSource::kRaw);
    sampler_->useOutput(output(kLevelled), SampleSource::kLevelled);

    addProcessor(sampler_);
    SynthModule::init();
  }

  void SampleModule::process(int num_samples) {
    bool on = on_->value();

    if (on)
      SynthModule::process(num_samples);
    else if (*was_on_) {
      output(kRaw)->clearBuffer();
      output(kLevelled)->clearBuffer();
      getPhaseOutput()->buffer[0] = 0.0f;
    }

    *was_on_ = on;
  }
} // namespace vital
