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

#include "wavetable.h"
#include "fourier_transform.h"

#include <thread>

namespace vital {

  const mono_float Wavetable::kZeroWaveform[kWaveformSize + kExtraValues] = { };

  Wavetable::Wavetable(int max_frames) :
      max_frames_(max_frames), current_data_(nullptr), 
      active_audio_data_(nullptr), shepard_table_(false), fft_data_() {
    loadDefaultWavetable();
  }

  void Wavetable::loadDefaultWavetable() {
    setNumFrames(1);
    WaveFrame default_frame;
    loadWaveFrame(&default_frame);
  }
  
  void Wavetable::setNumFrames(int num_frames) {
    VITAL_ASSERT(active_audio_data_.is_lock_free());
    VITAL_ASSERT(num_frames <= max_frames_);
    if (data_ && num_frames == data_->num_frames)
      return;

    int old_version = 0;
    int old_num_frames = 0;
    if (data_) {
      old_version = data_->version;
      old_num_frames = data_->num_frames;
    }

    std::unique_ptr<WavetableData> old_data = std::move(data_);
    data_ = std::make_unique<WavetableData>(num_frames, old_version + 1);
    data_->wave_data = std::make_unique<mono_float[][kWaveformSize]>(num_frames);
    data_->frequency_amplitudes = std::make_unique<poly_float[][kPolyFrequencySize]>(num_frames);
    data_->normalized_frequencies = std::make_unique<poly_float[][kPolyFrequencySize]>(num_frames);
    data_->phases = std::make_unique<poly_float[][kPolyFrequencySize]>(num_frames);

    int frame_size = kWaveformSize * sizeof(mono_float);
    int frequency_size = kPolyFrequencySize * sizeof(poly_float);
    int copy_frames = std::min(num_frames, old_num_frames);
    for (int i = 0; i < copy_frames; ++i) {
      memcpy(data_->wave_data[i], old_data->wave_data[i], frame_size);
      memcpy(data_->frequency_amplitudes[i], old_data->frequency_amplitudes[i], frequency_size);
      memcpy(data_->normalized_frequencies[i], old_data->normalized_frequencies[i], frequency_size);
      memcpy(data_->phases[i], old_data->phases[i], frequency_size);
    }

    if (old_data) {
      data_->frequency_ratio = old_data->frequency_ratio;
      data_->sample_rate = old_data->sample_rate;

      int remaining_frames = num_frames - old_num_frames;
      void* last_old_frame = old_data->wave_data[old_num_frames - 1];
      void* last_old_amplitudes = old_data->frequency_amplitudes[old_num_frames - 1];
      void* last_old_normalized = old_data->normalized_frequencies[old_num_frames - 1];
      void* last_old_phases = old_data->phases[old_num_frames - 1];
      for (int i = 0; i < remaining_frames; ++i) {
        memcpy(data_->wave_data[i + old_num_frames], last_old_frame, frame_size);
        memcpy(data_->frequency_amplitudes[i + old_num_frames], last_old_amplitudes, frequency_size);
        memcpy(data_->normalized_frequencies[i + old_num_frames], last_old_normalized, frequency_size);
        memcpy(data_->phases[i + old_num_frames], last_old_phases, frequency_size);
      }
    }

    current_data_ = data_.get();
    while (active_audio_data_.load())
      std::this_thread::yield(); // Wait for audio thread to finish using old_data.
  }

  void Wavetable::setFrequencyRatio(float frequency_ratio) {
    current_data_->frequency_ratio = frequency_ratio;
  }

  void Wavetable::setSampleRate(float rate) {
    current_data_->sample_rate = rate;
  }

  void Wavetable::loadWaveFrame(const WaveFrame* wave_frame) {
    loadWaveFrame(wave_frame, wave_frame->index);
  }

  void Wavetable::loadWaveFrame(const WaveFrame* wave_frame, int to_index) {
    if (to_index >= current_data_->num_frames)
      return;

    loadFrequencyAmplitudes(wave_frame->frequency_domain, to_index);
    loadNormalizedFrequencies(wave_frame->frequency_domain, to_index);
    memcpy(current_data_->wave_data[to_index], wave_frame->time_domain, kWaveformSize * sizeof(mono_float));
  }

  void Wavetable::postProcess(float max_span) {
    static constexpr float kMinAmplitudePhase = 0.1f;

    if (max_span > 0.0f) {
      float scale = 2.0f / max_span;
      for (int w = 0; w < current_data_->num_frames; ++w) {
        poly_float* frequency_amplitudes = current_data_->frequency_amplitudes[w];
        for (int i = 0; i < kPolyFrequencySize; ++i)
          frequency_amplitudes[i] *= scale;

        mono_float* wave_data = current_data_->wave_data[w];
        for (int i = 0; i < kWaveformSize; ++i)
          wave_data[i] *= scale;
      }
    }

    std::unique_ptr<std::complex<float>[]> normalized_defaults = 
        std::make_unique<std::complex<float>[]>(kNumHarmonics);
    for (int i = 0; i < kNumHarmonics; ++i) {
      int amp_index = 2 * i;

      int last_min_amp_frame = -1;
      std::complex<float> last_normalized_frequency = std::complex<float>(0.0f, 1.0f);
      for (int w = 0; w < current_data_->num_frames; ++w) {
        mono_float amplitude = ((mono_float*)current_data_->frequency_amplitudes[w])[amp_index];
        std::complex<float> normalized_frequency = ((std::complex<float>*)current_data_->normalized_frequencies[w])[i];

        if (amplitude > kMinAmplitudePhase) {
          if (last_min_amp_frame < 0) {
            last_min_amp_frame = 0;
            last_normalized_frequency = normalized_frequency;
          }

          std::complex<float> delta_normalized_frequency = normalized_frequency - last_normalized_frequency;

          for (int frame = last_min_amp_frame + 1; frame < w; ++frame) {
            float t = (frame - last_min_amp_frame) * 1.0f / (w - last_min_amp_frame);
            std::complex<float> normalized = delta_normalized_frequency * t + last_normalized_frequency;
            ((std::complex<float>*)current_data_->normalized_frequencies[frame])[i] = normalized;
          }
          last_normalized_frequency = normalized_frequency;
          last_min_amp_frame = w;
        }
      }
      for (int frame = last_min_amp_frame + 1; frame < current_data_->num_frames; ++frame)
        ((std::complex<float>*)current_data_->normalized_frequencies[frame])[i] = last_normalized_frequency;
    }
  }

  void Wavetable::loadFrequencyAmplitudes(const std::complex<float>* frequencies, int to_index) {
    mono_float* amplitudes = (mono_float*)current_data_->frequency_amplitudes[to_index];
    for (int i = 0; i < kNumHarmonics; ++i) {
      float amplitude = std::abs(frequencies[i]);
      amplitudes[2 * i] = amplitude;
      amplitudes[2 * i + 1] = amplitude;
    }
  }

  void Wavetable::loadNormalizedFrequencies(const std::complex<float>* frequencies, int to_index) {
    std::complex<float>* normalized = (std::complex<float>*)current_data_->normalized_frequencies[to_index];
    mono_float* phases = (mono_float*)current_data_->phases[to_index];
    for (int i = 0; i < kNumHarmonics; ++i) {
      mono_float arg = std::arg(frequencies[i]);
      normalized[i] = std::polar(1.0f, arg);
      phases[2 * i] = arg;
      phases[2 * i + 1] = arg;
    }
  }
} // namespace vital
