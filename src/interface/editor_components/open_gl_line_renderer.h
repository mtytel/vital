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

class OpenGlLineRenderer : public OpenGlComponent {
  public:
    static constexpr int kLineFloatsPerVertex = 3;
    static constexpr int kFillFloatsPerVertex = 4;
    static constexpr int kLineVerticesPerPoint = 6;
    static constexpr int kFillVerticesPerPoint = 2;
    static constexpr int kLineFloatsPerPoint = kLineVerticesPerPoint * kLineFloatsPerVertex;
    static constexpr int kFillFloatsPerPoint = kFillVerticesPerPoint * kFillFloatsPerVertex;

    OpenGlLineRenderer(int num_points, bool loop = false);
    virtual ~OpenGlLineRenderer();

    virtual void init(OpenGlWrapper& open_gl) override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    virtual void destroy(OpenGlWrapper& open_gl) override;

    force_inline void setColor(Colour color) { color_ = color; }
    force_inline void setLineWidth(float width) { line_width_ = width; }
    force_inline void setBoost(float boost) { boost_ = boost; }

    force_inline float boostLeftAt(int index) const { return boost_left_[index]; }
    force_inline float boostRightAt(int index) const { return boost_right_[index]; }
    force_inline float yAt(int index) const { return y_[index]; }
    force_inline float xAt(int index) const { return x_[index]; }

    force_inline void setBoostLeft(int index, float val) {
      boost_left_[index] = val;
      dirty_ = true;
      VITAL_ASSERT(num_points_ > index);
    }
    force_inline void setBoostRight(int index, float val) {
      boost_right_[index] = val;
      dirty_ = true;
      VITAL_ASSERT(num_points_ > index);
    }
    force_inline void setYAt(int index, float val) {
      y_[index] = val;
      dirty_ = true;
      VITAL_ASSERT(num_points_ > index);
    }
    force_inline void setXAt(int index, float val) {
      x_[index] = val;
      dirty_ = true;
      VITAL_ASSERT(num_points_ > index);
    }

    void setFillVertices(bool left);
    void setLineVertices(bool left);

    force_inline void setFill(bool fill) { fill_ = fill; }
    force_inline void setFillColor(Colour fill_color) {
      setFillColors(fill_color, fill_color);
    }
    force_inline void setFillColors(Colour fill_color_from, Colour fill_color_to) {
      fill_color_from_ = fill_color_from;
      fill_color_to_ = fill_color_to;
    }
    force_inline void setFillCenter(float fill_center) { fill_center_ = fill_center; }
    force_inline void setFit(bool fit) { fit_ = fit; }

    force_inline void setBoostAmount(float boost_amount) { boost_amount_ = boost_amount; }
    force_inline void setFillBoostAmount(float boost_amount) { fill_boost_amount_ = boost_amount; }
    force_inline void setIndex(int index) { index_ = index; }
    void boostLeftRange(float start, float end, int buffer_vertices, float min);
    void boostRightRange(float start, float end, int buffer_vertices, float min);
    void boostRange(float* boosts, float start, float end, int buffer_vertices, float min);
    void boostRange(vital::poly_float start, vital::poly_float end, int buffer_vertices, vital::poly_float min);
    void decayBoosts(vital::poly_float mult);
    void enableBackwardBoost(bool enable) { enable_backward_boost_ = enable; }

    force_inline int numPoints() const { return num_points_; }
    force_inline Colour color() const { return color_; }

    void drawLines(OpenGlWrapper& open_gl, bool left);
    bool anyBoostValue() { return any_boost_value_; }

  private:
    Colour color_;
    Colour fill_color_from_;
    Colour fill_color_to_;

    int num_points_;
    float line_width_;
    float boost_;
    bool fill_;
    float fill_center_;
    bool fit_;

    float boost_amount_;
    float fill_boost_amount_;
    bool enable_backward_boost_;
    int index_;

    bool dirty_;
    bool last_drawn_left_;
    bool last_negative_boost_;
    bool loop_;
    bool any_boost_value_;
    int num_padding_;
    int num_line_vertices_;
    int num_fill_vertices_;
    int num_line_floats_;
    int num_fill_floats_;

    OpenGLShaderProgram* shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> scale_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> color_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> boost_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> line_width_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;

    OpenGLShaderProgram* fill_shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> fill_scale_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> fill_color_from_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> fill_color_to_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> fill_center_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> fill_boost_amount_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> fill_position_;

    GLuint vertex_array_object_;
    GLuint line_buffer_;
    GLuint fill_buffer_;
    GLuint indices_buffer_;

    std::unique_ptr<float[]> x_;
    std::unique_ptr<float[]> y_;
    std::unique_ptr<float[]> boost_left_;
    std::unique_ptr<float[]> boost_right_;
    std::unique_ptr<float[]> line_data_;
    std::unique_ptr<float[]> fill_data_;
    std::unique_ptr<int[]> indices_data_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlLineRenderer)
};

