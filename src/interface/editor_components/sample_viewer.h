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

#include "audio_file_drop_source.h"
#include "common.h"
#include "open_gl_line_renderer.h"
#include "synth_types.h"
#include "sample_source.h"
#include "synth_module.h"

class SampleViewer : public OpenGlLineRenderer, public AudioFileDropSource {
  public:
    static constexpr float kResolution = 256;
    static constexpr float kBoostDecay = 0.9f;
    static constexpr float kSpeedDecayMult = 5.0f;

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void sampleLoaded(const File& file) = 0;
    };

    SampleViewer();
    virtual ~SampleViewer();

    void init(OpenGlWrapper& open_gl) override {
      OpenGlLineRenderer::init(open_gl);
      bottom_.init(open_gl);
      dragging_overlay_.init(open_gl);
    }
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void destroy(OpenGlWrapper& open_gl) override {
      OpenGlLineRenderer::destroy(open_gl);
      bottom_.destroy(open_gl);
      dragging_overlay_.destroy(open_gl);
    }

    void resized() override;
    void setActive(bool active) { active_ = active; }
    bool isActive() const { return active_; }

    void audioFileLoaded(const File& file) override;
    void repaintAudio();
    void setLinePositions();

    void fileDragEnter(const StringArray& files, int x, int y) override {
      dragging_audio_file_ = true;
    }

    void fileDragExit(const StringArray& files) override {
      dragging_audio_file_ = false;
    }

    std::string getName();

    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }

    void setSample(vital::Sample* sample) { sample_ = sample; setLinePositions(); }

  private:
    std::vector<Listener*> listeners_;

    const vital::StatusOutput* sample_phase_output_;
    vital::poly_float last_phase_;
    vital::poly_float last_voice_;
    vital::Sample* sample_;
  
    OpenGlLineRenderer bottom_;
    OpenGlQuad dragging_overlay_;

    bool dragging_audio_file_;
    bool animate_;
    bool active_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleViewer)
};

