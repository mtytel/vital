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

#include "upsampler.h"

namespace vital {
  Upsampler::Upsampler() : ProcessorRouter(kNumInputs, 1) { }

  Upsampler::~Upsampler() { }

  void Upsampler::process(int num_samples) {
    const poly_float* audio_in = input(kAudio)->source->buffer;
    processWithInput(audio_in, num_samples);
  }

  void Upsampler::processWithInput(const poly_float* audio_in, int num_samples) {
    poly_float* destination = output()->buffer;

    int oversample_amount = getOversampleAmount();

    for (int i = 0; i < num_samples; ++i) {
      int offset = i * oversample_amount;
      for (int s = 0; s < oversample_amount; ++s)
        destination[offset + s] = audio_in[i];
    }
  }
} // namespace vital
