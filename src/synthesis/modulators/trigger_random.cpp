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

#include "trigger_random.h"

#include <cstdlib>

namespace vital {

  TriggerRandom::TriggerRandom() : Processor(1, 1, true), value_(0.0f), random_generator_(0.0f, 1.0f) { }

  void TriggerRandom::process(int num_samples) {
    poly_mask trigger_mask = getResetMask(kReset);
    if (trigger_mask.anyMask()) {
      for (int i = 0; i < poly_float::kSize; i += 2) {
        if ((poly_float(1.0f) & trigger_mask)[i]) {
          mono_float rand_value = random_generator_.next();
          value_.set(i, rand_value);
          value_.set(i + 1, rand_value);
        }
      }
    }

    output()->buffer[0] = value_;
  }
} // namespace vital
