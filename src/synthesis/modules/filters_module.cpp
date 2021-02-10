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

#include "filters_module.h"
#include "filter_module.h"

namespace vital {

  FiltersModule::FiltersModule() : SynthModule(kNumInputs, 1), filter_1_(nullptr), filter_2_(nullptr),
                                   filter_1_filter_input_(nullptr), filter_2_filter_input_(nullptr) {
    filter_1_input_ = std::make_shared<Output>();
    filter_2_input_ = std::make_shared<Output>();
  }

  void FiltersModule::init() {
    filter_1_filter_input_ = createBaseControl("filter_1_filter_input");
    filter_1_ = new FilterModule("filter_1");
    addSubmodule(filter_1_);
    addProcessor(filter_1_);

    filter_1_->plug(filter_1_input_.get(), FilterModule::kAudio);
    filter_1_->useInput(input(kReset), FilterModule::kReset);
    filter_1_->useInput(input(kKeytrack), FilterModule::kKeytrack);
    filter_1_->useInput(input(kMidi), FilterModule::kMidi);

    filter_2_filter_input_ = createBaseControl("filter_2_filter_input");
    filter_2_ = new FilterModule("filter_2");
    addSubmodule(filter_2_);
    addProcessor(filter_2_);

    filter_2_->plug(filter_2_input_.get(), FilterModule::kAudio);
    filter_2_->useInput(input(kReset), FilterModule::kReset);
    filter_2_->useInput(input(kKeytrack), FilterModule::kKeytrack);
    filter_2_->useInput(input(kMidi), FilterModule::kMidi);

    SynthModule::init();
  }

  void FiltersModule::processParallel(int num_samples) {
    filter_1_input_->buffer = input(kFilter1Input)->source->buffer;
    filter_2_input_->buffer = input(kFilter2Input)->source->buffer;

    getLocalProcessor(filter_1_)->process(num_samples);
    getLocalProcessor(filter_2_)->process(num_samples);

    poly_float* output_buffer = output()->buffer;
    const poly_float* filter_1_buffer = filter_1_->output()->buffer;
    const poly_float* filter_2_buffer = filter_2_->output()->buffer;

    for (int i = 0; i < num_samples; ++i)
      output_buffer[i] = filter_1_buffer[i] + filter_2_buffer[i];
  }

  void FiltersModule::processSerialForward(int num_samples) {
    filter_1_input_->buffer = input(kFilter1Input)->source->buffer;
    filter_2_input_->buffer = filter_2_input_->owned_buffer.get();

    getLocalProcessor(filter_1_)->process(num_samples);

    poly_float* filter_2_input_buffer = filter_2_input_->buffer;
    const poly_float* filter_1_output_buffer = filter_1_->output()->buffer;
    const poly_float* filter_2_straight_input = input(kFilter2Input)->source->buffer;

    for (int i = 0; i < num_samples; ++i)
      filter_2_input_buffer[i] = filter_1_output_buffer[i] + filter_2_straight_input[i];

    getLocalProcessor(filter_2_)->process(num_samples);
    utils::copyBuffer(output()->buffer, filter_2_->output()->buffer, num_samples);
  }

  void FiltersModule::processSerialBackward(int num_samples) {
    filter_1_input_->buffer = filter_1_input_->owned_buffer.get();
    filter_2_input_->buffer = input(kFilter2Input)->source->buffer;

    getLocalProcessor(filter_2_)->process(num_samples);

    poly_float* filter_1_input_buffer = filter_1_input_->buffer;
    const poly_float* filter_2_output_buffer = filter_2_->output()->buffer;
    const poly_float* filter_1_straight_input = input(kFilter1Input)->source->buffer;

    for (int i = 0; i < num_samples; ++i)
      filter_1_input_buffer[i] = filter_2_output_buffer[i] + filter_1_straight_input[i];

    getLocalProcessor(filter_1_)->process(num_samples);
    utils::copyBuffer(output()->buffer, filter_1_->output()->buffer, num_samples);
  }

  void FiltersModule::process(int num_samples) {
    if (filter_1_filter_input_->value() && filter_1_->getOnValue()->value())
      processSerialBackward(num_samples);
    else if (filter_2_filter_input_->value() && filter_2_->getOnValue()->value())
      processSerialForward(num_samples);
    else
      processParallel(num_samples);
  }
} // namespace vital
