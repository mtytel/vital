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
#include "skin.h"
#include "open_gl_image.h"
#include "open_gl_line_renderer.h"
#include "synth_module.h"
#include "synth_slider.h"

class EnvelopeEditor : public OpenGlLineRenderer, public SynthSlider::SliderListener {
  public:
    static constexpr float kMarkerWidth = 9.0f;
    static constexpr float kRingThickness = 0.45f;
    static constexpr float kPowerMarkerWidth = 7.0f;
    static constexpr float kMarkerHoverRadius = 12.0f;
    static constexpr float kMarkerGrabRadius = 20.0f;
    static constexpr float kTailDecay = 0.965f;
    static constexpr float kPaddingX = 0.018f;
    static constexpr float kPaddingY = 0.06f;
    static constexpr float kMinPointDistanceForPower = 3.0f;
    static constexpr float kPowerMouseMultiplier = 0.06f;
    static constexpr float kTimeDisplaySize = 0.05f;

    static constexpr int kRulerDivisionSize = 4;
    static constexpr int kMaxGridLines = 36;
    static constexpr int kMaxTimesShown = 24;
    static constexpr int kNumPointsPerSection = 98;
    static constexpr int kNumSections = 4;
    static constexpr int kTotalPoints = kNumSections * kNumPointsPerSection + 1;

    EnvelopeEditor(const String& prefix,
                   const vital::output_map& mono_modulations, const vital::output_map& poly_modulations);
    ~EnvelopeEditor();

    void paintBackground(Graphics& g) override;

    void resized() override {
      OpenGlLineRenderer::resized();
      drag_circle_.setBounds(getLocalBounds());
      hover_circle_.setBounds(getLocalBounds());
      grid_lines_.setBounds(getLocalBounds());
      sub_grid_lines_.setBounds(getLocalBounds());
      position_circle_.setBounds(getLocalBounds());
      point_circles_.setBounds(getLocalBounds());
      power_circles_.setBounds(getLocalBounds());
      
      float font_height = kTimeDisplaySize * getHeight();
      for (int i = 0; i < kMaxTimesShown; ++i)
        times_[i]->setTextSize(font_height);
      
      setTimePositions();
      reset_positions_ = true;
    }

    void resetEnvelopeLine(int index);
    void guiChanged(SynthSlider* slider) override;

    void setDelaySlider(SynthSlider* delay_slider);
    void setAttackSlider(SynthSlider* attack_slider);
    void setAttackPowerSlider(SynthSlider* attack_slider);
    void setHoldSlider(SynthSlider* hold_slider);
    void setDecaySlider(SynthSlider* decay_slider);
    void setDecayPowerSlider(SynthSlider* decay_slider);
    void setSustainSlider(SynthSlider* sustain_slider);
    void setReleaseSlider(SynthSlider* release_slider);
    void setReleasePowerSlider(SynthSlider* release_slider);
    void setSizeRatio(float ratio) { size_ratio_ = ratio; }

    void parentHierarchyChanged() override;
    void pickHoverPosition(Point<float> position);
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;

    void magnifyZoom(Point<float> delta);
    void magnifyReset();

    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void destroy(OpenGlWrapper& open_gl) override;
    void resetPositions() { reset_positions_ = true; }

  private:
    void setEditingCircleBounds();
    void setGridPositions();
    void setTimePositions();
    void setPointPositions();
    void setGlPositions();
    void setColors();
    void zoom(float amount);

    static std::pair<vital::Output*, vital::Output*> getOutputs(const vital::output_map& mono_modulations,
                                                                const vital::output_map& poly_modulations,
                                                                const String& name) {
      return {
        mono_modulations.at(name.toStdString()),
        poly_modulations.at(name.toStdString())
      };
    }
  
    vital::poly_float getOutputsTotal(std::pair<vital::Output*, vital::Output*> outputs,
                                      vital::poly_float default_value);
    void drawPosition(OpenGlWrapper& open_gl, int index);
    std::pair<float, float> getPosition(int index);

    float padX(float x);
    float padY(float y);
    float unpadX(float x);
    float unpadY(float y);
    float padOpenGlX(float x);
    float padOpenGlY(float y);

    float getSliderDelayX();
    float getSliderAttackX();
    float getSliderHoldX();
    float getSliderDecayX();
    float getSliderSustainY();
    float getSliderReleaseX();
    float getDelayTime(int index);
    float getAttackTime(int index);
    float getHoldTime(int index);
    float getDecayTime(int index);
    float getReleaseTime(int index);
    float getDelayX(int index);
    float getAttackX(int index);
    float getHoldX(int index);
    float getDecayX(int index);
    float getSustainY(int index);
    float getReleaseX(int index);

    float getBackupPhase(float phase, int index);
    vital::poly_float getBackupPhase(vital::poly_float phase);
    float getEnvelopeValue(float t, float power, float start, float end);
    float getSliderAttackValue(float t);
    float getSliderDecayValue(float t);
    float getSliderReleaseValue(float t);
    float getAttackValue(float t, int index);
    float getDecayValue(float t, int index);
    float getReleaseValue(float t, int index);

    void setDelayX(float x);
    void setAttackX(float x);
    void setHoldX(float x);
    void setDecayX(float x);
    void setSustainY(float y);
    void setReleaseX(float x);

    void setPower(SynthSlider* slider, float power);
    void setAttackPower(float power);
    void setDecayPower(float power);
    void setReleasePower(float power);

    SynthGuiInterface* parent_;
    bool delay_hover_;
    bool attack_hover_;
    bool hold_hover_;
    bool sustain_hover_;
    bool release_hover_;
    bool attack_power_hover_;
    bool decay_power_hover_;
    bool release_power_hover_;
    bool mouse_down_;
    Point<float> last_edit_position_;

    bool animate_;
    float size_ratio_;
    float window_time_;

    vital::poly_float current_position_alpha_;
    vital::poly_float last_phase_;

    Colour line_left_color_;
    Colour line_right_color_;
    Colour line_center_color_;
    Colour fill_left_color_;
    Colour fill_right_color_;
    Colour background_color_;
    Colour time_color_;

    bool reset_positions_;
    OpenGlQuad drag_circle_;
    OpenGlQuad hover_circle_;
    OpenGlMultiQuad grid_lines_;
    OpenGlMultiQuad sub_grid_lines_;
    OpenGlQuad position_circle_;
    OpenGlMultiQuad point_circles_;
    OpenGlMultiQuad power_circles_;
    std::unique_ptr<PlainTextComponent> times_[kMaxTimesShown];

    const vital::StatusOutput* envelope_phase_;

    SynthSlider* delay_slider_;
    SynthSlider* attack_slider_;
    SynthSlider* hold_slider_;
    SynthSlider* attack_power_slider_;
    SynthSlider* decay_slider_;
    SynthSlider* decay_power_slider_;
    SynthSlider* sustain_slider_;
    SynthSlider* release_slider_;
    SynthSlider* release_power_slider_;

    std::pair<vital::Output*, vital::Output*> delay_outputs_;
    std::pair<vital::Output*, vital::Output*> attack_outputs_;
    std::pair<vital::Output*, vital::Output*> hold_outputs_;
    std::pair<vital::Output*, vital::Output*> decay_outputs_;
    std::pair<vital::Output*, vital::Output*> sustain_outputs_;
    std::pair<vital::Output*, vital::Output*> release_outputs_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeEditor)
};
