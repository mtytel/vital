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

#include "processor.h"

#include "feedback.h"
#include "processor_router.h"

namespace vital {

  const Output Processor::null_source_(kMaxBufferSize, kMaxOversample);

  Processor::Processor(int num_inputs, int num_outputs, bool control_rate, int max_oversample) {
    plugging_start_ = 0;
    state_ = std::make_shared<ProcessorState>();
    state_->oversample_amount = max_oversample;
    state_->control_rate = control_rate;

    inputs_ = std::make_shared<std::vector<Input*>>();
    outputs_ = std::make_shared<std::vector<Output*>>();
    router_ = nullptr;

    for (int i = 0; i < num_inputs; ++i)
      addInput();

    for (int i = 0; i < num_outputs; ++i)
      addOutput(max_oversample);
  }

  bool Processor::inputMatchesBufferSize(int input) {
    if (input >= inputs_->size())
      return false;

    if (numOutputs() == 0)
      return true;

    return inputs_->at(input)->source->buffer_size >= output()->buffer_size;
  }

  bool Processor::checkInputAndOutputSize(int num_samples) {
    if (isControlRate())
      return true;

    int num_outputs = numOutputs();
    for (int i = 0; i < num_outputs; ++i) {
      int buffer_size = output(i)->buffer_size;
      if (buffer_size > 1 && buffer_size < num_samples)
        return false;
    }

    int num_inputs = numInputs();
    for (int i = 0; i < num_inputs; ++i) {
      int buffer_size = input(i)->source->buffer_size;
      if (buffer_size > 1 && buffer_size < num_samples)
        return false;
    }

    return true;
  }

  bool Processor::isPolyphonic() const {
    if (router_)
      return router_->isPolyphonic(this);
    return false;
  }

  void Processor::plug(const Output* source) {
    plug(source, 0);
  }

  void Processor::plug(const Output* source, unsigned int input_index) {
    VITAL_ASSERT(input_index < inputs_->size());
    VITAL_ASSERT(source);
    VITAL_ASSERT(inputs_->at(input_index));

    inputs_->at(input_index)->source = source;

    if (router_)
      router_->connect(this, source, input_index);

    numInputsChanged();
  }

  void Processor::plug(const Processor* source) {
    plug(source, 0);
  }

  void Processor::plug(const Processor* source, unsigned int input_index) {
    plug(source->output(), input_index);
  }

  void Processor::plugNext(const Output* source) {
    int num_inputs = static_cast<int>(inputs_->size());
    for (int i = plugging_start_; i < num_inputs; ++i) {
      Input* input = inputs_->at(i);
      if (input && input->source == &Processor::null_source_) {
        plug(source, i);
        return;
      }
    }

    // If there are no empty inputs, create another.
    std::shared_ptr<Input> input = std::make_shared<Input>();
    owned_inputs_.push_back(input);
    input->source = source;
    registerInput(input.get());
    numInputsChanged();
  }

  void Processor::plugNext(const Processor* source) {
    plugNext(source->output());
  }

  void Processor::useInput(Input* input) {
    useInput(input, 0);
  }

  void Processor::useInput(Input* input, int index) {
    VITAL_ASSERT(index < inputs_->size());
    VITAL_ASSERT(input);

    inputs_->at(index) = input;
    numInputsChanged();
  }

  void Processor::useOutput(Output* output) {
    useOutput(output, 0);
  }

  void Processor::useOutput(Output* output, int index) {
    VITAL_ASSERT(index < outputs_->size());
    VITAL_ASSERT(output);

    outputs_->at(index) = output;
  }

  int Processor::connectedInputs() {
    int count = 0;
    int num_inputs = static_cast<int>(inputs_->size());
    for (int i = 0; i < num_inputs; ++i) {
      Input* input = inputs_->at(i);
      if (input && input->source != &Processor::null_source_)
        count++;
    }

    return count;
  }

  void Processor::unplugIndex(unsigned int input_index) {
    if (inputs_->at(input_index))
      inputs_->at(input_index)->source = &Processor::null_source_;
    numInputsChanged();
  }

  void Processor::unplug(const Output* source) {
    if (router_)
      router_->disconnect(this, source);

    for (unsigned int i = 0; i < inputs_->size(); ++i) {
      if (inputs_->at(i) && inputs_->at(i)->source == source)
        inputs_->at(i)->source = &Processor::null_source_;
    }
    numInputsChanged();
  }

  void Processor::unplug(const Processor* source) {
    if (router_) {
      for (int i = 0; i < source->numOutputs(); ++i)
        router_->disconnect(this, source->output(i));
    }
    for (unsigned int i = 0; i < inputs_->size(); ++i) {
      if (inputs_->at(i) && inputs_->at(i)->source->owner == source)
        inputs_->at(i)->source = &Processor::null_source_;
    }
    numInputsChanged();
  }

  ProcessorRouter* Processor::getTopLevelRouter() const {
    ProcessorRouter* top_level = nullptr;
    ProcessorRouter* current_level = router_;

    while (current_level) {
      top_level = current_level;
      current_level = current_level->router();
    }
    return top_level;
  }

  void Processor::registerInput(Input* input) {
    inputs_->push_back(input);

    if (router_ && input->source != &Processor::null_source_)
      router_->connect(this, input->source, static_cast<int>(inputs_->size()) - 1);
  }

  Output* Processor::registerOutput(Output* output) {
    outputs_->push_back(output);
    return output;
  }

  void Processor::registerInput(Input* input, int index) {
    while (inputs_->size() <= index)
      inputs_->push_back(nullptr);

    inputs_->at(index) = input;

    if (router_ && input->source != &Processor::null_source_)
      router_->connect(this, input->source, index);
  }

  Output* Processor::registerOutput(Output* output, int index) {
    while (outputs_->size() <= index)
      outputs_->push_back(nullptr);

    outputs_->at(index) = output;
    return output;
  }

  Output* Processor::addOutput(int oversample) {
    std::shared_ptr<Output> output;
    if (isControlRate())
      output = std::make_shared<cr::Output>();
    else
      output = std::make_shared<Output>(kMaxBufferSize, oversample);

    owned_outputs_.push_back(output);

    // All outputs are owned by this Processor.
    output->owner = this;
    registerOutput(output.get());
    return output.get();
  }

  Input* Processor::addInput() {
    std::shared_ptr<Input> input = std::make_shared<Input>();
    owned_inputs_.push_back(input);

    // All inputs start off with null input.
    input->source = &Processor::null_source_;
    registerInput(input.get());
    return input.get();
  }
} // namespace vital
