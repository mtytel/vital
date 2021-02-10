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
#include "one_pole_filter.h"

namespace vital {

  class StereoMemory;

  class Reverb : public Processor {
    public:
      static constexpr mono_float kT60Amplitude = 0.001f;
      static constexpr mono_float kAllpassFeedback = 0.6f;
      static constexpr float kMinDelay = 3.0f;

      static constexpr int kBaseSampleRate = 44100;
      static constexpr int kDefaultSampleRate = 88200;
      static constexpr int kNetworkSize = 16;
      static constexpr int kBaseFeedbackBits = 14;
      static constexpr int kExtraLookupSample = 4;
      static constexpr int kBaseAllpassBits = 10;
      static constexpr int kNetworkContainers = kNetworkSize / poly_float::kSize;
      static constexpr int kMinSizePower = -3;
      static constexpr int kMaxSizePower = 1;
      static constexpr float kSizePowerRange = kMaxSizePower - kMinSizePower;

      static const poly_int kAllpassDelays[kNetworkContainers];
      static const poly_float kFeedbackDelays[kNetworkContainers];

      enum {
        kAudio,
        kDecayTime,
        kPreLowCutoff,
        kPreHighCutoff,
        kLowCutoff,
        kLowGain,
        kHighCutoff,
        kHighGain,
        kChorusAmount,
        kChorusFrequency,
        kStereoWidth,
        kSize,
        kDelay,
        kWet,
        kNumInputs
      };

      Reverb();
      virtual ~Reverb() { }

      void process(int num_samples) override;
      void processWithInput(const poly_float* audio_in, int num_samples) override;
      force_inline float getSampleRateRatio(int sample_rate) { return sample_rate / (1.0f * kBaseSampleRate); }
      force_inline int getBufferScale(int sample_rate) {
        int scale = 1;
        float ratio = getSampleRateRatio(sample_rate);
        for (; scale < ratio; scale *= 2)
          ;
        return scale;
      }
      void setSampleRate(int sample_rate) override;
      void setOversampleAmount(int oversample_amount) override;
      void setupBuffersForSampleRate(int sample_rate);
      void hardReset() override;

      force_inline poly_float readFeedback(const mono_float* const* lookups, poly_float offset) {
        poly_float write_offset = poly_float(write_index_) - offset;
        poly_float floored_offset = utils::floor(write_offset);
        poly_float t = write_offset - floored_offset;
        matrix interpolation_matrix = utils::getPolynomialInterpolationMatrix(t);
        poly_int indices = utils::toInt(floored_offset) & feedback_mask_;
        matrix value_matrix = utils::getValueMatrix(lookups, indices);
        value_matrix.transpose();
        return interpolation_matrix.multiplyAndSumRows(value_matrix);
      }

      force_inline poly_float readAllpass(const mono_float* lookup, poly_int offset) {
        poly_int indices = (poly_int(write_index_ * poly_float::kSize) - offset) & allpass_mask_;
        return poly_float(lookup[indices[0]], lookup[indices[1]], lookup[indices[2]], lookup[indices[3]]);
      }

      force_inline void wrapFeedbackBuffer(mono_float* buffer) {
        buffer[0] = buffer[max_feedback_size_];
        buffer[max_feedback_size_ + 1] = buffer[1];
        buffer[max_feedback_size_ + 2] = buffer[2];
        buffer[max_feedback_size_ + 3] = buffer[3];
      }

      virtual Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }

    private:
      std::unique_ptr<StereoMemory> memory_;

      std::unique_ptr<poly_float[]> allpass_lookups_[kNetworkContainers];
      std::unique_ptr<mono_float[]> feedback_memories_[kNetworkSize];
      mono_float* feedback_lookups_[kNetworkSize];
      poly_float decays_[kNetworkContainers];

      OnePoleFilter<> low_shelf_filters_[kNetworkContainers];
      OnePoleFilter<> high_shelf_filters_[kNetworkContainers];

      OnePoleFilter<> low_pre_filter_;
      OnePoleFilter<> high_pre_filter_;

      poly_float low_pre_coefficient_;
      poly_float high_pre_coefficient_;
      poly_float low_coefficient_;
      poly_float low_amplitude_;
      poly_float high_coefficient_;
      poly_float high_amplitude_;

      mono_float chorus_phase_;
      poly_float chorus_amount_;
      poly_float feedback_;
      poly_float damping_;
      poly_float sample_delay_;
      poly_float sample_delay_increment_;
      poly_float dry_;
      poly_float wet_;
      int write_index_;

      int max_allpass_size_;
      int max_feedback_size_;
      int feedback_mask_;
      poly_mask allpass_mask_;
      int poly_allpass_mask_;

      JUCE_LEAK_DETECTOR(Reverb)
  };
} // namespace vital

