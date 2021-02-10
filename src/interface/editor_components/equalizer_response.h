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
#include "digital_svf.h"
#include "open_gl_line_renderer.h"
#include "open_gl_multi_quad.h"
#include "synth_slider.h"

class EqualizerResponse : public OpenGlLineRenderer, SynthSlider::SliderListener {
  public:
    static constexpr int kResolution = 128;
    static constexpr int kViewSampleRate = 100000;
    static constexpr float kDefaultDbBufferRatio = 0.2f;
    static constexpr float kMouseMultiplier = 0.3f;

    class Listener {
      public:
        virtual ~Listener() = default;
      
        virtual void lowBandSelected() = 0;
        virtual void midBandSelected() = 0;
        virtual void highBandSelected() = 0;
    };

    EqualizerResponse();
    ~EqualizerResponse();
  
    void initEq(const vital::output_map& mono_modulations);
    void initReverb(const vital::output_map& mono_modulations);

    virtual void init(OpenGlWrapper& open_gl) override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    virtual void destroy(OpenGlWrapper& open_gl) override;

    void setControlPointBounds(float selected_x, float selected_y,
                               float unselected_x1, float unselected_y1,
                               float unselected_x2, float unselected_y2);
    void drawControlPoints(OpenGlWrapper& open_gl);
    void drawResponse(OpenGlWrapper& open_gl, int index);

    void computeFilterCoefficients();
    void moveFilterSettings(Point<float> position);

    void setLowSliders(SynthSlider* cutoff, SynthSlider* resonance, SynthSlider* gain);
    void setBandSliders(SynthSlider* cutoff, SynthSlider* resonance, SynthSlider* gain);
    void setHighSliders(SynthSlider* cutoff, SynthSlider* resonance, SynthSlider* gain);
    void setSelectedBand(int selected_band);

    Point<float> getLowPosition();
    Point<float> getBandPosition();
    Point<float> getHighPosition();

    void resized() override {
      OpenGlLineRenderer::resized();

      unselected_points_.setBounds(getLocalBounds());
      selected_point_.setBounds(getLocalBounds());
      dragging_point_.setBounds(getLocalBounds());
    }

    void paintBackground(Graphics& g) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    int getHoveredBand(const MouseEvent& e);

    void setActive(bool active);
    void setHighPass(bool high_pass);
    void setNotch(bool notch);
    void setLowPass(bool high_pass);
    void setDbBufferRatio(float ratio) { db_buffer_ratio_ = ratio; }
    void setDrawFrequencyLines(bool draw_lines) { draw_frequency_lines_ = draw_lines; }

    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    vital::poly_float getOutputTotal(vital::Output* output, Slider* slider);

    int resolution_;
    bool active_;
    bool high_pass_;
    bool notch_;
    bool low_pass_;
    bool animate_;
    bool draw_frequency_lines_;
    int selected_band_;
    float db_buffer_ratio_;
    float min_db_;
    float max_db_;

    OpenGlMultiQuad unselected_points_;
    OpenGlQuad selected_point_;
    OpenGlQuad dragging_point_;

    vital::DigitalSvf low_filter_;
    vital::DigitalSvf band_filter_;
    vital::DigitalSvf high_filter_;

    vital::SynthFilter::FilterState low_filter_state_;
    vital::SynthFilter::FilterState band_filter_state_;
    vital::SynthFilter::FilterState high_filter_state_;

    SynthSlider* low_cutoff_;
    SynthSlider* low_resonance_;
    SynthSlider* low_gain_;
    SynthSlider* band_cutoff_;
    SynthSlider* band_resonance_;
    SynthSlider* band_gain_;
    SynthSlider* high_cutoff_;
    SynthSlider* high_resonance_;
    SynthSlider* high_gain_;

    vital::Output* low_cutoff_output_;
    vital::Output* low_resonance_output_;
    vital::Output* low_gain_output_;
    vital::Output* band_cutoff_output_;
    vital::Output* band_resonance_output_;
    vital::Output* band_gain_output_;
    vital::Output* high_cutoff_output_;
    vital::Output* high_resonance_output_;
    vital::Output* high_gain_output_;

    SynthSlider* current_cutoff_;
    SynthSlider* current_gain_;

    std::unique_ptr<float[]> line_data_;
    OpenGLShaderProgram* shader_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_attribute_;

    std::unique_ptr<OpenGLShaderProgram::Uniform> midi_cutoff_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> resonance_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> low_amount_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> band_amount_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> high_amount_uniform_;

    GLuint vertex_array_object_;
    GLuint line_buffer_;
    GLuint response_buffer_;
    std::vector<Listener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EqualizerResponse)
};

