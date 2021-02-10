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
#include "synth_filter.h"

#include "one_pole_filter.h"

namespace vital {

  class PhaserFilter : public Processor, public SynthFilter {
    public:
      static constexpr mono_float kMinResonance = 0.0f;
      static constexpr mono_float kMaxResonance = 1.0f;
      static constexpr mono_float kMinCutoff = 1.0f;
      static constexpr mono_float kClearRatio = 20.0f;

      static constexpr int kPeakStage = 4;
      static constexpr int kMaxStages = 3 * kPeakStage;

      PhaserFilter(bool clean);
      virtual ~PhaserFilter() { }

      virtual Processor* clone() const override { return new PhaserFilter(*this); }
      virtual void process(int num_samples) override;
      void processWithInput(const poly_float* audio_in, int num_samples) override;

      void setupFilter(const FilterState& filter_state) override;

      void reset(poly_mask reset_mask) override;
      void hardReset() override;
      void setClean(bool clean) { clean_ = clean; }

      poly_float getResonance() { return resonance_; }
      poly_float getDrive() { return drive_; }
      poly_float getPeak1Amount() { return peak1_amount_; }
      poly_float getPeak3Amount() { return peak3_amount_; }
      poly_float getPeak5Amount() { return peak5_amount_; }

    private:
      template <poly_float(*saturateResonance)(poly_float), poly_float(*saturateInput)(poly_float)>
      void process(const poly_float* audio_in, int num_samples) {
        poly_float current_resonance = resonance_;
        poly_float current_drive = drive_;
        poly_float current_peak1 = peak1_amount_;
        poly_float current_peak3 = peak3_amount_;
        poly_float current_peak5 = peak5_amount_;

        filter_state_.loadSettings(this);
        setupFilter(filter_state_);

        poly_mask reset_mask = getResetMask(kReset);
        if (reset_mask.anyMask()) {
          reset(reset_mask);

          current_resonance = utils::maskLoad(current_resonance, resonance_, reset_mask);
          current_drive = utils::maskLoad(current_drive, drive_, reset_mask);
          current_peak1 = utils::maskLoad(current_peak1, peak1_amount_, reset_mask);
          current_peak3 = utils::maskLoad(current_peak3, peak3_amount_, reset_mask);
          current_peak5 = utils::maskLoad(current_peak5, peak5_amount_, reset_mask);
        }

        mono_float tick_increment = 1.0f / num_samples;
        poly_float delta_resonance = (resonance_ - current_resonance) * tick_increment;
        poly_float delta_drive = (drive_ - current_drive) * tick_increment;
        poly_float delta_peak1 = (peak1_amount_ - current_peak1) * tick_increment;
        poly_float delta_peak3 = (peak3_amount_ - current_peak3) * tick_increment;
        poly_float delta_peak5 = (peak5_amount_ - current_peak5) * tick_increment;

        poly_float* audio_out = output()->buffer;
        const CoefficientLookup* coefficient_lookup = getCoefficientLookup();
        const poly_float* midi_cutoff_buffer = filter_state_.midi_cutoff_buffer;
        poly_float base_midi = midi_cutoff_buffer[num_samples - 1];
        poly_float base_frequency = utils::midiNoteToFrequency(base_midi) * (1.0f / getSampleRate());

        for (int i = 0; i < num_samples; ++i) {
          poly_float midi_delta = midi_cutoff_buffer[i] - base_midi;
          poly_float frequency = utils::min(base_frequency * futils::midiOffsetToRatio(midi_delta), 1.0f);
          poly_float coefficient = coefficient_lookup->cubicLookup(frequency);

          current_resonance += delta_resonance;
          current_drive += delta_drive;
          current_peak1 += delta_peak1;
          current_peak3 += delta_peak3;
          current_peak5 += delta_peak5;

          tick<saturateResonance, saturateInput>(audio_in[i], coefficient, current_resonance, current_drive,
                                                 current_peak1, current_peak3, current_peak5);

          audio_out[i] = (audio_in[i] + invert_mult_ * allpass_output_) * 0.5f;
        }
      }

      template <poly_float(*saturateResonance)(poly_float), poly_float(*saturateInput)(poly_float)>
      force_inline void tick(poly_float audio_in, poly_float coefficient,
                             poly_float resonance, poly_float drive,
                             poly_float peak1, poly_float peak3, poly_float peak5) {
        poly_float filter_state_lows = remove_lows_stage_.tickBasic(allpass_output_,
                                                                    utils::min(coefficient * kClearRatio, 0.9f));
        poly_float filter_state_highs = remove_highs_stage_.tickBasic(filter_state_lows,
                                                                      coefficient * (1.0f / kClearRatio));
        poly_float filter_state = saturateResonance(resonance * (filter_state_lows - filter_state_highs));

        poly_float filter_input = utils::mulAdd(drive * audio_in, invert_mult_, filter_state);
        poly_float all_pass_input = saturateInput(filter_input);
        poly_float stage_out;

        for (int i = 0; i < kPeakStage; ++i) {
          stage_out = stages_[i].tickBasic(all_pass_input, coefficient);
          all_pass_input = utils::mulAdd(all_pass_input, stage_out, -2.0f);
        }

        poly_float peak1_out = all_pass_input;

        for (int i = kPeakStage; i < 2 * kPeakStage; ++i) {
          stage_out = stages_[i].tickBasic(all_pass_input, coefficient);
          all_pass_input = utils::mulAdd(all_pass_input, stage_out, -2.0f);
        }

        poly_float peak3_out = all_pass_input;

        for (int i = 2 * kPeakStage; i < 3 * kPeakStage; ++i) {
          stage_out = stages_[i].tickBasic(all_pass_input, coefficient);
          all_pass_input = utils::mulAdd(all_pass_input, stage_out, -2.0f);
        }

        poly_float peak5_out = all_pass_input;
        poly_float all_pass_output_1_3 = utils::mulAdd(peak1 * peak1_out, peak3, peak3_out);
        allpass_output_ = utils::mulAdd(all_pass_output_1_3, peak5, peak5_out);
      }

      bool clean_;

      poly_float resonance_;
      poly_float drive_;
      poly_float peak1_amount_;
      poly_float peak3_amount_;
      poly_float peak5_amount_;
      poly_float invert_mult_;

      OnePoleFilter<> stages_[kMaxStages];

      OnePoleFilter<> remove_lows_stage_;
      OnePoleFilter<> remove_highs_stage_;

      poly_float allpass_output_;

      JUCE_LEAK_DETECTOR(PhaserFilter)
  };
} // namespace vital

