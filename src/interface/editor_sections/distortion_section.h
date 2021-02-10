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
#include "open_gl_line_renderer.h"
#include "synth_section.h"
#include "digital_svf.h"

class SynthButton;
class SynthSlider;
class TabSelector;
class TextSelector;
class DistortionViewer;

class DistortionFilterResponse : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 256;
    static constexpr int kDefaultVisualSampleRate = 200000;

    DistortionFilterResponse(const vital::output_map& mono_modulations);
    virtual ~DistortionFilterResponse();

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

    void setActive(bool active) { active_ = active; }

  private:
    struct FilterResponseShader {
      static constexpr int kMaxStages = 5;
      OpenGLShaderProgram* shader;
      std::unique_ptr<OpenGLShaderProgram::Attribute> position;

      std::unique_ptr<OpenGLShaderProgram::Uniform> mix;
      std::unique_ptr<OpenGLShaderProgram::Uniform> midi_cutoff;
      std::unique_ptr<OpenGLShaderProgram::Uniform> resonance;
      std::unique_ptr<OpenGLShaderProgram::Uniform> drive;
      std::unique_ptr<OpenGLShaderProgram::Uniform> db24;
      std::unique_ptr<OpenGLShaderProgram::Uniform> stages[kMaxStages];
    };

    void drawFilterResponse(OpenGlWrapper& open_gl, bool animate);
    vital::poly_float getOutputTotal(vital::Output* output, vital::poly_float default_value);

    void setupFilterState();
    void loadShader(int index);
    void bind(OpenGLContext& open_gl_context);
    void unbind(OpenGLContext& open_gl_context);
    void renderLineResponse(OpenGlWrapper& open_gl);

    bool active_;
    Point<int> last_mouse_position_;
    vital::DigitalSvf filter_;
    vital::SynthFilter::FilterState filter_state_;

    SynthSlider* cutoff_slider_;
    SynthSlider* resonance_slider_;
    SynthSlider* blend_slider_;

    vital::Output* cutoff_output_;
    vital::Output* resonance_output_;
    vital::Output* blend_output_;

    FilterResponseShader response_shader_;
    std::unique_ptr<float[]> line_data_;
    GLuint vertex_array_object_;
    GLuint line_buffer_;
    GLuint response_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistortionFilterResponse)
};

class DistortionSection : public SynthSection {
  public:
    static constexpr int kViewerResolution = 124;

    DistortionSection(String name, const vital::output_map& mono_modulations);
    virtual ~DistortionSection();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { if (isActive()) paintTabShadow(g); }
    void resized() override;
    void setActive(bool active) override;
    void sliderValueChanged(Slider* changed_slider) override;
    void setAllValues(vital::control_map& controls) override;
    void setFilterActive(bool active);

  private:
    std::unique_ptr<SynthButton> on_;
    std::unique_ptr<TextSelector> type_;
    std::unique_ptr<TextSelector> filter_order_;
    std::unique_ptr<SynthSlider> drive_;
    std::unique_ptr<SynthSlider> mix_;
    std::unique_ptr<SynthSlider> filter_cutoff_;
    std::unique_ptr<SynthSlider> filter_resonance_;
    std::unique_ptr<SynthSlider> filter_blend_;
    std::unique_ptr<DistortionViewer> distortion_viewer_;
    std::unique_ptr<DistortionFilterResponse> filter_response_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistortionSection)
};

