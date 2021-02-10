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

#include "common.h"
#include "open_gl_component.h"
#include "open_gl_multi_quad.h"

class BarRenderer : public OpenGlComponent {
  public:
    static constexpr float kScaleConstant = 5.0f;
    static constexpr int kFloatsPerVertex = 3;
    static constexpr int kVerticesPerBar = 4;
    static constexpr int kFloatsPerBar = kVerticesPerBar * kFloatsPerVertex;
    static constexpr int kTriangleIndicesPerBar = 6;
    static constexpr int kCornerFloatsPerVertex = 2;
    static constexpr int kCornerFloatsPerBar = kVerticesPerBar * kCornerFloatsPerVertex;

    BarRenderer(int num_points, bool vertical = true);
    virtual ~BarRenderer();

    virtual void init(OpenGlWrapper& open_gl) override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    virtual void destroy(OpenGlWrapper& open_gl) override;

    void setColor(const Colour& color) { color_ = color; }
    void setScale(float scale) { scale_ = scale; }
    void setOffset(float offset) { offset_ = offset; }
    void setBarWidth(float bar_width) { bar_width_ = bar_width; }
    void setNumPoints(int num_points) { num_points_ = num_points; }
    float getBarWidth() { return bar_width_; }

    inline float xAt(int index) { return bar_data_[kFloatsPerBar * index]; }
    inline float rightAt(int index) { return bar_data_[kFloatsPerBar * index + kFloatsPerVertex]; }
    inline float yAt(int index) { return bar_data_[kFloatsPerBar * index + 1]; }
    inline float bottomAt(int index) { return bar_data_[kFloatsPerBar * index + 2 * kFloatsPerVertex + 1]; }

    force_inline void setX(int index, float val) {
      bar_data_[kFloatsPerBar * index] = val;
      bar_data_[kFloatsPerBar * index + 2 * kFloatsPerVertex] = val;
      bar_data_[kFloatsPerBar * index + kFloatsPerVertex] = val;
      bar_data_[kFloatsPerBar * index + 3 * kFloatsPerVertex] = val;
      dirty_ = true;
    }

    force_inline void setY(int index, float val) {
      bar_data_[kFloatsPerBar * index + 1] = val;
      bar_data_[kFloatsPerBar * index + kFloatsPerVertex + 1] = val;
      dirty_ = true;
    }

    force_inline void setBottom(int index, float val) {
      bar_data_[kFloatsPerBar * index + 2 * kFloatsPerVertex + 1] = val;
      bar_data_[kFloatsPerBar * index + 3 * kFloatsPerVertex + 1] = val;
      dirty_ = true;
    }

    inline void positionBar(int index, float x, float y, float width, float height) {
      bar_data_[kFloatsPerBar * index] = x;
      bar_data_[kFloatsPerBar * index + 1] = y;

      bar_data_[kFloatsPerBar * index + kFloatsPerVertex] = x + width;
      bar_data_[kFloatsPerBar * index + kFloatsPerVertex + 1] = y;

      bar_data_[kFloatsPerBar * index + 2 * kFloatsPerVertex] = x;
      bar_data_[kFloatsPerBar * index + 2 * kFloatsPerVertex + 1] = y + height;

      bar_data_[kFloatsPerBar * index + 3 * kFloatsPerVertex] = x + width;
      bar_data_[kFloatsPerBar * index + 3 * kFloatsPerVertex + 1] = y + height;
      dirty_ = true;
    }

    void setBarSizes();

    void setPowerScale(bool scale);
    void setSquareScale(bool scale);

    force_inline float scaledYAt(int index) {
      float value = yAt(index) * 0.5f + 0.5f;
      if (square_scale_)
        value *= value;
      if (power_scale_)
        value /= std::max(index, 1) / kScaleConstant;

      return value;
    }

    force_inline void setScaledY(int index, float val) {
      float value = val;

      if (power_scale_)
        value *= std::max(index, 1) / kScaleConstant;
      if (square_scale_)
        value = sqrtf(value);

      setY(index, 2.0f * value - 1.0f);
    }
    force_inline void setAdditiveBlending(bool additive_blending) { additive_blending_ = additive_blending; }

  protected:
    void drawBars(OpenGlWrapper& open_gl);

    OpenGLShaderProgram* shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> color_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> dimensions_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> offset_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> scale_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> width_percent_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> corner_;

    Colour color_;
    bool vertical_;
    float scale_;
    float offset_;
    float bar_width_;
    bool additive_blending_;
    float display_scale_;

    bool power_scale_;
    bool square_scale_;
    bool dirty_;
    int num_points_;
    int total_points_;
    std::unique_ptr<float[]> bar_data_;
    std::unique_ptr<float[]> bar_corner_data_;
    std::unique_ptr<int[]> bar_indices_;
    GLuint bar_buffer_;
    GLuint bar_corner_buffer_;
    GLuint bar_indices_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarRenderer)
};

