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
#include "line_generator.h"

namespace vital {

  class SynthLfo : public Processor {
    public:
      enum {
        kFrequency,
        kPhase,
        kAmplitude,
        kNoteTrigger,
        kSyncType,
        kSmoothMode,
        kFade,
        kSmoothTime,
        kStereoPhase,
        kDelay,
        kNoteCount,
        kNumInputs
      };

      enum {
        kValue,
        kOscPhase,
        kOscFrequency,
        kNumOutputs
      };

      enum SyncType {
        kTrigger,
        kSync,
        kEnvelope,
        kSustainEnvelope,
        kLoopPoint,
        kLoopHold,
        kNumSyncTypes
      };

      enum {
        kTime,
        kTempo,
        kDottedTempo,
        kTripletTempo,
        kKeytrack,
        kNumSyncOptions
      };

      struct LfoState {
        poly_float delay_time_passed = 0.0f;
        poly_float fade_amplitude = 0.0f;
        poly_float smooth_value = 0.0f;
        poly_float fade_amount = 0.0f;
        poly_float offset = 0.0f;
        poly_float phase = 0.0f;
      };

      static constexpr mono_float kMaxPower = 20.0f;
      static constexpr float kHalfLifeRatio = 0.2f;
      static constexpr float kMinHalfLife = 0.0002f;

      force_inline poly_float getValueAtPhase(mono_float* buffer, poly_float resolution,
                                              poly_int max_index, poly_float phase) {
        poly_float boost = utils::clamp(phase * resolution, 0.0f, resolution);
        poly_int indices = utils::clamp(utils::toInt(boost), 0, max_index);
        poly_float t = boost - utils::toFloat(indices);

        matrix interpolation_matrix = utils::getCatmullInterpolationMatrix(t);
        matrix value_matrix = utils::getValueMatrix(buffer, indices);
        value_matrix.transpose();

        return interpolation_matrix.multiplyAndSumRows(value_matrix);
      }

      force_inline poly_float getValueAtPhase(poly_float phase) {
        int resolution = source_->resolution();
        return getValueAtPhase(source_->getCubicInterpolationBuffer(), resolution, resolution - 1, phase);
      }

      SynthLfo(LineGenerator* source);

      force_inline poly_mask getReleaseMask() {
        poly_mask trigger_mask = input(kNoteTrigger)->source->trigger_mask;
        poly_float trigger_value = input(kNoteTrigger)->source->trigger_value;
        return trigger_mask & poly_float::equal(trigger_value, kVoiceOff);
      }

      virtual Processor* clone() const override { return new SynthLfo(*this); }

      void process(int num_samples) override;
      void correctToTime(double seconds);

    protected:
      void processTrigger();
      void processControlRate(int num_samples);

      poly_float processAudioRateEnvelope(int num_samples, poly_float current_phase,
                                          poly_float current_offset, poly_float delta_offset);
      poly_float processAudioRateSustainEnvelope(int num_samples, poly_float current_phase,
                                                 poly_float current_offset, poly_float delta_offset);
      poly_float processAudioRateLfo(int num_samples, poly_float current_phase,
                                     poly_float current_offset, poly_float delta_offset);
      poly_float processAudioRateLoopPoint(int num_samples, poly_float current_phase,
                                           poly_float current_offset, poly_float delta_offset);
      poly_float processAudioRateLoopHold(int num_samples, poly_float current_phase,
                                          poly_float current_offset, poly_float delta_offset);
      void processAudioRate(int num_samples);

      bool was_control_rate_;
      LfoState control_rate_state_;
      LfoState audio_rate_state_;

      poly_mask held_mask_;
      poly_int trigger_sample_;
      poly_float trigger_delay_;
      LineGenerator* source_;

      std::shared_ptr<double> sync_seconds_;

      JUCE_LEAK_DETECTOR(SynthLfo)
  };
} // namespace vital

