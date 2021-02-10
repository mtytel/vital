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
#include "linkwitz_riley_filter.h"

namespace vital {

  class Compressor : public Processor {
    public:
      enum {
        kAudio,
        kUpperThreshold,
        kLowerThreshold,
        kUpperRatio,
        kLowerRatio,
        kOutputGain,
        kAttack,
        kRelease,
        kMix,
        kNumInputs
      };

      enum {
        kAudioOut,
        kNumOutputs
      };

      Compressor(mono_float base_attack_ms_first, mono_float base_release_ms_first,
                 mono_float base_attack_ms_second, mono_float base_release_ms_second);
      virtual ~Compressor() { }

      virtual Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }
      virtual void process(int num_samples) override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;
      void processRms(const poly_float* audio_in, int num_samples);
      void scaleOutput(const poly_float* audio_input, int num_samples);
      void reset(poly_mask reset_mask) override;

      force_inline poly_float getInputMeanSquared() { return input_mean_squared_; }
      force_inline poly_float getOutputMeanSquared() { return output_mean_squared_; }

    protected:
      poly_float computeMeanSquared(const poly_float* audio_in, int num_samples, poly_float mean_squared);

      poly_float input_mean_squared_;
      poly_float output_mean_squared_;
      poly_float high_enveloped_mean_squared_;
      poly_float low_enveloped_mean_squared_;

      poly_float mix_;

      poly_float base_attack_ms_;
      poly_float base_release_ms_;

      poly_float output_mult_;

      JUCE_LEAK_DETECTOR(Compressor)
  };

  class MultibandCompressor : public Processor {
    public:
      enum {
        kAudio,
        kLowUpperRatio,
        kBandUpperRatio,
        kHighUpperRatio,
        kLowLowerRatio,
        kBandLowerRatio,
        kHighLowerRatio,
        kLowUpperThreshold,
        kBandUpperThreshold,
        kHighUpperThreshold,
        kLowLowerThreshold,
        kBandLowerThreshold,
        kHighLowerThreshold,
        kLowOutputGain,
        kBandOutputGain,
        kHighOutputGain,
        kAttack,
        kRelease,
        kEnabledBands,
        kMix,
        kNumInputs
      };

      enum BandOptions {
        kMultiband,
        kLowBand,
        kHighBand,
        kSingleBand,
        kNumBandOptions
      };

      enum OutputType {
        kAudioOut,
        kLowInputMeanSquared,
        kBandInputMeanSquared,
        kHighInputMeanSquared,
        kLowOutputMeanSquared,
        kBandOutputMeanSquared,
        kHighOutputMeanSquared,
        kNumOutputs
      };

      MultibandCompressor();
      virtual ~MultibandCompressor() { }

      virtual Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }
      virtual void process(int num_samples) override;
      void setOversampleAmount(int oversample) override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;
      void setSampleRate(int sample_rate) override;
      void reset(poly_mask reset_mask) override;

    protected:
      void packFilterOutput(LinkwitzRileyFilter* filter, int num_samples, poly_float* dest);
      void packLowBandCompressor(int num_samples, poly_float* dest);
      void writeAllCompressorOutputs(int num_samples, poly_float* dest);
      void writeCompressorOutputs(Compressor* compressor, int num_samples, poly_float* dest);

      bool was_low_enabled_;
      bool was_high_enabled_;

      cr::Output low_band_upper_ratio_;
      cr::Output band_high_upper_ratio_;
      cr::Output low_band_lower_ratio_;
      cr::Output band_high_lower_ratio_;
      cr::Output low_band_upper_threshold_;
      cr::Output band_high_upper_threshold_;
      cr::Output low_band_lower_threshold_;
      cr::Output band_high_lower_threshold_;

      cr::Output low_band_output_gain_;
      cr::Output band_high_output_gain_;

      LinkwitzRileyFilter low_band_filter_;
      LinkwitzRileyFilter band_high_filter_;

      Compressor low_band_compressor_;
      Compressor band_high_compressor_;

      JUCE_LEAK_DETECTOR(MultibandCompressor)
  };
} // namespace vital

