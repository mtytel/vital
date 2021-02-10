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

#include "open_gl_line_renderer.h"

#include "shaders.h"
#include "utils.h"

namespace {
  constexpr float kLoopWidth = 2.001f;
  constexpr float kDefaultLineWidth = 7.0f;

  force_inline float inverseSqrt(float value) {
    static constexpr float kThreeHalves = 1.5f;
    int i;
    float x2, y;

    x2 = value * 0.5f;
    y = value;
    i = *(int *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *)&i;
    y = y * (kThreeHalves - (x2 * y * y));
    y = y * (kThreeHalves - (x2 * y * y));
    return y;
  }

  force_inline float inverseMagnitudeOfPoint(Point<float> point) {
    return inverseSqrt(point.x * point.x + point.y * point.y);
  }

  force_inline Point<float> normalize(Point<float> point) {
    return point * inverseMagnitudeOfPoint(point);
  }
}

OpenGlLineRenderer::OpenGlLineRenderer(int num_points, bool loop) :
    num_points_(num_points), boost_(0.0f), fill_(false), fill_center_(0.0f), fit_(false),
    boost_amount_(0.0f), fill_boost_amount_(0.0f), enable_backward_boost_(true),
    index_(0), dirty_(false), last_drawn_left_(false), loop_(loop), any_boost_value_(false),
    shader_(nullptr), fill_shader_(nullptr) {
  addRoundedCorners();
  num_padding_ = 1;
  if (loop)
    num_padding_ = 2;

  num_line_vertices_ = kLineVerticesPerPoint * (num_points_ + 2 * num_padding_);
  num_fill_vertices_ = kFillVerticesPerPoint * (num_points_ + 2 * num_padding_);
  num_line_floats_ = kLineFloatsPerVertex * num_line_vertices_;
  num_fill_floats_ = kFillFloatsPerVertex * num_fill_vertices_;

  line_width_ = kDefaultLineWidth;

  x_ = std::make_unique<float[]>(num_points_);
  y_ = std::make_unique<float[]>(num_points_);
  boost_left_ = std::make_unique<float[]>(num_points_);
  boost_right_ = std::make_unique<float[]>(num_points_);

  line_data_ = std::make_unique<float[]>(num_line_floats_);
  fill_data_ = std::make_unique<float[]>(num_fill_floats_);
  indices_data_ = std::make_unique<int[]>(num_line_vertices_);
  vertex_array_object_ = 0;
  line_buffer_ = 0;
  fill_buffer_ = 0;
  indices_buffer_ = 0;
  last_negative_boost_ = false;

  for (int i = 0; i < num_line_vertices_; ++i)
    indices_data_[i] = i;

  for (int i = 0; i < num_line_floats_; i += 2 * kLineFloatsPerVertex)
    line_data_[i + 2] = 1.0f;

  for (int i = 0; i < num_points_; ++i)
    setXAt(i, 2.0f * i / (num_points_ - 1.0f) - 1.0f);
}

OpenGlLineRenderer::~OpenGlLineRenderer() { }

void OpenGlLineRenderer::init(OpenGlWrapper& open_gl) {
  OpenGlComponent::init(open_gl);

  open_gl.context.extensions.glGenVertexArrays(1, &vertex_array_object_);
  open_gl.context.extensions.glBindVertexArray(vertex_array_object_);

  open_gl.context.extensions.glGenBuffers(1, &line_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

  GLsizeiptr line_vert_size = static_cast<GLsizeiptr>(num_line_floats_ * sizeof(float));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, line_vert_size, line_data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &fill_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, fill_buffer_);

  GLsizeiptr fill_vert_size = static_cast<GLsizeiptr>(num_fill_floats_ * sizeof(float));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, fill_vert_size, fill_data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &indices_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

  GLsizeiptr line_size = static_cast<GLsizeiptr>(num_line_vertices_ * sizeof(int));
  open_gl.context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_size, indices_data_.get(), GL_STATIC_DRAW);

  shader_ = open_gl.shaders->getShaderProgram(Shaders::kLineVertex, Shaders::kLineFragment);
  shader_->use();
  color_uniform_ = getUniform(open_gl, *shader_, "color");
  scale_uniform_ = getUniform(open_gl, *shader_, "scale");
  boost_uniform_ = getUniform(open_gl, *shader_, "boost");
  line_width_uniform_ = getUniform(open_gl, *shader_, "line_width");
  position_ = getAttribute(open_gl, *shader_, "position");

  fill_shader_ = open_gl.shaders->getShaderProgram(Shaders::kFillVertex, Shaders::kFillFragment);
  fill_shader_->use();
  fill_color_from_uniform_ = getUniform(open_gl, *fill_shader_, "color_from");
  fill_color_to_uniform_ = getUniform(open_gl, *fill_shader_, "color_to");
  fill_center_uniform_ = getUniform(open_gl, *fill_shader_, "center_position");
  fill_boost_amount_uniform_ = getUniform(open_gl, *fill_shader_, "boost_amount");
  fill_scale_uniform_ = getUniform(open_gl, *fill_shader_, "scale");
  fill_position_ = getAttribute(open_gl, *fill_shader_, "position");
}

void OpenGlLineRenderer::boostLeftRange(float start, float end, int buffer_vertices, float min) {
  boostRange(boost_left_.get(), start, end, buffer_vertices, min);
}

void OpenGlLineRenderer::boostRightRange(float start, float end, int buffer_vertices, float min) {
  boostRange(boost_right_.get(), start, end, buffer_vertices, min);
}

void OpenGlLineRenderer::boostRange(float* boosts, float start, float end, int buffer_vertices, float min) {
  any_boost_value_ = true;
  dirty_ = true;
  
  int active_points = num_points_ - 2 * buffer_vertices;
  int start_index = std::max((int)std::ceil(start * (active_points - 1)), 0);
  float end_position = end * (active_points - 1);
  int end_index = std::max(std::ceil(end_position), 0.0f);
  float progress = end_position - (int)end_position;

  start_index %= active_points;
  end_index %= active_points;
  int num_points = end_index - start_index;
  int direction = 1;
  if (enable_backward_boost_) {
    if ((num_points < 0 && num_points > -num_points_ / 2) || (num_points == 0 && last_negative_boost_)) {
      num_points = -num_points;
      direction = -1;
    }
    else if (num_points > num_points_ / 2) {
      num_points -= active_points;
      num_points = -num_points;
      direction = -1;
    }
  }

  last_negative_boost_ = direction < 0;
  if (last_negative_boost_) {
    start_index = std::max((int)std::floor(start * (active_points - 1)), 0);
    end_index = std::max(std::floor(end_position), 0.0f);
    start_index %= active_points;
    end_index %= active_points;

    num_points = start_index - end_index;
    progress = 1.0f - progress;
  }

  float delta = (1.0f - min) / num_points;
  float val = min;

  for (int i = start_index; i != end_index; i = (i + active_points + direction) % active_points) {
    val += delta;
    val = std::min(1.0f, val);
    float last_value = boosts[i + buffer_vertices];
    boosts[i + buffer_vertices] = std::max(last_value, val);
  }

  float end_value = boosts[end_index + buffer_vertices];
  boosts[end_index + buffer_vertices] = std::max(end_value, progress * progress);
}

void OpenGlLineRenderer::boostRange(vital::poly_float start, vital::poly_float end,
                                    int buffer_vertices, vital::poly_float min) {
  boostLeftRange(start[0], end[0], buffer_vertices, min[0]);
  boostRightRange(start[1], end[1], buffer_vertices, min[1]);
}

void OpenGlLineRenderer::decayBoosts(vital::poly_float mult) {
  bool any_boost = false;
  for (int i = 0; i < num_points_; ++i) {
    boost_left_[i] *= mult[0];
    boost_right_[i] *= mult[1];
    any_boost = any_boost || boost_left_[i] || boost_right_[i];
  }
  
  any_boost_value_ = any_boost;
}

void OpenGlLineRenderer::setFillVertices(bool left) {
  float* boosts = left ? boost_left_.get() : boost_right_.get();
  float x_adjust = 2.0f / getWidth();
  float y_adjust = 2.0f / getHeight();

  for (int i = 0; i < num_points_; ++i) {
    int index_top = (i + num_padding_) * kFillFloatsPerPoint;
    int index_bottom = index_top + kFillFloatsPerVertex;
    float x = x_adjust * x_[i] - 1.0f;
    float y = 1.0f - y_adjust * y_[i];
    fill_data_[index_top] = x;
    fill_data_[index_top + 1] = y;
    fill_data_[index_top + 2] = boosts[i];
    fill_data_[index_bottom] = x;
    fill_data_[index_bottom + 1] = fill_center_;
    fill_data_[index_bottom + 2] = boosts[i];
  }

  int padding_copy_size = num_padding_ * kFillFloatsPerPoint * sizeof(float);
  int begin_copy_source = num_padding_ * kFillFloatsPerPoint;
  int end_copy_source = num_points_ * kFillFloatsPerPoint;
  if (loop_ && num_points_ >= 2) {
    memcpy(fill_data_.get(), fill_data_.get() + end_copy_source, padding_copy_size);
    int begin_copy_dest = (num_padding_ + num_points_) * kFillFloatsPerPoint;
    memcpy(fill_data_.get() + begin_copy_dest, fill_data_.get() + begin_copy_source, padding_copy_size);

    for (int i = 0; i < num_padding_; ++i) {
      fill_data_[i * kFillFloatsPerPoint] -= kLoopWidth;
      fill_data_[i * kFillFloatsPerPoint + kFillFloatsPerVertex] -= kLoopWidth;
      fill_data_[begin_copy_dest + i * kFillFloatsPerPoint] += kLoopWidth;
      fill_data_[begin_copy_dest + i * kFillFloatsPerPoint + kFillFloatsPerVertex] += kLoopWidth;
    }
  }
  else {
    int end_copy_dest = (num_padding_ + num_points_) * kFillFloatsPerPoint;
    memcpy(fill_data_.get() + end_copy_dest, fill_data_.get() + end_copy_source, padding_copy_size);
    memcpy(fill_data_.get(), fill_data_.get() + begin_copy_source, padding_copy_size);
  }
}

void OpenGlLineRenderer::setLineVertices(bool left) {
  float* boosts = left ? boost_left_.get() : boost_right_.get();

  Point<float> prev_normalized_delta;
  for (int i = 0; i < num_points_ - 1; ++i) {
    if (x_[i] != x_[i + 1] || y_[i] != y_[i + 1]) {
      prev_normalized_delta = normalize(Point<float>(x_[i + 1] - x_[i], y_[i + 1] - y_[i]));
      break;
    }
  }

  Point<float> prev_delta_normal(-prev_normalized_delta.y, prev_normalized_delta.x);
  float line_radius = line_width_ / 2.0f + 0.5f;
  float prev_magnitude = line_radius;

  float x_adjust = 2.0f / getWidth();
  float y_adjust = 2.0f / getHeight();

  for (int i = 0; i < num_points_; ++i) {
    float radius = line_radius * (1.0f + boost_amount_ * boosts[i]);
    Point<float> point(x_[i], y_[i]);
    int next_index = i + 1;
    int clamped_next_index = std::min(next_index, num_points_ - 1);

    Point<float> next_point(x_[clamped_next_index], y_[clamped_next_index]);
    Point<float> delta = next_point - point;
    if (point == next_point) {
      delta = prev_normalized_delta;
      next_point = point + delta;
    }

    float inverse_magnitude = inverseMagnitudeOfPoint(delta);
    float magnitude = 1.0f / std::max(0.00001f, inverse_magnitude);
    Point<float> normalized_delta(delta.x * inverse_magnitude, delta.y * inverse_magnitude);
    Point<float> delta_normal = Point<float>(-normalized_delta.y, normalized_delta.x);

    Point<float> angle_bisect_delta = normalized_delta - prev_normalized_delta;
    Point<float> bisect_line;
    bool straight = angle_bisect_delta.x < 0.001f && angle_bisect_delta.x > -0.001f && 
                    angle_bisect_delta.y < 0.001f && angle_bisect_delta.y > -0.001f;
    if (straight)
      bisect_line = delta_normal;
    else
      bisect_line = normalize(angle_bisect_delta);

    float x1, x2, x3, x4, x5, x6;
    float y1, y2, y3, y4, y5, y6;

    float max_inner_radius = std::max(radius, 0.5f * (magnitude + prev_magnitude));
    prev_magnitude = magnitude;

    float bisect_normal_dot_product = bisect_line.getDotProduct(delta_normal);
    float inner_mult = 1.0f / std::max(0.1f, std::fabs(bisect_normal_dot_product));
    Point<float> inner_point = point + std::min(inner_mult * radius, max_inner_radius) * bisect_line;
    Point<float> outer_point = point - bisect_line * radius;

    if (bisect_normal_dot_product < 0.0f) {
      Point<float> outer_point_start = outer_point;
      Point<float> outer_point_end = outer_point;
      if (!straight) {
        outer_point_start = point + prev_delta_normal * radius;
        outer_point_end = point + delta_normal * radius;
      }
      x1 = outer_point_start.x;
      y1 = outer_point_start.y;
      x3 = outer_point.x;
      y3 = outer_point.y;
      x5 = outer_point_end.x;
      y5 = outer_point_end.y;
      x2 = x4 = x6 = inner_point.x;
      y2 = y4 = y6 = inner_point.y;
    }
    else {
      Point<float> outer_point_start = outer_point;
      Point<float> outer_point_end = outer_point;
      if (!straight) {
        outer_point_start = point - prev_delta_normal * radius;
        outer_point_end = point - delta_normal * radius;
      }
      x2 = outer_point_start.x;
      y2 = outer_point_start.y;
      x4 = outer_point.x;
      y4 = outer_point.y;
      x6 = outer_point_end.x;
      y6 = outer_point_end.y;
      x1 = x3 = x5 = inner_point.x;
      y1 = y3 = y5 = inner_point.y;
    }

    int first = (i + num_padding_) * kLineFloatsPerPoint;
    int second = first + kLineFloatsPerVertex;
    int third = second + kLineFloatsPerVertex;
    int fourth = third + kLineFloatsPerVertex;
    int fifth = fourth + kLineFloatsPerVertex;
    int sixth = fifth + kLineFloatsPerVertex;

    line_data_[first] = x_adjust * x1 - 1.0f;
    line_data_[first + 1] = 1.0f - y_adjust * y1;

    line_data_[second] = x_adjust * x2 - 1.0f;
    line_data_[second + 1] = 1.0f - y_adjust * y2;

    line_data_[third] = x_adjust * x3 - 1.0f;
    line_data_[third + 1] = 1.0f - y_adjust * y3;

    line_data_[fourth] = x_adjust * x4 - 1.0f;
    line_data_[fourth + 1] = 1.0f - y_adjust * y4;

    line_data_[fifth] = x_adjust * x5 - 1.0f;
    line_data_[fifth + 1] = 1.0f - y_adjust * y5;

    line_data_[sixth] = x_adjust * x6 - 1.0f;
    line_data_[sixth + 1] = 1.0f - y_adjust * y6;

    prev_delta_normal = delta_normal;
    prev_normalized_delta = normalized_delta;
  }

  int begin_copy_dest = (num_padding_ + num_points_) * kLineFloatsPerPoint;
  if (loop_ && num_points_ >= 2) {
    int padding_copy_size = num_padding_ * kLineFloatsPerPoint * sizeof(float);
    int begin_copy_source = num_padding_ * kLineFloatsPerPoint;
    int end_copy_source = num_points_ * kLineFloatsPerPoint;

    memcpy(line_data_.get(), line_data_.get() + end_copy_source, padding_copy_size);
    memcpy(line_data_.get() + begin_copy_dest, line_data_.get() + begin_copy_source, padding_copy_size);

    for (int i = 0; i < num_padding_ * kLineVerticesPerPoint; ++i) {
      line_data_[i * kLineFloatsPerVertex] -= kLoopWidth;
      line_data_[begin_copy_dest + i * kLineFloatsPerVertex] += kLoopWidth;
    }
  }
  else {
    Point<float> delta_start(xAt(0) - xAt(1), yAt(0) - yAt(1));
    Point<float> delta_start_offset = normalize(delta_start) * line_radius;
    Point<float> delta_end(xAt(num_points_ - 1) - xAt(num_points_ - 2), yAt(num_points_ - 1) - yAt(num_points_ - 2));
    Point<float> delta_end_offset = normalize(delta_end) * line_radius;
    for (int i = 0; i < kLineVerticesPerPoint; ++i) {
      line_data_[i * kLineFloatsPerVertex] = (xAt(0) + delta_start_offset.x) * x_adjust - 1.0f;
      line_data_[i * kLineFloatsPerVertex + 1] = 1.0f - (yAt(0) + delta_start_offset.y) * y_adjust;
      line_data_[i * kLineFloatsPerVertex + 2] = boosts[0];

      int copy_index_start = begin_copy_dest + i * kLineFloatsPerVertex;
      line_data_[copy_index_start] = (xAt(num_points_ - 1) + delta_end_offset.x) * x_adjust - 1.0f;
      line_data_[copy_index_start + 1] = 1.0f - (yAt(num_points_ - 1) + delta_end_offset.y) * y_adjust;
      line_data_[copy_index_start + 2] = boosts[num_points_ - 1];
    }
  }
}

void OpenGlLineRenderer::drawLines(OpenGlWrapper& open_gl, bool left) {
  if (!setViewPort(open_gl))
    return;

  if (fill_shader_ == nullptr)
    init(open_gl);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);

  open_gl.context.extensions.glBindVertexArray(vertex_array_object_);

  if (dirty_ || last_drawn_left_ != left) {
    dirty_ = false;
    last_drawn_left_ = left;
    setLineVertices(left);
    setFillVertices(left);

    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);

    GLsizeiptr line_vert_size = static_cast<GLsizeiptr>(num_line_floats_ * sizeof(float));
    open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, line_vert_size, line_data_.get(), GL_STATIC_DRAW);

    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, fill_buffer_);

    GLsizeiptr fill_vert_size = static_cast<GLsizeiptr>(num_fill_floats_ * sizeof(float));
    open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, fill_vert_size, fill_data_.get(), GL_STATIC_DRAW);

    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

  float x_shrink = 1.0f;
  float y_shrink = 1.0f;
  if (fit_) {
    x_shrink = 1.0f - 0.33f * line_width_ / getWidth();
    y_shrink = 1.0f - 0.33f * line_width_ / getHeight();
  }

  if (fill_) {
    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, fill_buffer_);
    fill_shader_->use();
    fill_color_from_uniform_->set(fill_color_from_.getFloatRed(), fill_color_from_.getFloatGreen(),
                                  fill_color_from_.getFloatBlue(), fill_color_from_.getFloatAlpha());
    fill_color_to_uniform_->set(fill_color_to_.getFloatRed(), fill_color_to_.getFloatGreen(),
                                fill_color_to_.getFloatBlue(), fill_color_to_.getFloatAlpha());
    fill_center_uniform_->set(fill_center_);
    fill_boost_amount_uniform_->set(fill_boost_amount_);
    fill_scale_uniform_->set(x_shrink, y_shrink);

    open_gl.context.extensions.glVertexAttribPointer(fill_position_->attributeID, kFillFloatsPerVertex, GL_FLOAT,
                                                     GL_FALSE, kFillFloatsPerVertex * sizeof(float), nullptr);
    open_gl.context.extensions.glEnableVertexAttribArray(fill_position_->attributeID);
    glDrawElements(GL_TRIANGLE_STRIP, num_fill_vertices_, GL_UNSIGNED_INT, nullptr);
  }

  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, line_buffer_);
  shader_->use();
  open_gl.context.extensions.glVertexAttribPointer(position_->attributeID, kLineFloatsPerVertex, GL_FLOAT,
                                                  GL_FALSE, kLineFloatsPerVertex * sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(position_->attributeID);
  color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(), color_.getFloatBlue(), color_.getFloatAlpha());

  scale_uniform_->set(x_shrink, y_shrink);
  boost_uniform_->set(boost_);
  line_width_uniform_->set(line_width_);

  glDrawElements(GL_TRIANGLE_STRIP, num_line_vertices_, GL_UNSIGNED_INT, nullptr);

  open_gl.context.extensions.glDisableVertexAttribArray(position_->attributeID);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}

void OpenGlLineRenderer::render(OpenGlWrapper& open_gl, bool animate) {
  drawLines(open_gl, true);
}

void OpenGlLineRenderer::destroy(OpenGlWrapper& open_gl) {
  OpenGlComponent::destroy(open_gl);

  shader_ = nullptr;
  position_ = nullptr;
  color_uniform_ = nullptr;
  scale_uniform_ = nullptr;
  boost_uniform_ = nullptr;
  line_width_uniform_ = nullptr;

  fill_shader_ = nullptr;
  fill_color_from_uniform_ = nullptr;
  fill_color_to_uniform_ = nullptr;
  fill_center_uniform_ = nullptr;
  fill_boost_amount_uniform_ = nullptr;
  fill_scale_uniform_ = nullptr;
  fill_position_ = nullptr;

  open_gl.context.extensions.glDeleteBuffers(1, &line_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &fill_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &indices_buffer_);

  vertex_array_object_ = 0;
  line_buffer_ = 0;
  fill_buffer_ = 0;
  indices_buffer_ = 0;
}
