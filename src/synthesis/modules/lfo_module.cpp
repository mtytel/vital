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

#include "lfo_module.h"

#include "line_generator.h"
#include "synth_lfo.h"

namespace vital {

  LfoModule::LfoModule(const std::string& prefix, LineGenerator* line_generator, const Output* beats_per_second) :
      SynthModule(kNumInputs, kNumOutputs), prefix_(prefix), beats_per_second_(beats_per_second) {
    lfo_ = new SynthLfo(line_generator);
    addProcessor(lfo_);

    setControlRate(true);
  }

  void LfoModule::init() {
    Output* free_frequency = createPolyModControl(prefix_ + "_frequency");
    Output* phase = createPolyModControl(prefix_ + "_phase");
    Output* fade = createPolyModControl(prefix_ + "_fade_time");
    Output* delay = createPolyModControl(prefix_ + "_delay_time");
    Output* stereo_phase = createPolyModControl(prefix_ + "_stereo");
    Value* sync_type = createBaseControl(prefix_ + "_sync_type");
    Value* smooth_mode = createBaseControl(prefix_ + "_smooth_mode");
    Output* smooth_time = createPolyModControl(prefix_ + "_smooth_time");

    Output* frequency = createTempoSyncSwitch(prefix_, free_frequency->owner, beats_per_second_, true, input(kMidi));
    lfo_->useInput(input(kNoteTrigger), SynthLfo::kNoteTrigger);
    lfo_->useInput(input(kNoteCount), SynthLfo::kNoteCount);

    lfo_->useOutput(output(kValue), SynthLfo::kValue);
    lfo_->useOutput(output(kOscPhase), SynthLfo::kOscPhase);
    lfo_->useOutput(output(kOscFrequency), SynthLfo::kOscFrequency);
    lfo_->plug(frequency, SynthLfo::kFrequency);
    lfo_->plug(phase, SynthLfo::kPhase);
    lfo_->plug(stereo_phase, SynthLfo::kStereoPhase);
    lfo_->plug(sync_type, SynthLfo::kSyncType);
    lfo_->plug(smooth_mode, SynthLfo::kSmoothMode);
    lfo_->plug(fade, SynthLfo::kFade);
    lfo_->plug(smooth_time, SynthLfo::kSmoothTime);
    lfo_->plug(delay, SynthLfo::kDelay);
  }

  void LfoModule::correctToTime(double seconds) {
    lfo_->correctToTime(seconds);
  }

  void LfoModule::setControlRate(bool control_rate) {
    Processor::setControlRate(control_rate);
    lfo_->setControlRate(control_rate);
  }
} // namespace vital
