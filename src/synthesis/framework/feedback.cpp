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

#include "feedback.h"

#include "processor_router.h"

namespace vital {

  void Feedback::process(int num_samples) {
    VITAL_ASSERT(inputMatchesBufferSize());

    const poly_float* audio_in = input(0)->source->buffer;
    for (int i = 0; i < num_samples; ++i) {
      buffer_[buffer_index_] = audio_in[i];
      buffer_index_ = (buffer_index_ + 1) % kMaxBufferSize;
    }
  }

  void Feedback::refreshOutput(int num_samples) {
    poly_float* audio_out = output(0)->buffer;
    int index = (kMaxBufferSize + buffer_index_ - num_samples) % kMaxBufferSize;
    for (int i = 0; i < num_samples; ++i) {
      audio_out[i] = buffer_[index];
      index = (index + 1) % kMaxBufferSize;
    }
  }
} // namespace vital
