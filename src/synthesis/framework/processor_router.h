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

#pragma once

#include "processor.h"
#include "circular_queue.h"

#include <map>
#include <set>
#include <vector>

namespace vital {

  class Feedback;

  class ProcessorRouter : public Processor {
    public:
      ProcessorRouter(int num_inputs = 0, int num_outputs = 0, bool control_rate = false);
      ProcessorRouter(const ProcessorRouter& original);

      virtual ~ProcessorRouter();

      virtual Processor* clone() const override {
        return new ProcessorRouter(*this);
      }

      virtual void process(int num_samples) override;
      virtual void init() override;
      virtual void setSampleRate(int sample_rate) override;
      virtual void setOversampleAmount(int oversample) override;

      virtual void addProcessor(Processor* processor);
      virtual void addProcessorRealTime(Processor* processor);
      virtual void addIdleProcessor(Processor* processor);
      virtual void removeProcessor(Processor* processor);

      // Any time new dependencies are added into the ProcessorRouter graph, we
      // should call _connect_ on the destination Processor and source Output.
      void connect(Processor* destination, const Output* source, int index);
      void disconnect(const Processor* destination, const Output* source);
      bool isDownstream(const Processor* first, const Processor* second) const;
      bool areOrdered(const Processor* first, const Processor* second) const;

      virtual bool isPolyphonic(const Processor* processor) const;

      virtual ProcessorRouter* getMonoRouter();
      virtual ProcessorRouter* getPolyRouter();
      virtual void resetFeedbacks(poly_mask reset_mask);

    protected:
      // When we create a cycle into the ProcessorRouter graph, we must insert
      // a Feedback node and add it here.
      virtual void addFeedback(Feedback* feedback);
      virtual void removeFeedback(Feedback* feedback);

      // Makes sure _processor_ runs in a topologically sorted order in
      // relation to all other Processors in _this_.
      void reorder(Processor* processor);

      // Ensures our local copies of all processors and feedback processors match the master order.
      virtual void updateAllProcessors();

      force_inline bool shouldUpdate() { return local_changes_ != *global_changes_; }

      // Will create local copies of added processors. 
      virtual void createAddedProcessors();

      // Will delete local copies of removed processors. 
      virtual void deleteRemovedProcessors();

      // Returns the ancestor of _processor_ which is a child of _this_.
      // Returns null if _processor_ is not a descendant of _this_.
      const Processor* getContext(const Processor* processor) const;
      void getDependencies(const Processor* processor) const;

      // Returns the processor for this voice from the globally created one.
      Processor* getLocalProcessor(const Processor* global_processor);

      std::shared_ptr<CircularQueue<Processor*>> global_order_;
      std::shared_ptr<CircularQueue<Processor*>> global_reorder_;
      CircularQueue<Processor*> local_order_;
      std::map<const Processor*, std::pair<int, std::unique_ptr<Processor>>> processors_;
      std::map<const Processor*, std::unique_ptr<Processor>> idle_processors_;

      std::shared_ptr<std::vector<const Feedback*>> global_feedback_order_;
      std::vector<Feedback*> local_feedback_order_;
      std::map<const Processor*, std::pair<int, std::unique_ptr<Feedback>>> feedback_processors_;

      std::shared_ptr<int> global_changes_;
      int local_changes_;

      std::shared_ptr<CircularQueue<const Processor*>> dependencies_;
      std::shared_ptr<CircularQueue<const Processor*>> dependencies_visited_;
      std::shared_ptr<CircularQueue<const Processor*>> dependency_inputs_;

      JUCE_LEAK_DETECTOR(ProcessorRouter)
  };
} // namespace vital

