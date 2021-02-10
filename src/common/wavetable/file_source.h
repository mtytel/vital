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
#include "pitch_detector.h"
#include "wavetable_component.h"
#include "wave_frame.h"
#include "wave_source.h"
#include "utils.h"

class FileSource : public WavetableComponent {
  public:
    static constexpr float kMaxFileSourceSamples = 176400;
    static constexpr int kExtraSaveSamples = 4;
    static constexpr int kExtraBufferSamples = 4;
    static constexpr int kPitchDetectMaxPeriod = 8096;

    enum FadeStyle {
      kWaveBlend,
      kNoInterpolate,
      kTimeInterpolate,
      kFreqInterpolate,
      kNumFadeStyles
    };

    enum PhaseStyle {
      kNone,
      kClear,
      kVocode,
      kNumPhaseStyles
    };
  
    struct SampleBuffer {
      SampleBuffer() : size(0), sample_rate(0) { }
      std::unique_ptr<float[]> data;
      int size;
      int sample_rate;
    };

    class FileSourceKeyframe : public WavetableKeyframe {
      public:
        FileSourceKeyframe(SampleBuffer* sample_buffer);
        virtual ~FileSourceKeyframe() { }

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;

        float getNormalizationScale();

        void render(vital::WaveFrame* wave_frame) override;
        void renderWaveBlend(vital::WaveFrame* wave_frame);
        void renderNoInterpolate(vital::WaveFrame* wave_frame);
        void renderTimeInterpolate(vital::WaveFrame* wave_frame);
        void renderFreqInterpolate(vital::WaveFrame* wave_frame);
        json stateToJson() override;
        void jsonToState(json data) override;

        double getStartPosition() { return start_position_; }
        double getWindowSize() { return window_size_; }
        double getWindowFade() { return window_fade_; }
        double getWindowFadeSamples() { return window_fade_ * window_size_; }
        int getSamplesNeeded() { return getWindowSize() + getWindowFadeSamples(); }

        force_inline void setStartPosition(double start_position) { start_position_ = start_position; }
        force_inline void setWindowFade(double window_fade) { window_fade_ = window_fade; }
        force_inline void setWindowSize(double window_size) { window_size_ = window_size; }
        force_inline void setFadeStyle(FadeStyle fade_style) { fade_style_ = fade_style; }
        force_inline void setPhaseStyle(PhaseStyle phase_style) { phase_style_ = phase_style; }
        force_inline void setOverriddenPhaseBuffer(const float* buffer) { overridden_phase_ = buffer; }
        force_inline const float* getDataBuffer() {
          if (sample_buffer_ == nullptr || sample_buffer_->data == nullptr)
            return nullptr;
          return sample_buffer_->data.get() + 1;
        }
        force_inline const float* getCubicInterpolationBuffer() {
          if (sample_buffer_ == nullptr)
            return nullptr;
          return sample_buffer_->data.get();
        }

        float getScaledInterpolatedSample(float time);

        void setInterpolateFromFrame(WaveSourceKeyframe* frame) {
          interpolate_from_frame_ = frame;
        }

        void setInterpolateToFrame(WaveSourceKeyframe* frame) {
          interpolate_to_frame_ = frame;
        }

      protected:
        SampleBuffer* sample_buffer_;
        const float* overridden_phase_;
        WaveSourceKeyframe* interpolate_from_frame_;
        WaveSourceKeyframe* interpolate_to_frame_;
      
        double start_position_;
        double window_fade_;
        double window_size_;
        FadeStyle fade_style_;
        PhaseStyle phase_style_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileSourceKeyframe)
    };

    FileSource();
    virtual ~FileSource() { }

    WavetableKeyframe* createKeyframe(int position) override;
    void render(vital::WaveFrame* wave_frame, float position) override;
    WavetableComponentFactory::ComponentType getType() override;
    json stateToJson() override;
    void jsonToState(json data) override;

    FileSourceKeyframe* getKeyframe(int index);
    const SampleBuffer* buffer() const { return &sample_buffer_; }
    FadeStyle getFadeStyle() { return fade_style_; }
    PhaseStyle getPhaseStyle() { return phase_style_; }
    bool getNormalizeGain() { return normalize_gain_; }

    void setNormalizeGain(bool normalize_gain) { normalize_gain_ = normalize_gain; }
    void setWindowSize(double window_size) { window_size_ = window_size; }
    void setFadeStyle(FadeStyle fade_style) { fade_style_ = fade_style; }
    void setPhaseStyle(PhaseStyle phase_style);
    void writePhaseOverrideBuffer();
    double getWindowSize() { return window_size_; }
  
    void loadBuffer(const float* buffer, int size, int sample_rate);
    void detectPitch(int max_period = vital::WaveFrame::kWaveformSize);
    void detectWaveEditTable();

    force_inline const float* getDataBuffer() {
      if (sample_buffer_.data == nullptr)
        return nullptr;
      return sample_buffer_.data.get() + 1;
    }
    force_inline const float* getCubicInterpolationBuffer() { return sample_buffer_.data.get(); }

  protected:
    FileSourceKeyframe compute_frame_;
    WaveSourceKeyframe interpolate_from_frame_;
    WaveSourceKeyframe interpolate_to_frame_;

    SampleBuffer sample_buffer_;
    float overridden_phase_[vital::WaveFrame::kWaveformSize];
    FadeStyle fade_style_;
    PhaseStyle phase_style_;
    bool normalize_gain_;
    bool normalize_mult_;
    double window_size_;

    int random_seed_;
    vital::utils::RandomGenerator random_generator_;
    PitchDetector pitch_detector_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileSource)
};

