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

#include "vocal_tract.h"

namespace vital {
  VocalTract::VocalTract() : ProcessorRouter(kNumInputs, 1) { }

  VocalTract::~VocalTract() { }

  void VocalTract::reset(poly_mask reset_mask) {
  }

  void VocalTract::hardReset() {
    reset(constants::kFullMask);
  }

  void VocalTract::process(int num_samples) {
    const poly_float* audio_in = input(kAudio)->source->buffer;
    processWithInput(audio_in, num_samples);
  }

  void VocalTract::processWithInput(const poly_float* audio_in, int num_samples) {
  }
} // namespace vital
