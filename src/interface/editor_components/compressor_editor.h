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

#include "line_generator.h"
#include "open_gl_component.h"
#include "open_gl_multi_quad.h"
#include "synth_slider.h"
#include "synth_module.h"

class SynthGuiInterface;

class CompressorEditor : public OpenGlComponent, public SynthSlider::SliderListener {
  public:
    static constexpr float kGrabRadius = 8.0f;
    static constexpr float kMinDb = -80.0f;
    static constexpr float kMaxDb = 0.0f;
    static constexpr float kDbEditBuffer = 1.0f;
    static constexpr float kMinEditDb = kMinDb + kDbEditBuffer;
    static constexpr float kMaxEditDb = kMaxDb - kDbEditBuffer;
    static constexpr float kMinLowerRatio = -1.0f;
    static constexpr float kMaxLowerRatio = 1.0f;
    static constexpr float kMinUpperRatio = 0.0f;
    static constexpr float kMaxUpperRatio = 1.0f;
    static constexpr float kRatioEditMultiplier = 0.6f;
    static constexpr float kCompressorAreaBuffer = 0.05f;
    static constexpr float kBarWidth = 1.0f / 5.0f;
    static constexpr float kInputLineRadius = 0.02f;
    static constexpr float kMouseMultiplier = 1.0f;
    static constexpr int kMaxBands = 3;
    static constexpr int kNumChannels = kMaxBands * 2;
    static constexpr int kDbLineSections = 8;
    static constexpr int kExtraDbLines = 6;
    static constexpr int kRatioDbLines = kDbLineSections + kExtraDbLines;
    static constexpr int kTotalRatioLines = kRatioDbLines * kNumChannels;

    CompressorEditor();
    virtual ~CompressorEditor();

    void paintBackground(Graphics& g) override;
    void resized() override;

    void mouseDown(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void parentHierarchyChanged() override;

    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void renderCompressor(OpenGlWrapper& open_gl, bool animate);
    void destroy(OpenGlWrapper& open_gl) override;
    void setSizeRatio(float ratio) { size_ratio_ = ratio; }
    void setAllValues(vital::control_map& controls);

    void setHighBandActive(bool active) { high_band_active_ = active; }
    void setLowBandActive(bool active) { low_band_active_ = active; }
    void setActive(bool active) { active_ = active; }

  private:
    enum DragPoint {
      kNone,
      kLowUpperThreshold,
      kBandUpperThreshold,
      kHighUpperThreshold,
      kLowLowerThreshold,
      kBandLowerThreshold,
      kHighLowerThreshold,
      kLowUpperRatio,
      kBandUpperRatio,
      kHighUpperRatio,
      kLowLowerRatio,
      kBandLowerRatio,
      kHighLowerRatio,
      kNumDragPoints
    };

    bool isRatio(DragPoint drag_point) {
      return drag_point == kLowLowerRatio || drag_point == kBandLowerRatio || drag_point == kHighLowerRatio ||
             drag_point == kLowUpperRatio || drag_point == kBandUpperRatio || drag_point == kHighUpperRatio;
    }

    void setThresholdPositions(int low_start, int low_end, int band_start, int band_end,
                               int high_start, int high_end, float ratio_match);
    void setRatioLines(int start_index, int start_x, int end_x, float threshold, float ratio, bool upper, bool hover);
    void setRatioLinePositions(int low_start, int low_end, int band_start, int band_end,
                               int high_start, int high_end);
    void renderHover(OpenGlWrapper& open_gl, int low_start, int low_end,
                     int band_start, int band_end, int high_start, int high_end);

    void setLowUpperThreshold(float db, bool clamp);
    void setBandUpperThreshold(float db, bool clamp);
    void setHighUpperThreshold(float db, bool clamp);
    void setLowLowerThreshold(float db, bool clamp);
    void setBandLowerThreshold(float db, bool clamp);
    void setHighLowerThreshold(float db, bool clamp);

    void setLowUpperRatio(float ratio);
    void setBandUpperRatio(float ratio);
    void setHighUpperRatio(float ratio);
    void setLowLowerRatio(float ratio);
    void setBandLowerRatio(float ratio);
    void setHighLowerRatio(float ratio);

    String formatValue(float value);

    DragPoint getHoverPoint(const MouseEvent& e);
    float getYForDb(float db);
    float getCompressedDb(float input_db, float upper_threshold, float upper_ratio,
                          float lower_threshold, float lower_ratio);
    Colour getColorForRatio(float ratio);

    SynthGuiInterface* parent_;
    SynthSection* section_parent_;

    DragPoint hover_;
    Point<int> last_mouse_position_;

    OpenGlQuad hover_quad_;
    OpenGlMultiQuad input_dbs_;
    OpenGlMultiQuad output_dbs_;
    OpenGlMultiQuad thresholds_;
    OpenGlMultiQuad ratio_lines_;

    float low_upper_threshold_;
    float band_upper_threshold_;
    float high_upper_threshold_;
    float low_lower_threshold_;
    float band_lower_threshold_;
    float high_lower_threshold_;
    float low_upper_ratio_;
    float band_upper_ratio_;
    float high_upper_ratio_;
    float low_lower_ratio_;
    float band_lower_ratio_;
    float high_lower_ratio_;

    const vital::StatusOutput* low_input_ms_;
    const vital::StatusOutput* band_input_ms_;
    const vital::StatusOutput* high_input_ms_;

    const vital::StatusOutput* low_output_ms_;
    const vital::StatusOutput* band_output_ms_;
    const vital::StatusOutput* high_output_ms_;

    float size_ratio_;
    bool animate_;
    bool active_;
    bool high_band_active_;
    bool low_band_active_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorEditor)
};

