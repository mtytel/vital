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

#include "synth_filter.h"

#include "comb_filter.h"
#include "digital_svf.h"
#include "diode_filter.h"
#include "dirty_filter.h"
#include "formant_filter.h"
#include "ladder_filter.h"
#include "phaser_filter.h"
#include "sallen_key_filter.h"

namespace vital {

  namespace {
    constexpr mono_float kMaxDriveGain = 36.0f;
    constexpr mono_float kMinDriveGain = 0.0f;
  } // namespace

  const SynthFilter::CoefficientLookup SynthFilter::coefficient_lookup_;

  void SynthFilter::FilterState::loadSettings(Processor* processor) {
    midi_cutoff = processor->input(kMidiCutoff)->at(0);
    midi_cutoff_buffer = processor->input(kMidiCutoff)->source->buffer;
    resonance_percent = processor->input(kResonance)->at(0);
    poly_float input_drive = utils::clamp(processor->input(kDriveGain)->at(0), kMinDriveGain, kMaxDriveGain);
    drive_percent = (input_drive - kMinDriveGain) * (1.0f / (kMaxDriveGain - kMinDriveGain));
    drive = futils::dbToMagnitude(input_drive);
    gain = processor->input(kGain)->at(0);
    style = processor->input(kStyle)->at(0)[0];
    pass_blend = utils::clamp(processor->input(kPassBlend)->at(0), 0.0f, 2.0f);
    interpolate_x = processor->input(kInterpolateX)->at(0);
    interpolate_y = processor->input(kInterpolateY)->at(0);
    transpose = processor->input(kTranspose)->at(0);
  }

  SynthFilter* SynthFilter::createFilter(constants::FilterModel model) {
    switch (model) {
      case constants::kAnalog:
        return new SallenKeyFilter();
      case constants::kComb:
        return new CombFilter(1);
      case constants::kDigital:
        return new DigitalSvf();
      case constants::kDirty:
        return new DirtyFilter();
      case constants::kLadder:
        return new LadderFilter();
      case constants::kDiode:
        return new DiodeFilter();
      case constants::kFormant:
        return new FormantFilter(0);
      case constants::kPhase:
        return new PhaserFilter(false);
      default:
        return nullptr;
    }
  }

} // namespace vital
