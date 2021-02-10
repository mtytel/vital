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

#include "JuceHeader.h"

#include "common.h"
#include "futils.h"
#include "utils.h"
#include "wave_frame.h"

namespace vital {

  class Wavetable {
    public:
      static constexpr int kFrequencyBins = WaveFrame::kWaveformBits;
      static constexpr int kWaveformSize = WaveFrame::kWaveformSize;
      static constexpr int kExtraValues = 3;
      static constexpr int kNumHarmonics = kWaveformSize / 2 + 1;
      static constexpr int kPolyFrequencySize = 2 * kNumHarmonics / poly_float::kSize + 2;

      struct WavetableData {
        WavetableData(int frames, int table_version) :
            num_frames(frames), frequency_ratio(1.0f), sample_rate(kDefaultSampleRate), version(table_version) { }

        int num_frames;
        mono_float frequency_ratio;
        mono_float sample_rate;
        int version;
        std::unique_ptr<mono_float[][kWaveformSize]> wave_data;
        std::unique_ptr<poly_float[][kPolyFrequencySize]> frequency_amplitudes;
        std::unique_ptr<poly_float[][kPolyFrequencySize]> normalized_frequencies;
        std::unique_ptr<poly_float[][kPolyFrequencySize]> phases;
      };

      static constexpr const mono_float* null_waveform() { return kZeroWaveform; }

      Wavetable(int max_frames);

      void loadDefaultWavetable();
      void setNumFrames(int num_frames);
      void setFrequencyRatio(float frequency_ratio);
      void setSampleRate(float rate);
      std::string getName() { return name_; }
      std::string getAuthor() { return author_; }
      void setName(const std::string& name) { name_ = name; }
      void setAuthor(const std::string& author) { author_ = author; }

      static force_inline mono_float getFrequencyFloatBin(mono_float phase_increment) {
        return futils::log2(1.0f / phase_increment);
      }

      static force_inline int getFrequencyBin(mono_float phase_increment) {
        int num_waves = 1.0f / phase_increment;
        return utils::iclamp(utils::ilog2(num_waves), 0, kFrequencyBins - 1);
      }

      force_inline int clampFrame(int frame) {
        return std::min(frame, current_data_->num_frames - 1);
      }

      force_inline const WavetableData* getAllData() {
        return current_data_;
      }

      force_inline mono_float* getBuffer(int frame_index) {
        return current_data_->wave_data[clampFrame(frame_index)];
      }

      force_inline poly_float* getFrequencyAmplitudes(int frame_index) {
        return current_data_->frequency_amplitudes[clampFrame(frame_index)];
      }

      force_inline poly_float* getNormalizedFrequencies(int frame_index) {
        return current_data_->normalized_frequencies[clampFrame(frame_index)];
      }

      force_inline int getVersion() {
        return current_data_->version;
      }

      force_inline int clampActiveFrame(int frame) {
        return std::min(frame, active_audio_data_.load()->num_frames - 1);
      }

      force_inline float getActiveFrequencyRatio() const {
        return active_audio_data_.load()->frequency_ratio;
      };

      force_inline float getActiveSampleRate() const {
        return active_audio_data_.load()->sample_rate;
      };

      force_inline const WavetableData* getAllActiveData() {
        return active_audio_data_.load();
      }

      force_inline poly_float* getActiveFrequencyAmplitudes(int frame_index) {
        return active_audio_data_.load()->frequency_amplitudes[clampActiveFrame(frame_index)];
      }

      force_inline poly_float* getActiveNormalizedFrequencies(int frame_index) {
        return active_audio_data_.load()->normalized_frequencies[clampActiveFrame(frame_index)];
      }

      force_inline int getActiveVersion() {
        return active_audio_data_.load()->version;
      }

      void loadWaveFrame(const WaveFrame* wave_frame);
      void loadWaveFrame(const WaveFrame* wave_frame, int to_index);
      void postProcess(float max_span);

      force_inline int numFrames() const { return current_data_->num_frames; }
      force_inline int numActiveFrames() const { return active_audio_data_.load()->num_frames; }

      force_inline void markUsed() { active_audio_data_ = current_data_; }
      force_inline void markUnused() { active_audio_data_ = nullptr; }

      force_inline void setShepardTable(bool shepard) { shepard_table_ = shepard; }
      force_inline bool isShepardTable() { return shepard_table_; }

    protected:
      Wavetable() = default;
    
      void loadFrequencyAmplitudes(const std::complex<float>* frequencies, int to_index);
      void loadNormalizedFrequencies(const std::complex<float>* frequencies, int to_index);

      static const mono_float kZeroWaveform[kWaveformSize + kExtraValues];

      std::string name_;
      std::string author_;
      int max_frames_;
      WavetableData* current_data_;
      std::atomic<WavetableData*> active_audio_data_;
      std::unique_ptr<WavetableData> data_;
      bool shepard_table_;

      mono_float fft_data_[2 * kWaveformSize];

      JUCE_LEAK_DETECTOR(Wavetable)
  };
} // namespace vital

