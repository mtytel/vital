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

#include "processor_router.h"

#include "feedback.h"
#include "synth_constants.h"

#include <algorithm>
#include <vector>

namespace vital {

  ProcessorRouter::ProcessorRouter(int num_inputs, int num_outputs, bool control_rate) :
      Processor(num_inputs, num_outputs, control_rate),
      global_order_(new CircularQueue<Processor*>(kMaxModulationConnections)),
      global_reorder_(new CircularQueue<Processor*>(kMaxModulationConnections)),
      local_order_(kMaxModulationConnections),
      global_feedback_order_(new std::vector<const Feedback*>()),
      global_changes_(new int(0)), local_changes_(0),
      dependencies_(new CircularQueue<const Processor*>(kMaxModulationConnections)),
      dependencies_visited_(new CircularQueue<const Processor*>(kMaxModulationConnections)),
      dependency_inputs_(new CircularQueue<const Processor*>(kMaxModulationConnections)) { }

  ProcessorRouter::ProcessorRouter(const ProcessorRouter& original) :
      Processor(original), global_order_(original.global_order_), global_reorder_(original.global_reorder_),
      global_feedback_order_(original.global_feedback_order_),
      global_changes_(original.global_changes_),
      local_changes_(original.local_changes_) {
    local_order_.reserve(global_order_->capacity());
    local_order_.assign(global_order_->size(), 0);
    local_feedback_order_.assign(global_feedback_order_->size(), nullptr);

    int num_processors = global_order_->size();
    for (int i = 0; i < num_processors; ++i) {
      const Processor* next = global_order_->at(i);
      std::unique_ptr<Processor> clone(next->clone());
      local_order_[i] = clone.get();
      processors_[next] = { 0, std::move(clone) };
    }

    int num_feedbacks = static_cast<int>(global_feedback_order_->size());
    for (int i = 0; i < num_feedbacks; ++i) {
      const Feedback* next = global_feedback_order_->at(i);
      std::unique_ptr<Feedback> clone((Feedback*)(next->clone()));
      local_feedback_order_[i] = clone.get();
      feedback_processors_[next] = { 0, std::move(clone) };
    }
  }

  ProcessorRouter::~ProcessorRouter() { }

  void ProcessorRouter::process(int num_samples) {
    if (shouldUpdate())
      updateAllProcessors();

    // First make sure all the Feedback loops are ready to be read.
    int num_feedbacks = static_cast<int>(local_feedback_order_.size());
    for (int i = 0; i < num_feedbacks; ++i)
      local_feedback_order_[i]->refreshOutput(num_samples);

    // Run all the main processors.
    int normal_samples = std::max(1, num_samples / getOversampleAmount());
    for (Processor* processor : local_order_) {
      if (processor->enabled()) {
        int processor_samples = normal_samples * processor->getOversampleAmount();

        VITAL_ASSERT(processor->checkInputAndOutputSize(processor_samples));
        processor->process(processor_samples);
        VITAL_ASSERT(utils::isFinite(processor->output()->buffer, processor->isControlRate() ? 0 : processor_samples));
      }
    }

    // Store the outputs into the Feedback objects for next time.
    for (int i = 0; i < num_feedbacks; ++i) {
      if (global_feedback_order_->at(i)->enabled())
        local_feedback_order_[i]->process(num_samples);
    }
  }

  void ProcessorRouter::init() {
    Processor::init();

    for (Processor* processor : local_order_)
      processor->init();
  }

  void ProcessorRouter::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    if (shouldUpdate())
      updateAllProcessors();

    int num_processors = local_order_.size();
    for (int i = 0; i < num_processors; ++i)
      local_order_[i]->setSampleRate(sample_rate);

    int num_feedbacks = static_cast<int>(local_feedback_order_.size());
    for (int i = 0; i < num_feedbacks; ++i)
      local_feedback_order_[i]->setSampleRate(sample_rate);
  }

  void ProcessorRouter::setOversampleAmount(int oversample) {
    Processor::setOversampleAmount(oversample);
    if (shouldUpdate())
      updateAllProcessors();

    for (auto& idle_processor : idle_processors_)
      idle_processor.second->setOversampleAmount(oversample);

    int num_processors = local_order_.size();
    for (int i = 0; i < num_processors; ++i)
      local_order_[i]->setOversampleAmount(oversample);

    int num_feedbacks = static_cast<int>(local_feedback_order_.size());
    for (int i = 0; i < num_feedbacks; ++i)
      local_feedback_order_[i]->setOversampleAmount(oversample);
  }

  void ProcessorRouter::addProcessor(Processor* processor) {
    VITAL_ASSERT(processor->router() == nullptr);
    global_order_->ensureSpace();
    global_reorder_->ensureCapacity(global_order_->capacity());
    local_order_.ensureSpace();
    addProcessorRealTime(processor);
  }

  void ProcessorRouter::addProcessorRealTime(Processor* processor) {
    VITAL_ASSERT(processor->router() == nullptr);
    (*global_changes_)++;
    local_changes_++;

    processor->router(this);
    if (getOversampleAmount() > 1)
      processor->setOversampleAmount(getOversampleAmount());

    global_order_->push_back(processor);
    processors_[processor] = { 0, std::unique_ptr<Processor>(processor) };
    local_order_.push_back(processor);

    for (int i = 0; i < processor->numInputs(); ++i)
      connect(processor, processor->input(i)->source, i);
  }

  void ProcessorRouter::addIdleProcessor(Processor *processor) {
    processor->router(this);
    idle_processors_[processor] = std::unique_ptr<Processor>(processor);
  }

  void ProcessorRouter::removeProcessor(Processor* processor) {
    for (int i = 0; i < processor->numInputs(); ++i)
      disconnect(processor, processor->input(i)->source);

    VITAL_ASSERT(processor->router() == this);
    (*global_changes_)++;
    local_changes_++;
    global_order_->remove(processor);
    local_order_.remove(processor);

    Processor* old_processor = processors_[processor].second.release();
    VITAL_ASSERT(old_processor == processor);
    UNUSED(old_processor);
    processor->router(nullptr);
    processors_.erase(processor);
  }

  void ProcessorRouter::connect(Processor* destination, const Output* source, int index) {
    if (isDownstream(destination, source->owner)) {
      // We are introducing a cycle so insert a Feedback node.
      Feedback* feedback = new cr::Feedback();
      feedback->plug(source);
      destination->plug(feedback, index);
      addFeedback(feedback);
    }
    else {
      // Not introducing a cycle so just make sure _destination_ is in order.
      reorder(destination);
    }
  }

  void ProcessorRouter::disconnect(const Processor* destination, const Output* source) {
    if (isDownstream(destination, source->owner)) {
      // We're fine unless there is a cycle and need to delete a Feedback node.
      for (int i = 0; i < destination->numInputs(); ++i) {
        const Processor* owner = destination->input(i)->source->owner;

        if (feedback_processors_.find(owner) != feedback_processors_.end()) {
          Feedback* feedback = feedback_processors_[owner].second.get();
          if (feedback->input()->source == source)
            removeFeedback(feedback);
          destination->input(i)->source = &Processor::null_source_;
        }
      }
    }
  }

  void ProcessorRouter::reorder(Processor* processor) {
    (*global_changes_)++;
    local_changes_++;

    getDependencies(processor);
    if (dependencies_->size() == 0) {
      if (router_)
        router_->reorder(processor);

      return;
    }

    int num_processors = static_cast<int>(processors_.size());
    global_reorder_->clear();

    for (int i = 0; i < num_processors; ++i) {
      Processor* current_processor = global_order_->at(i);
      if (current_processor != processor && dependencies_->contains(current_processor))
        global_reorder_->push_back(current_processor);
    }

    if (processors_.find(processor) != processors_.end())
      global_reorder_->push_back(processor);

    for (int i = 0; i < num_processors; ++i) {
      Processor* current_processor = global_order_->at(i);
      if (current_processor != processor && !dependencies_->contains(current_processor))
        global_reorder_->push_back(current_processor);
    }

    for (int i = 0; i < num_processors; ++i)
      global_order_->at(i) = global_reorder_->at(i);

    if (router_)
      router_->reorder(processor);
  }

  bool ProcessorRouter::isDownstream(const Processor* first, const Processor* second) const {
    getDependencies(second);
    return dependencies_->contains(first);
  }

  bool ProcessorRouter::areOrdered(const Processor* first, const Processor* second) const {
    const Processor* first_context = getContext(first);
    const Processor* second_context = getContext(second);

    if (first_context && second_context) {
      size_t num_processors = global_order_->size();
      for (size_t i = 0; i < num_processors; ++i) {
        if (global_order_->at(i) == first_context)
          return true;
        else if (global_order_->at(i) == second_context)
          return false;
      }
    }
    else if (router_)
      return router_->areOrdered(first, second);

    return true;
  }

  bool ProcessorRouter::isPolyphonic(const Processor* processor) const {
    if (router_)
      return router_->isPolyphonic(this);
    return false;
  }

  ProcessorRouter* ProcessorRouter::getMonoRouter() {
    if (isPolyphonic(this))
      return router_->getMonoRouter();
    return this;
  }

  ProcessorRouter* ProcessorRouter::getPolyRouter() {
    return this;
  }

  void ProcessorRouter::resetFeedbacks(poly_mask reset_mask) {
    for (auto feedback : local_feedback_order_)
      feedback->reset(reset_mask);
  }

  void ProcessorRouter::addFeedback(Feedback* feedback) {
    feedback->router(this);
    global_feedback_order_->push_back(feedback);
    local_feedback_order_.push_back(feedback);
    feedback_processors_[feedback] = { 0, std::unique_ptr<Feedback>(feedback) };
  }

  void ProcessorRouter::removeFeedback(Feedback* feedback) {
    (*global_changes_)++;
    local_changes_++;

    auto pos = std::find(global_feedback_order_->begin(), global_feedback_order_->end(), feedback);
    VITAL_ASSERT(pos != global_feedback_order_->end());
    global_feedback_order_->erase(pos, pos + 1);

    auto local_pos = std::find(local_feedback_order_.begin(), local_feedback_order_.end(), feedback);
    VITAL_ASSERT(local_pos != local_feedback_order_.end());
    local_feedback_order_.erase(local_pos, local_pos + 1);

    feedback_processors_.erase(feedback);
  }

  void ProcessorRouter::updateAllProcessors() {
    if (local_changes_ == *global_changes_)
      return;

    createAddedProcessors();
    deleteRemovedProcessors();

    local_changes_ = *global_changes_;
  }

  void ProcessorRouter::createAddedProcessors() {
    if (global_order_->size() > local_order_.capacity())
      local_order_.reserve(global_order_->capacity());
   
    local_order_.assign(global_order_->size(), nullptr);
    local_feedback_order_.assign(global_feedback_order_->size(), nullptr);

    int num_processors = global_order_->size();
    for (int i = 0; i < num_processors; ++i) {
      Processor* next = global_order_->at(i);
      if (next->hasState()) {
        if (processors_.count(next) == 0)
          processors_[next] = { 0, std::unique_ptr<Processor>(next->clone()) };
        local_order_[i] = processors_[next].second.get();
      }
      else
        local_order_[i] = next;
    }

    int num_feedbacks = static_cast<int>(global_feedback_order_->size());
    for (int i = 0; i < num_feedbacks; ++i) {
      const Feedback* next = global_feedback_order_->at(i);
      if (feedback_processors_.count(next) == 0)
        feedback_processors_[next] = { 0, std::unique_ptr<Feedback>((Feedback*)(next->clone())) };
      local_feedback_order_[i] = feedback_processors_[next].second.get();
    }
  }

  void ProcessorRouter::deleteRemovedProcessors() {
    for (const Processor* global_processor : *global_order_)
      processors_[global_processor].first = *global_changes_;

    for (auto iter = processors_.cbegin(); iter != processors_.cend();) {
      if (iter->second.first != *global_changes_)
        iter = processors_.erase(iter);
      else
        iter++;
    }

    for (const Feedback* global_feedback : *global_feedback_order_)
      feedback_processors_[global_feedback].first = *global_changes_;

    for (auto iter = feedback_processors_.cbegin(); iter != feedback_processors_.cend();) {
      if (iter->second.first != *global_changes_)
        iter = feedback_processors_.erase(iter);
      else
        iter++;
    }

    int num_feedbacks = static_cast<int>(global_feedback_order_->size());
    local_feedback_order_.clear();
    for (int i = 0; i < num_feedbacks; ++i) {
      const Feedback* next = global_feedback_order_->at(i);
      local_feedback_order_.push_back(feedback_processors_[next].second.get());
    }
  }

  const Processor* ProcessorRouter::getContext(const Processor* processor) const {
    const Processor* context = processor;
    while (context && processors_.find(context) == processors_.end() &&
           idle_processors_.find(context) == idle_processors_.end()) {
      context = context->router();
    }

    return context;
  }

  Processor* ProcessorRouter::getLocalProcessor(const Processor* global_processor) {
    return processors_[global_processor].second.get();
  }

  void ProcessorRouter::getDependencies(const Processor* processor) const {
    dependencies_->clear();
    dependencies_visited_->clear();
    dependency_inputs_->clear();
    const Processor* context = getContext(processor);

    dependency_inputs_->ensureSpace();
    dependency_inputs_->push_back(processor);
    for (size_t i = 0; i < dependency_inputs_->size(); ++i) {
      const Processor* dependency = getContext(dependency_inputs_->at(i));

      if (dependency) {
        if (!dependencies_->contains(dependency)) {
          dependencies_->ensureSpace();
          dependencies_->push_back(dependency);
        }

        for (int j = 0; j < dependency_inputs_->at(i)->numInputs(); ++j) {
          const Input* input = dependency_inputs_->at(i)->ownedInput(j);
          if (input->source && input->source->owner && !dependencies_visited_->contains(input->source->owner)) {
            dependency_inputs_->ensureSpace();
            dependency_inputs_->push_back(input->source->owner);

            if (!dependencies_visited_->contains(input->source->owner)) {
              dependencies_visited_->ensureSpace();
              dependencies_visited_->push_back(input->source->owner);
            }
          }
        }
      }
    }

    dependencies_->removeAll(context);
  }
} // namespace vital
