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

#include "file_source.h"

FileSource::FileSourceKeyframe::FileSourceKeyframe(SampleBuffer* sample_buffer) {
  sample_buffer_ = sample_buffer;
  start_position_ = 0.0f;
  window_fade_ = 1.0f;
  window_size_ = vital::WaveFrame::kWaveformSize;
  fade_style_ = kWaveBlend;
  phase_style_ = kNone;
  overridden_phase_ = nullptr;
  interpolate_from_frame_ = nullptr;
  interpolate_to_frame_ = nullptr;
}

void FileSource::FileSourceKeyframe::copy(const WavetableKeyframe* keyframe) {
  const FileSourceKeyframe* source = dynamic_cast<const FileSourceKeyframe*>(keyframe);
  VITAL_ASSERT(source);
  start_position_ = source->start_position_;
  window_fade_ = source->window_fade_;
}

void FileSource::FileSourceKeyframe::interpolate(const WavetableKeyframe* from_keyframe,
                                                 const WavetableKeyframe* to_keyframe,
                                                 float t) {
  const FileSourceKeyframe* from = dynamic_cast<const FileSourceKeyframe*>(from_keyframe);
  const FileSourceKeyframe* to = dynamic_cast<const FileSourceKeyframe*>(to_keyframe);
  VITAL_ASSERT(from);
  VITAL_ASSERT(to);

  start_position_ = linearTween(from->start_position_, to->start_position_, t);
  window_fade_ = linearTween(from->window_fade_, to->window_fade_, t);
}

force_inline float FileSource::FileSourceKeyframe::getScaledInterpolatedSample(float position) {
  const float* buffer = getCubicInterpolationBuffer();
  float clamped_position = vital::utils::clamp(position, 0.0f, sample_buffer_->size - 1);
  int start_index = clamped_position;
  float t = clamped_position - start_index;

  vital::matrix interpolation_matrix = vital::utils::getCatmullInterpolationMatrix(t);
  vital::matrix value_matrix = vital::utils::getValueMatrix(buffer, start_index);
  value_matrix.transpose();
  
  return interpolation_matrix.multiplyAndSumRows(value_matrix)[0];
}

float FileSource::FileSourceKeyframe::getNormalizationScale() {
  const float* buffer = getDataBuffer();
  if (buffer == nullptr)
    return 1.0f;

  double cycles_in = start_position_ / window_size_;
  int cycle = cycles_in;

  double start_index = cycle * window_size_;

  float max = 0.0f;
  float min = 0.0f;
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    double t = i / (vital::WaveFrame::kWaveformSize * 1.0);
    double position = std::min<double>(start_index + t * window_size_, sample_buffer_->size - 1);
    int from_index = position;
    int to_index = std::min(sample_buffer_->size - 1, from_index + 1);

    VITAL_ASSERT(from_index >= 0 && from_index < sample_buffer_->size);
    VITAL_ASSERT(to_index >= 0 && to_index < sample_buffer_->size);

    float from_sample = buffer[from_index];
    float to_sample = buffer[to_index];
    max = std::max(from_sample, max);
    max = std::max(to_sample, max);
    min = std::min(from_sample, min);
    min = std::min(to_sample, min);
  }

  return 2.0f / std::max(max - min, 0.001f);
}

void FileSource::FileSourceKeyframe::render(vital::WaveFrame* wave_frame) {
  if (sample_buffer_->size <= 0) {
    wave_frame->clear();
    return;
  }

  if (fade_style_ == kWaveBlend)
    renderWaveBlend(wave_frame);
  else if (fade_style_ == kNoInterpolate)
    renderNoInterpolate(wave_frame);
  else if (fade_style_ == kTimeInterpolate)
    renderTimeInterpolate(wave_frame);
  else if (fade_style_ == kFreqInterpolate)
    renderFreqInterpolate(wave_frame);

  if (phase_style_ == kClear || phase_style_ == kVocode) {
    for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
      float amplitude = std::abs(wave_frame->frequency_domain[i]);
      wave_frame->frequency_domain[i] = std::polar(amplitude, overridden_phase_[i]);
    }
  }

  wave_frame->toTimeDomain();
}

void FileSource::FileSourceKeyframe::renderWaveBlend(vital::WaveFrame* wave_frame) {
  double window_ratio = window_size_ / vital::WaveFrame::kWaveformSize;
  int waveform_middle = vital::WaveFrame::kWaveformSize / 2;
  int start_index = start_position_ / window_ratio + window_size_ / 2.0f + waveform_middle;
  start_index = start_index % vital::WaveFrame::kWaveformSize;

  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    double t = i / (vital::WaveFrame::kWaveformSize * 1.0);
    double position = start_position_ + t * window_size_;
    int write_index = (start_index + i) % vital::WaveFrame::kWaveformSize;
    wave_frame->time_domain[write_index] = getScaledInterpolatedSample(position);
  }

  int fade_samples = window_fade_ * vital::WaveFrame::kWaveformSize;
  double fade_size = fade_samples * window_ratio;
  for (int i = 0; i < fade_samples; ++i) {
    double t = i / (fade_samples - 1.0f);
    double fade = 0.5 + 0.5 * cos(vital::kPi * t);

    int write_index = (start_index + i) % vital::WaveFrame::kWaveformSize;
    double position = start_position_ + window_size_ + t * fade_size;
    double existing_value = wave_frame->time_domain[write_index];
    double fade_value = getScaledInterpolatedSample(position);
    wave_frame->time_domain[write_index] = linearTween(existing_value, fade_value, fade);
  }
  wave_frame->toFrequencyDomain();
}

void FileSource::FileSourceKeyframe::renderNoInterpolate(vital::WaveFrame* wave_frame) {
  double cycles_in = start_position_ / window_size_;
  int cycle = cycles_in;

  double start_index = cycle * window_size_;

  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    double t = i / (vital::WaveFrame::kWaveformSize * 1.0);
    double position = start_index + t * window_size_;
    wave_frame->time_domain[i] = getScaledInterpolatedSample(position);
  }

  wave_frame->toFrequencyDomain();
}

void FileSource::FileSourceKeyframe::renderTimeInterpolate(vital::WaveFrame* wave_frame) {
  double cycles_in = start_position_ / window_size_;
  int from_cycle = cycles_in;
  int to_cycle = from_cycle + 1;
  float transition = cycles_in - from_cycle;

  double start_index_from = from_cycle * window_size_;
  double start_index_to = to_cycle * window_size_;

  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    double t = i / (vital::WaveFrame::kWaveformSize * 1.0);
    double from_position = start_index_from + t * window_size_;
    double to_position = start_index_to + t * window_size_;
    float from_sample = getScaledInterpolatedSample(from_position);
    float to_sample = getScaledInterpolatedSample(to_position);
    wave_frame->time_domain[i] = vital::utils::interpolate(from_sample, to_sample, transition);
  }

  wave_frame->toFrequencyDomain();
}

void FileSource::FileSourceKeyframe::renderFreqInterpolate(vital::WaveFrame* wave_frame) {
  double cycles_in = start_position_ / window_size_;
  int from_cycle = cycles_in;
  int to_cycle = from_cycle + 1;
  float transition = cycles_in - from_cycle;

  double start_index_from = from_cycle * window_size_;
  double start_index_to = to_cycle * window_size_;

  vital::WaveFrame* from_wave_frame = interpolate_from_frame_->wave_frame();
  vital::WaveFrame* to_wave_frame = interpolate_to_frame_->wave_frame();
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i) {
    double t = i / (vital::WaveFrame::kWaveformSize * 1.0);
    double from_position = start_index_from + t * window_size_;
    double to_position = start_index_to + t * window_size_;
    from_wave_frame->time_domain[i] = getScaledInterpolatedSample(from_position);
    to_wave_frame->time_domain[i] = getScaledInterpolatedSample(to_position);
  }

  from_wave_frame->toFrequencyDomain();
  to_wave_frame->toFrequencyDomain();
  interpolate_from_frame_->linearFrequencyInterpolate(from_wave_frame, to_wave_frame, transition);
  wave_frame->copy(interpolate_from_frame_->wave_frame());
}

json FileSource::FileSourceKeyframe::stateToJson() {
  json data = WavetableKeyframe::stateToJson();
  data["start_position"] = start_position_;
  data["window_fade"] = window_fade_;
  data["window_size"] = window_size_;
  return data;
}

void FileSource::FileSourceKeyframe::jsonToState(json data) {
  WavetableKeyframe::jsonToState(data);
  start_position_ = data["start_position"];
  window_fade_ = data["window_fade"];
  window_size_ = data["window_size"];
}

FileSource::FileSource() : compute_frame_(&sample_buffer_), overridden_phase_(),
                           fade_style_(kWaveBlend), phase_style_(kNone),
                           normalize_gain_(false), normalize_mult_(false),
                           random_generator_(-vital::kPi, vital::kPi) {
  window_size_ = vital::WaveFrame::kWaveformSize;
  random_seed_ = random_generator_.next() * (INT_MAX / vital::kPi);
}

WavetableKeyframe* FileSource::createKeyframe(int position) {
  FileSourceKeyframe* keyframe = new FileSourceKeyframe(&sample_buffer_);
  interpolate(keyframe, position);
  return keyframe;
}

void FileSource::render(vital::WaveFrame* wave_frame, float position) {
  if (sample_buffer_.data == nullptr)
    wave_frame->clear();
  else {
    interpolate(&compute_frame_, position);
    compute_frame_.setWindowSize(window_size_);
    compute_frame_.setFadeStyle(fade_style_);
    compute_frame_.setPhaseStyle(phase_style_);
    compute_frame_.setInterpolateFromFrame(&interpolate_from_frame_);
    compute_frame_.setInterpolateToFrame(&interpolate_to_frame_);
    compute_frame_.setOverriddenPhaseBuffer(overridden_phase_);
    compute_frame_.render(wave_frame);
    wave_frame->setFrequencyRatio(window_size_ / vital::WaveFrame::kWaveformSize);
    wave_frame->setSampleRate(sample_buffer_.sample_rate);
    if (normalize_mult_)
      wave_frame->normalize(normalize_gain_);
    wave_frame->toFrequencyDomain();
  }
}

WavetableComponentFactory::ComponentType FileSource::getType() {
  return WavetableComponentFactory::kFileSource;
}

json FileSource::stateToJson() {
  double max_position = 0;
  for (int i = 0; i < numFrames(); ++i)
    max_position = std::max(max_position, getKeyframe(i)->getStartPosition());

  json data = WavetableComponent::stateToJson();
  data["normalize_gain"] = normalize_gain_;
  data["normalize_mult"] = normalize_mult_;
  data["window_size"] = window_size_;
  data["fade_style"] = fade_style_;
  data["phase_style"] = phase_style_;
  data["random_seed"] = random_seed_;
  data["audio_sample_rate"] = sample_buffer_.sample_rate;

  int save_samples = max_position + 2 * window_size_ + kExtraSaveSamples;
  int num_samples = std::min(sample_buffer_.size, save_samples);
  String encoded = "";
  if (getDataBuffer()) {
    std::unique_ptr<int16_t[]> pcm_data = std::make_unique<int16_t[]>(num_samples);
    vital::utils::floatToPcmData(pcm_data.get(), getDataBuffer(), num_samples);
    encoded = Base64::toBase64(pcm_data.get(), num_samples * sizeof(int16_t));
  }
  data["audio_file"] = encoded.toStdString();
  return data;
}

void FileSource::jsonToState(json data) {
  normalize_gain_ = data["normalize_gain"];
  if (data.count("normalize_mult"))
    normalize_mult_ = data["normalize_mult"];
  else
    normalize_mult_ = true;
  window_size_ = data["window_size"];
  fade_style_ = kWaveBlend;
  if (data.count("fade_style"))
    fade_style_ = data["fade_style"];

  phase_style_ = kNone;
  if (data.count("phase_style"))
    phase_style_ = data["phase_style"];

  if (data.count("random_seed"))
    random_seed_ = data["random_seed"];

  writePhaseOverrideBuffer();

  WavetableComponent::jsonToState(data);

  int sample_rate = vital::kDefaultSampleRate;
  if (data.count("audio_sample_rate"))
    sample_rate = data["audio_sample_rate"];

  MemoryOutputStream decoded;
  std::string audio_data = data["audio_file"];
  Base64::convertFromBase64(decoded, audio_data);

  int size = static_cast<int>(decoded.getDataSize()) / sizeof(int16_t);
  std::unique_ptr<float[]> float_data = std::make_unique<float[]>(size);
  vital::utils::pcmToFloatData(float_data.get(), (int16_t*)decoded.getData(), size);
  loadBuffer(float_data.get(), size, sample_rate);
}

FileSource::FileSourceKeyframe* FileSource::getKeyframe(int index) {
  WavetableKeyframe* wavetable_keyframe = keyframes_[index].get();
  return dynamic_cast<FileSource::FileSourceKeyframe*>(wavetable_keyframe);
}

void FileSource::setPhaseStyle(PhaseStyle phase_style) {
  if (phase_style_ == phase_style)
    return;

  phase_style_ = phase_style;
  if (phase_style_ == kVocode)
    random_seed_++;

  writePhaseOverrideBuffer();
}

void FileSource::writePhaseOverrideBuffer() {
  if (phase_style_ == kClear) {
    for (int i = 0; i < vital::WaveFrame::kWaveformSize / 2; ++i) {
      overridden_phase_[2 * i] = -0.5f * vital::kPi;
      overridden_phase_[2 * i + 1] = 0.5f * vital::kPi;
    }
  }
  else if (phase_style_ == kVocode) {
    random_generator_.seed(random_seed_);
    for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i)
      overridden_phase_[i] = random_generator_.next();
  }
}

void FileSource::loadBuffer(const float* buffer, int size, int sample_rate) {
  sample_buffer_.sample_rate = sample_rate;
  sample_buffer_.size = size;
  sample_buffer_.data = std::make_unique<float[]>(size + kExtraBufferSamples);
  memcpy(sample_buffer_.data.get() + 1, buffer, size * sizeof(float));

  sample_buffer_.data[0] = sample_buffer_.data[1];

  for (int i = 1; i < kExtraBufferSamples; ++i)
    sample_buffer_.data[sample_buffer_.size + i] = sample_buffer_.data[size];
}

void FileSource::detectPitch(int max_period) {
  int start = (sample_buffer_.size - kPitchDetectMaxPeriod) / 3;
  pitch_detector_.loadSignal(getDataBuffer() + start, kPitchDetectMaxPeriod);
  float period = pitch_detector_.matchPeriod(max_period);
  setWindowSize(period);
}

void FileSource::detectWaveEditTable() {
  static constexpr int kWaveEditFrameLength = 256;
  static constexpr int kFrequencyDomainTotals = 8;
  static constexpr int kWaveEditNumFrames = 64;
  if (sample_buffer_.size != kWaveEditFrameLength * kWaveEditNumFrames)
    return;

  vital::WaveFrame wave_frame;
  const float* buffer = getDataBuffer();
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i)
    wave_frame.time_domain[i] = buffer[i];

  wave_frame.toFrequencyDomain();

  int size_mult = vital::WaveFrame::kWaveformSize / kWaveEditFrameLength;
  std::unique_ptr<float[]> totals = std::make_unique<float[]>(size_mult);
  for (int i = 0; i < size_mult; ++i) {
    for (int j = 0; j < kFrequencyDomainTotals; ++j)
      totals[i] += std::abs(wave_frame.frequency_domain[i + 1 + j * size_mult]);
  }

  for (int i = 0; i < size_mult - 1; ++i) {
    if (totals[i] > totals[size_mult - 1])
      return;
  }

  setWindowSize(kWaveEditFrameLength);
}
