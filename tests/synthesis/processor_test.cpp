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

#include "processor_test.h"
#include "processor.h"
#include "value.h"

#include <cmath>

#define PROCESS_AMOUNT 600

#define RANDOMIZE_AMOUNT 50

void ProcessorTest::processAndCheckFinite(vital::Processor* processor, const std::set<int>& ignore_outputs) {
  processor->setSampleRate(processor->getSampleRate());

  int num_outputs = processor->numOutputs();
  for (int i = 0; i < PROCESS_AMOUNT; ++i)
    processor->process(vital::kMaxBufferSize);

  for (int i = 0; i < num_outputs; ++i) {
    if (ignore_outputs.count(i) == 0) {
      vital::Output* output = processor->output(i);
      expect(vital::utils::isContained(output->buffer, output->buffer_size));
    }
  }
}

void ProcessorTest::runInputBoundsTest(vital::Processor* processor) {
  runInputBoundsTest(processor, std::set<int>(), std::set<int>());
}

void ProcessorTest::runInputBoundsTest(vital::Processor* processor, std::set<int> leave_inputs,
                                       std::set<int> ignore_outputs) {
  int num_inputs = processor->numInputs();

  std::vector<vital::Value> inputs;
  vital::Output audio;
  audio.ensureBufferSize(vital::kMaxBufferSize);
  for (int i = 0; i < vital::kMaxBufferSize; ++i)
    audio.buffer[i] = (rand() * 2.0f) / RAND_MAX - 1.0f;

  inputs.resize(num_inputs);
  processor->plug(&audio, 0);
  for (int i = 1; i < num_inputs; ++i) {
    if (!leave_inputs.count(i))
      processor->plug(&inputs[i], i);
  }

  beginTest("Inputs Zeroed Test");
  processAndCheckFinite(processor, ignore_outputs);

  beginTest("Inputs High");
  for (int i = 1; i < num_inputs; ++i)
    inputs[i].set(100000.0f);

  processAndCheckFinite(processor, ignore_outputs);

  beginTest("Inputs Negative");
  for (int i = 1; i < num_inputs; ++i)
    inputs[i].set(-100000.0f);

  processAndCheckFinite(processor, ignore_outputs);

  beginTest("Inputs Random");
  for (int r = 0; r < RANDOMIZE_AMOUNT; ++r) {
    for (int i = 1; i < num_inputs; ++i)
      inputs[i].set(100000.0f * ((rand() % 3) - 1.0f));

    processAndCheckFinite(processor, ignore_outputs);
  }

  processAndCheckFinite(processor, ignore_outputs);
}
