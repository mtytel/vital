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
#include "synth_section.h"
#include "open_gl_line_renderer.h"
#include "phaser_filter.h"
#include "synth_module.h"

class SynthButton;
class TempoSelector;
class SynthSlider;
class SynthGuiInterface;

class PhaserResponse : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 256;
    static constexpr int kDefaultVisualSampleRate = 200000;

    PhaserResponse(const vital::output_map& mono_modulations);
    virtual ~PhaserResponse();

    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void destroy(OpenGlWrapper& open_gl) override;

    void mouseDown(const MouseEvent& e) override {
      last_mouse_position_ = e.getPosition();
    }

    void mouseDrag(const MouseEvent& e) override {
      Point<int> delta = e.getPosition() - last_mouse_position_;
      last_mouse_position_ = e.getPosition();

      float cutoff_range = cutoff_slider_->getMaximum() - cutoff_slider_->getMinimum();
      cutoff_slider_->setValue(cutoff_slider_->getValue() + delta.x * cutoff_range / getWidth());
      float resonance_range = resonance_slider_->getMaximum() - resonance_slider_->getMinimum();
      resonance_slider_->setValue(resonance_slider_->getValue() - delta.y * resonance_range / getHeight());
    }

    void setCutoffSlider(SynthSlider* slider) { cutoff_slider_ = slider; }
    void setResonanceSlider(SynthSlider* slider) { resonance_slider_ = slider; }
    void setBlendSlider(SynthSlider* slider) { blend_slider_ = slider; }
    void setMixSlider(SynthSlider* slider) { mix_slider_ = slider; }

    void setActive(bool active) { active_ = active; }
    void setStyle(int style) { filter_state_.style = style; }

    void setDefaultBlend(vital::poly_float blend) { blend_setting_ = blend; }

  private:
    struct FilterResponseShader {
      static constexpr int kMaxStages = 3;
      OpenGLShaderProgram* shader;
      std::unique_ptr<OpenGLShaderProgram::Attribute> position;

      std::unique_ptr<OpenGLShaderProgram::Uniform> mix;
      std::unique_ptr<OpenGLShaderProgram::Uniform> midi_cutoff;
      std::unique_ptr<OpenGLShaderProgram::Uniform> resonance;
      std::unique_ptr<OpenGLShaderProgram::Uniform> db24;
      std::unique_ptr<OpenGLShaderProgram::Uniform> stages[kMaxStages];
    };

    void drawFilterResponse(OpenGlWrapper& open_gl, bool animate);
    vital::poly_float getOutputTotal(const vital::Output* output, vital::poly_float default_value);

    void setupFilterState();
    void loadShader(int index);
    void bind(OpenGLContext& open_gl_context);
    void unbind(OpenGLContext& open_gl_context);
    void renderLineResponse(OpenGlWrapper& open_gl);

    SynthGuiInterface* parent_;
    bool active_;
    Point<int> last_mouse_position_;

    vital::PhaserFilter phaser_filter_;

    vital::SynthFilter::FilterState filter_state_;
    vital::poly_float mix_;

    SynthSlider* cutoff_slider_;
    SynthSlider* resonance_slider_;
    SynthSlider* blend_slider_;
    SynthSlider* mix_slider_;

    const vital::StatusOutput* phaser_cutoff_;
    const vital::Output* filter_mix_output_;
    const vital::Output* resonance_output_;
    const vital::Output* blend_output_;

    vital::poly_float blend_setting_;

    FilterResponseShader response_shader_;
    std::unique_ptr<float[]> line_data_;
    GLuint vertex_array_object_;
    GLuint line_buffer_;
    GLuint response_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserResponse)
};

class PhaserSection : public SynthSection {
  public:
    PhaserSection(String name, const vital::output_map& mono_modulations);
    virtual ~PhaserSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<SynthSlider> frequency_;
    std::unique_ptr<SynthSlider> tempo_;
    std::unique_ptr<TempoSelector> sync_;
    std::unique_ptr<SynthSlider> feedback_;
    std::unique_ptr<SynthSlider> center_;
    std::unique_ptr<SynthSlider> mod_depth_;
    std::unique_ptr<SynthSlider> phase_offset_;
    std::unique_ptr<SynthSlider> dry_wet_;
    std::unique_ptr<SynthSlider> blend_;

    std::unique_ptr<PhaserResponse> phaser_response_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserSection)
};

