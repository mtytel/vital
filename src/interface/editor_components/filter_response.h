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

#include "open_gl_line_renderer.h"

#include "skin.h"
#include "comb_filter.h"
#include "digital_svf.h"
#include "diode_filter.h"
#include "dirty_filter.h"
#include "formant_filter.h"
#include "ladder_filter.h"
#include "phaser_filter.h"
#include "sallen_key_filter.h"
#include "synth_types.h"

class SynthSlider;

class FilterResponse : public OpenGlLineRenderer {
  public:
    static constexpr int kResolution = 512;
    static constexpr int kDefaultVisualSampleRate = 200000;
    static constexpr int kCombAlternatePeriod = 3;

    static constexpr double kMouseSensitivityX = 0.3;
    static constexpr double kMouseSensitivityY = 0.3;

    enum FilterShader {
      kAnalog,
      kDirty,
      kLadder,
      kDigital,
      kDiode,
      kFormant,
      kComb,
      kPositiveFlange,
      kNegativeFlange,
      kPhase,
      kNumFilterShaders
    };

    FilterResponse(String suffix, const vital::output_map& mono_modulations);
    FilterResponse(int index, const vital::output_map& mono_modulations, const vital::output_map& poly_modulations);
    virtual ~FilterResponse();

    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void destroy(OpenGlWrapper& open_gl) override;
    void paintBackground(Graphics& g) override;

    void setCutoffSlider(SynthSlider* slider) { cutoff_slider_ = slider; }
    void setResonanceSlider(SynthSlider* slider) { resonance_slider_ = slider; }
    void setFormantXSlider(SynthSlider* slider) { formant_x_slider_ = slider; }
    void setFormantYSlider(SynthSlider* slider) { formant_y_slider_ = slider; }
    void setFilterMixSlider(SynthSlider* slider) { filter_mix_slider_ = slider; }
    void setBlendSlider(SynthSlider* slider) { blend_slider_ = slider; }
    void setTransposeSlider(SynthSlider* slider) { transpose_slider_ = slider; }
    void setFormantTransposeSlider(SynthSlider* slider) { formant_transpose_slider_ = slider; }
    void setFormantResonanceSlider(SynthSlider* slider) { formant_resonance_slider_ = slider; }
    void setFormantSpreadSlider(SynthSlider* slider) { formant_spread_slider_ = slider; }

    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;

    void setActive(bool active) { active_ = active; }
    void setModel(vital::constants::FilterModel model) { filter_model_ = model; }
    void setStyle(int style) { filter_state_.style = style; }

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

      std::unique_ptr<OpenGLShaderProgram::Uniform> formant_cutoff;
      std::unique_ptr<OpenGLShaderProgram::Uniform> formant_resonance;
      std::unique_ptr<OpenGLShaderProgram::Uniform> formant_spread;
      std::unique_ptr<OpenGLShaderProgram::Uniform> formant_low;
      std::unique_ptr<OpenGLShaderProgram::Uniform> formant_band;
      std::unique_ptr<OpenGLShaderProgram::Uniform> formant_high;
    };

    FilterResponse();

    void setFilterSettingsFromPosition(Point<int> position);

    void drawFilterResponse(OpenGlWrapper& open_gl);
    vital::poly_float getOutputsTotal(std::pair<vital::Output*, vital::Output*> outputs,
                                      vital::poly_float default_value);

    bool setupFilterState(vital::constants::FilterModel model);
    bool isStereoState();
    void loadShader(FilterShader shader, vital::constants::FilterModel model, int index);
    void bind(FilterShader shader, OpenGLContext& open_gl_context);
    void unbind(FilterShader shader, OpenGLContext& open_gl_context);
    void renderLineResponse(OpenGlWrapper& open_gl);

    bool active_;
    bool animate_;
    Point<int> last_mouse_position_;
    double current_resonance_value_;
    double current_cutoff_value_;
    double current_formant_x_value_;
    double current_formant_y_value_;

    Colour line_left_color_;
    Colour line_right_color_;
    Colour line_disabled_color_;
    Colour fill_left_color_;
    Colour fill_right_color_;
    Colour fill_disabled_color_;

    vital::SallenKeyFilter analog_filter_;
    vital::CombFilter comb_filter_;
    vital::DigitalSvf digital_filter_;
    vital::DiodeFilter diode_filter_;
    vital::DirtyFilter dirty_filter_;
    vital::FormantFilter formant_filter_;
    vital::LadderFilter ladder_filter_;
    vital::PhaserFilter phaser_filter_;

    int last_filter_style_;
    vital::constants::FilterModel last_filter_model_;
    vital::constants::FilterModel filter_model_;
    vital::SynthFilter::FilterState filter_state_;
    vital::poly_float mix_;

    SynthSlider* cutoff_slider_;
    SynthSlider* resonance_slider_;
    SynthSlider* formant_x_slider_;
    SynthSlider* formant_y_slider_;
    SynthSlider* filter_mix_slider_;
    SynthSlider* blend_slider_;
    SynthSlider* transpose_slider_;
    SynthSlider* formant_transpose_slider_;
    SynthSlider* formant_resonance_slider_;
    SynthSlider* formant_spread_slider_;

    std::pair<vital::Output*, vital::Output*> filter_mix_outputs_;
    std::pair<vital::Output*, vital::Output*> midi_cutoff_outputs_;
    std::pair<vital::Output*, vital::Output*> resonance_outputs_;
    std::pair<vital::Output*, vital::Output*> blend_outputs_;
    std::pair<vital::Output*, vital::Output*> transpose_outputs_;
    std::pair<vital::Output*, vital::Output*> interpolate_x_outputs_;
    std::pair<vital::Output*, vital::Output*> interpolate_y_outputs_;
    std::pair<vital::Output*, vital::Output*> formant_resonance_outputs_;
    std::pair<vital::Output*, vital::Output*> formant_spread_outputs_;
    std::pair<vital::Output*, vital::Output*> formant_transpose_outputs_;

    FilterResponseShader shaders_[kNumFilterShaders];
    std::unique_ptr<float[]> line_data_;
    GLuint vertex_array_object_;
    GLuint line_buffer_;
    GLuint response_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterResponse)
};

