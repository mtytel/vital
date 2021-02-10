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

#include "value_switch.h"

#include "utils.h"

namespace vital {

  ValueSwitch::ValueSwitch(mono_float value) : cr::Value(value) {
    while (numOutputs() < kNumOutputs)
      addOutput();

    enable(false);
  }

  void ValueSwitch::set(poly_float value) {
    cr::Value::set(value);
    setSource(value[0]);
  }

  void ValueSwitch::setOversampleAmount(int oversample) {
    cr::Value::setOversampleAmount(oversample);
    int num_inputs = numInputs();
    for (int i = 0; i < num_inputs; ++i) {
      input(i)->source->owner->setOversampleAmount(oversample);
    }
    setBuffer(value_[0]);
  }

  force_inline void ValueSwitch::setBuffer(int source) {
    source = utils::iclamp(source, 0, numInputs() - 1);
    output(kSwitch)->buffer = input(source)->source->buffer;
    output(kSwitch)->buffer_size = input(source)->source->buffer_size;
  }

  force_inline void ValueSwitch::setSource(int source) {
    setBuffer(source);

    bool enable_processors = source != 0;
    for (Processor* processor : processors_)
      processor->enable(enable_processors);
  }
} // namespace vital
