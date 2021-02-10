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

#include "wave_frame.h"
#include "futils.h"
#include "fourier_transform.h"

namespace vital {

  void WaveFrame::clear() {
    frequency_ratio = kDefaultFrequencyRatio;
    sample_rate = kDefaultSampleRate;
    for (int i = 0; i < kWaveformSize; ++i) {
      frequency_domain[i] = 0.0f;
      time_domain[i] = 0.0f;
    }
  }

  void WaveFrame::multiply(mono_float value) {
    for (int i = 0; i < kWaveformSize; ++i) {
      time_domain[i] *= value;
      frequency_domain[i] *= value;
    }
  }

  void WaveFrame::loadTimeDomain(float* buffer) {
    for (int i = 0; i < kWaveformSize; ++i)
      time_domain[i] = buffer[i];

    toFrequencyDomain();
  }

  mono_float WaveFrame::getMaxZeroOffset() const {
    mono_float max = 0.0f;
    for (int i = 0; i < kWaveformSize; ++i)
      max = std::max(max, fabsf(time_domain[i]));

    return max;
  }

  void WaveFrame::normalize(bool allow_positive_gain) {
    constexpr mono_float kMaxInverseMult = 0.0000001f;
    mono_float max = getMaxZeroOffset();
    mono_float min = 1.0f;
    if (allow_positive_gain)
      min = kMaxInverseMult;

    mono_float normalization = 1.0f / std::max(min, max);
    for (int i = 0; i < kWaveformSize; ++i)
      time_domain[i] *= normalization;
  }

  void WaveFrame::addFrom(WaveFrame* source) {
    for (int i = 0; i < kWaveformSize; ++i) {
      time_domain[i] += source->time_domain[i];
      frequency_domain[i] += source->frequency_domain[i];
    }
  }

  void WaveFrame::copy(const WaveFrame* other) {
    for (int i = 0; i < kWaveformSize; ++i) {
      frequency_domain[i] = other->frequency_domain[i];
      time_domain[i] = other->time_domain[i];
    }
  }

  void WaveFrame::toFrequencyDomain() {
    float* frequency_data = getFrequencyData();
    memcpy(frequency_data, time_domain, kWaveformSize * sizeof(float));
    memset(frequency_data + kWaveformSize, 0, kWaveformSize * sizeof(float));
    FFT<kWaveformBits>::transform()->transformRealForward(frequency_data);
  }

  void WaveFrame::toTimeDomain() {
    float* frequency_data = getFrequencyData();
    memcpy(time_domain, frequency_domain, 2 * kNumRealComplex * sizeof(float));
    memset(frequency_data + 2 * kNumRealComplex, 0, 2 * kNumExtraComplex * sizeof(float));
    FFT<kWaveformBits>::transform()->transformRealInverse(time_domain);
  }

  void WaveFrame::removedDc() {
    float offset = frequency_domain[0].imag();
    frequency_domain[0] = 0.0f;
    for (int i = 0; i < kWaveformSize; ++i)
      time_domain[i] -= offset;
  }

  PredefinedWaveFrames::PredefinedWaveFrames() {
    createSin(wave_frames_[kSin]);
    createSaturatedSin(wave_frames_[kSaturatedSin]);
    createTriangle(wave_frames_[kTriangle]);
    createSquare(wave_frames_[kSquare]);
    createSaw(wave_frames_[kSaw]);
    createPulse(wave_frames_[kPulse]);
  }

  void PredefinedWaveFrames::createSin(WaveFrame& wave_frame) {
    int half_waveform = WaveFrame::kWaveformSize / 2;
    wave_frame.frequency_domain[1] = half_waveform;
    wave_frame.toTimeDomain();
  }

  void PredefinedWaveFrames::createSaturatedSin(WaveFrame& wave_frame) {
    wave_frame.frequency_domain[1] = WaveFrame::kWaveformSize;
    wave_frame.toTimeDomain();
    for (int i = 0; i < WaveFrame::kWaveformSize; ++i) {
      wave_frame.time_domain[i] = futils::tanh(wave_frame.time_domain[i]);
    }

    wave_frame.toFrequencyDomain();
  }

  void PredefinedWaveFrames::createTriangle(WaveFrame& wave_frame) {
    int section_size = WaveFrame::kWaveformSize / 4;
    for (int i = 0; i < section_size; ++i) {
      mono_float t = i / (section_size * 1.0f);
      wave_frame.time_domain[i] = 1.0f - t;
      wave_frame.time_domain[i + section_size] = -t;
      wave_frame.time_domain[i + 2 * section_size] = t - 1.0f;
      wave_frame.time_domain[i + 3 * section_size] = t;
    }
    wave_frame.toFrequencyDomain();
  }

  void PredefinedWaveFrames::createSquare(WaveFrame& wave_frame) {
    int section_size = WaveFrame::kWaveformSize / 4;
    for (int i = 0; i < section_size; ++i) {
      wave_frame.time_domain[i] = 1.0f;
      wave_frame.time_domain[i + section_size] = -1.0f;
      wave_frame.time_domain[i + 2 * section_size] = -1.0f;
      wave_frame.time_domain[i + 3 * section_size] = 1.0f;
    }
    wave_frame.toFrequencyDomain();
  }

  void PredefinedWaveFrames::createPulse(WaveFrame& wave_frame) {
    int sections = 4;
    int pulse_size = WaveFrame::kWaveformSize / sections;

    for (int i = 0; i < pulse_size; ++i) {
      wave_frame.time_domain[i + (sections - 1) * pulse_size] = 1.0f;
      for (int s = 0; s < sections - 1; ++s)
        wave_frame.time_domain[i + s * pulse_size] = -1.0f;
    }
    wave_frame.toFrequencyDomain();
  }

  void PredefinedWaveFrames::createSaw(WaveFrame& wave_frame) {
    int section_size = WaveFrame::kWaveformSize / 2;
    for (int i = 0; i < section_size; ++i) {
      mono_float t = i / (section_size * 1.0f);
      wave_frame.time_domain[(i + WaveFrame::kWaveformSize / 4) % WaveFrame::kWaveformSize] = t - 1.0f;
      wave_frame.time_domain[(i + section_size + WaveFrame::kWaveformSize / 4) % WaveFrame::kWaveformSize] = t;
    }
    wave_frame.toFrequencyDomain();
  }
} // namespace vital
