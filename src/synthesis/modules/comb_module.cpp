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

#include "comb_module.h"

#include "comb_filter.h"

namespace vital {

  CombModule::CombModule() : SynthModule(kNumInputs, 1), comb_filter_(nullptr) { }

  void CombModule::init() {
    comb_filter_ = new CombFilter(kMaxFeedbackSamples);
    addProcessor(comb_filter_);

    comb_filter_->useInput(input(kAudio), CombFilter::kAudio);
    comb_filter_->useInput(input(kMidiCutoff), CombFilter::kMidiCutoff);
    comb_filter_->useInput(input(kStyle), CombFilter::kStyle);
    comb_filter_->useInput(input(kMidiBlendTranspose), CombFilter::kTranspose);
    comb_filter_->useInput(input(kFilterCutoffBlend), CombFilter::kPassBlend);
    comb_filter_->useInput(input(kResonance), CombFilter::kResonance);
    comb_filter_->useInput(input(kReset), CombFilter::kReset);
    comb_filter_->useOutput(output());

    SynthModule::init();
  }

  void CombModule::reset(poly_mask reset_mask) {
    getLocalProcessor(comb_filter_)->reset(reset_mask);
  }

  void CombModule::hardReset() {
    getLocalProcessor(comb_filter_)->hardReset();
  }
} // namespace vital
