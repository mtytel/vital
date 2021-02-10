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

#include "value.h"

#include "utils.h"

namespace vital {

  Value::Value(poly_float value, bool control_rate) : Processor(kNumInputs, 1, control_rate), value_(value) {
    for (int i = 0; i < output()->buffer_size; ++i)
      output()->buffer[i] = value_;
  }

  void Value::set(poly_float value) {
    value_ = value;
    for (int i = 0; i < output()->buffer_size; ++i)
      output()->buffer[i] = value;
  }

  void Value::process(int num_samples) {
    poly_mask trigger_mask = input(kSet)->source->trigger_mask;
    if (trigger_mask.anyMask()) {
      poly_float trigger_value = input(kSet)->source->trigger_value;
      value_ = utils::maskLoad(value_, trigger_value, trigger_mask);
    }

    poly_float* dest = output()->buffer;
    for (int i = 0; i < num_samples; ++i)
      dest[i] = value_;
  }

  void Value::setOversampleAmount(int oversample) {
    Processor::setOversampleAmount(oversample);
    for (int i = 0; i < output()->buffer_size; ++i)
      output()->buffer[i] = value_;
  }

  void cr::Value::process(int num_samples) {
    poly_mask trigger_mask = input(kSet)->source->trigger_mask;
    if (trigger_mask.anyMask()) {
      poly_float trigger_value = input(kSet)->source->trigger_value;
      value_ = utils::maskLoad(value_, trigger_value, trigger_mask);
    }

    output()->buffer[0] = value_;
  }
} // namespace vital
