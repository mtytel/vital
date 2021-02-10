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

#include "oscilloscope.h"

#include "fourier_transform.h"
#include "synth_constants.h"
#include "skin.h"
#include "shaders.h"
#include "utils.h"

Oscilloscope::Oscilloscope() : OpenGlLineRenderer(kResolution) {
  memory_ = nullptr;
  setFill(true);
  addRoundedCorners();
}

Oscilloscope::~Oscilloscope() { }

void Oscilloscope::drawWaveform(OpenGlWrapper& open_gl, int index) {
  float y_adjust = getHeight() / 2.0f;
  float width = getWidth();
  if (memory_) {
    for (int i = 0; i < kResolution; ++i) {
      float t = i / (kResolution - 1.0f);
      float memory_spot = (1.0f * i * vital::kOscilloscopeMemoryResolution) / kResolution;
      int memory_index = memory_spot;
      float remainder = memory_spot - memory_index;
      float from = memory_[memory_index][index];
      float to = memory_[memory_index + 1][index];
      setXAt(i, t * width);
      setYAt(i, (1.0f - vital::utils::interpolate(from, to, remainder)) * y_adjust);
    }
  }
  OpenGlLineRenderer::render(open_gl, true);
}

void Oscilloscope::render(OpenGlWrapper& open_gl, bool animate) {
  setLineWidth(findValue(Skin::kWidgetLineWidth));
  setFillCenter(findValue(Skin::kWidgetFillCenter));

  Colour color = findColour(Skin::kWidgetPrimary1, true);
  Colour fill_color = findColour(Skin::kWidgetPrimary2, true);
  setColor(color);
  float fill_fade = 0.0f;
  if (parent_)
    fill_fade = parent_->findValue(Skin::kWidgetFillFade);
  setFillColors(fill_color.withMultipliedAlpha(1.0f - fill_fade), fill_color);

  drawWaveform(open_gl, 0);
  drawWaveform(open_gl, 1);
  renderCorners(open_gl, animate);
}

Spectrogram::Spectrogram() : OpenGlLineRenderer(kResolution), transform_buffer_(),
                             left_amps_(), right_amps_(), transform_(kBits) {
  static constexpr float kDefaultAmp = 0.000001f;

  paint_background_lines_ = true;
  memory_ = nullptr;
  setFill(true);
  oversample_amount_ = 1;
  sample_rate_ = 44100;
  min_frequency_ = kDefaultMinFrequency;
  max_frequency_ = kDefaultMaxFrequency;
  min_db_ = kDefaultMinDb;
  max_db_ = kDefaultMaxDb;

  for (int i = 0; i < kAudioSize; ++i) {
    left_amps_[i] = kDefaultAmp;
    right_amps_[i] = kDefaultAmp;
  }

  addRoundedCorners();
}

Spectrogram::~Spectrogram() = default;

void Spectrogram::applyWindow() {
  static constexpr double kRadianIncrement = vital::kPi / (kAudioSize - 1.0);

  double real = -1.0f;
  double imag = 0.0f;
  double real_mult = cos(kRadianIncrement);
  double imag_mult = sin(kRadianIncrement);

  for (int i = 0; i < kAudioSize; ++i) {
    transform_buffer_[i] *= 0.5f * (real + 1.0f);

    double next_real = real * real_mult - imag * imag_mult;
    double next_imag = imag * real_mult + real * imag_mult;
    real = next_real;
    imag = next_imag;
  }
}

void Spectrogram::updateAmplitudes(int index, int offset) {
  static constexpr float kMinAmp = 0.000001f;
  static constexpr float kStartScaleAmp = 0.001f;
  static constexpr float kMinDecay = 0.06f;

  if (memory_ == nullptr)
    return;

  float min_frequency = min_frequency_ / oversample_amount_;
  float max_frequency = max_frequency_ / oversample_amount_;
  float* amps = index == 0 ? left_amps_ : right_amps_;
  float sample_hz = (1.0f * sample_rate_) / kAudioSize;
  float start_octave = log2f(min_frequency / sample_hz);
  float end_octave = std::min<float>(kBits - start_octave + 1.0f, log2f(max_frequency / sample_hz));
  float num_octaves = end_octave - start_octave;

  memory_->readSamples(transform_buffer_, kAudioSize, offset, index);
  applyWindow();
  transform_.transformRealForward(transform_buffer_);
  std::complex<float>* frequency_data = (std::complex<float>*)transform_buffer_;

  float last_bin = powf(2.0f, start_octave);
  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    float octave = num_octaves * t + start_octave;
    float bin = powf(2.0f, octave);

    int bin_index = last_bin;
    float bin_t = last_bin - bin_index;
    float prev_amplitude = std::abs(frequency_data[bin_index]);
    float next_amplitude = std::abs(frequency_data[bin_index + 1]);
    float amplitude = vital::utils::interpolate(prev_amplitude, next_amplitude, bin_t);
    if (bin - last_bin > 1.0f) {
      for (int j = last_bin + 1; j < bin; ++j)
        amplitude = std::max(amplitude, std::abs(frequency_data[j]));
    }
    last_bin = bin;

    amplitude = std::max(kMinAmp, 2.0f * amplitude / kAudioSize);
    float db = vital::utils::magnitudeToDb(std::max(amplitude, amps[i]) / kStartScaleAmp);
    db += octave * kDbSlopePerOctave;
    float decay = std::max(kMinDecay, std::min(1.0f, kDecayMult * db));
    amps[i] = std::max(kMinAmp, vital::utils::interpolate(amps[i], amplitude, decay));
  }
}

void Spectrogram::updateAmplitudes(int index) {
  updateAmplitudes(index, 0);
}

void Spectrogram::drawWaveform(OpenGlWrapper& open_gl, int index) {
  float* amps = index == 0 ? left_amps_ : right_amps_;
  float height = getHeight();
  float width = getWidth();
  float range_mult = 1.0f / (max_db_ - min_db_);
  float num_octaves = log2f(max_frequency_ / min_frequency_);

  for (int i = 0; i < kResolution; ++i) {
    float t = i / (kResolution - 1.0f);
    float db = vital::utils::magnitudeToDb(amps[i]);
    db += t * num_octaves * kDbSlopePerOctave;
    float y = (db - min_db_) * range_mult;
    setXAt(i, t * width);
    setYAt(i, height - y * height);
  }
  OpenGlLineRenderer::render(open_gl, true);
}

void Spectrogram::render(OpenGlWrapper& open_gl, bool animate) {
  SynthGuiInterface* synth_interface = findParentComponentOfClass<SynthGuiInterface>();
  if (synth_interface == nullptr)
    return;

  sample_rate_ = synth_interface->getSynth()->getSampleRate();

  setLineWidth(2.0f);
  setFillCenter(-1.0f);

  updateAmplitudes(0);
  updateAmplitudes(1);

  Colour color = findColour(Skin::kWidgetPrimary1, true);
  setColor(color);
  Colour fill_color = findColour(Skin::kWidgetPrimary2, true);
  float fill_fade = 0.0f;
  if (parent_)
    fill_fade = parent_->findValue(Skin::kWidgetFillFade);
  setFillColors(fill_color.withMultipliedAlpha(1.0f - fill_fade), fill_color);
  drawWaveform(open_gl, 0);
  drawWaveform(open_gl, 1);
  renderCorners(open_gl, animate);
}

void Spectrogram::paintBackground(Graphics& g) {
  static constexpr int kLineSpacing = 10;

  OpenGlLineRenderer::paintBackground(g);
  if (!paint_background_lines_)
    return;

  int height = getHeight();
  float max_octave = log2f(max_frequency_ / min_frequency_);
  g.setColour(findColour(Skin::kLightenScreen, true).withMultipliedAlpha(0.5f));
  float frequency = 0.0f;
  float increment = 1.0f;

  int x = 0;
  while (frequency < max_frequency_) {
    for (int i = 0; i < kLineSpacing; ++i) {
      frequency += increment;
      float t = log2f(frequency / min_frequency_) / max_octave;
      x = std::round(t * getWidth());
      g.fillRect(x, 0, 1, height);
    }
    g.fillRect(x, 0, 1, height);
    increment *= kLineSpacing;
  }
}
