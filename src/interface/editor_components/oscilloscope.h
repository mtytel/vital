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
#include "memory.h"
#include "fourier_transform.h"
#include "open_gl_line_renderer.h"

class Oscilloscope : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 512;

    Oscilloscope();
    virtual ~Oscilloscope();

    void drawWaveform(OpenGlWrapper& open_gl, int index);
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void setOscilloscopeMemory(const vital::poly_float* memory) { memory_ = memory; }

  private:
    const vital::poly_float* memory_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Oscilloscope)
};

class Spectrogram : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 300;
    static constexpr float kDecayMult = 0.008f;
    static constexpr int kBits = 14;
    static constexpr int kAudioSize = 1 << kBits;
    static constexpr float kDefaultMaxDb = 0.0f;
    static constexpr float kDefaultMinDb = -50.0f;
    static constexpr float kDefaultMinFrequency = 9.2f;
    static constexpr float kDefaultMaxFrequency = 21000.0f;
    static constexpr float kDbSlopePerOctave = 3.0f;

    Spectrogram();
    virtual ~Spectrogram();

    void drawWaveform(OpenGlWrapper& open_gl, int index);
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void setAudioMemory(const vital::StereoMemory* memory) { memory_ = memory; }
    void paintBackground(Graphics& g) override;
    void setOversampleAmount(int oversample) { oversample_amount_ = oversample; }
    void setMinFrequency(float frequency) { min_frequency_ = frequency; }
    void setMaxFrequency(float frequency) { max_frequency_ = frequency; }
    void setMinDb(float db) { min_db_ = db; }
    void setMaxDb(float db) { max_db_ = db; }
    void paintBackgroundLines(bool paint) { paint_background_lines_ = paint; }
  
  private:
    void updateAmplitudes(int index, int offset);
    void updateAmplitudes(int index);
    void applyWindow();

    int sample_rate_;
    int oversample_amount_;
    float min_frequency_;
    float max_frequency_;
    float min_db_;
    float max_db_;
    bool paint_background_lines_;
    float transform_buffer_[2 * kAudioSize];
    float left_amps_[kAudioSize];
    float right_amps_[kAudioSize];
    const vital::StereoMemory* memory_;
    vital::FourierTransform transform_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Spectrogram)
};

