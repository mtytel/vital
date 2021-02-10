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

#include "common.h"

#include <complex>

namespace vital {

  class WaveFrame {
    public:
      static constexpr int kWaveformBits = 11;
      static constexpr int kWaveformSize = 1 << kWaveformBits;
      static constexpr int kNumRealComplex = kWaveformSize / 2 + 1;
      static constexpr int kNumExtraComplex = kWaveformSize - kNumRealComplex;
      static constexpr float kDefaultFrequencyRatio = 1.0f;
      static constexpr float kDefaultSampleRate = 44100.0f;

      WaveFrame() : index(0), frequency_ratio(kDefaultFrequencyRatio), sample_rate(kDefaultSampleRate),
                    time_domain(), frequency_domain() { }

      mono_float getMaxZeroOffset() const;

      void normalize(bool allow_positive_gain = false);
      void clear();
      void setFrequencyRatio(float ratio) { frequency_ratio = ratio; }
      void setSampleRate(float rate) { sample_rate = rate; }
      void multiply(mono_float value);
      void loadTimeDomain(float* buffer);
      void addFrom(WaveFrame* source);
      void copy(const WaveFrame* other);
      void toFrequencyDomain();
      void toTimeDomain();
      void removedDc();

      int index;
      float frequency_ratio;
      float sample_rate;
      mono_float time_domain[2 * kWaveformSize];
      std::complex<float> frequency_domain[kWaveformSize];

      float* getFrequencyData() { return reinterpret_cast<float*>(frequency_domain); }

      JUCE_LEAK_DETECTOR(WaveFrame)
  };

  class PredefinedWaveFrames {
    public:
      enum Shape {
        kSin,
        kSaturatedSin,
        kTriangle,
        kSquare,
        kPulse,
        kSaw,
        kNumShapes
      };

      PredefinedWaveFrames();

      static const WaveFrame* getWaveFrame(Shape shape) { return &instance()->wave_frames_[shape]; }

    private:
      static const PredefinedWaveFrames* instance() {
        static const PredefinedWaveFrames wave_frames;
        return &wave_frames;
      }

      void createSin(WaveFrame& wave_frame);
      void createSaturatedSin(WaveFrame& wave_frame);
      void createTriangle(WaveFrame& wave_frame);
      void createSquare(WaveFrame& wave_frame);
      void createPulse(WaveFrame& wave_frame);
      void createSaw(WaveFrame& wave_frame);

      WaveFrame wave_frames_[kNumShapes];
  };
} // namespace vital

