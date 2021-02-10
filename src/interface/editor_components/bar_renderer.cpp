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

#include "bar_renderer.h"

#include "skin.h"
#include "shaders.h"
#include "utils.h"

BarRenderer::BarRenderer(int num_points, bool vertical) :
    shader_(nullptr), vertical_(vertical), additive_blending_(true), display_scale_(1.0f), power_scale_(false),
    square_scale_(false), dirty_(false), num_points_(num_points), total_points_(num_points) {
  addRoundedCorners();

  scale_ = 1.0f;
  offset_ = 0.0f;
  bar_data_ = std::make_unique<float[]>(kFloatsPerBar * total_points_);
  bar_indices_ = std::make_unique<int[]>(kTriangleIndicesPerBar * total_points_);
  bar_corner_data_ = std::make_unique<float[]>(kCornerFloatsPerBar * total_points_);
  bar_buffer_ = 0;
  bar_corner_buffer_ = 0;
  bar_indices_buffer_ = 0;
  bar_width_ = 1.0f;

  for (int i = 0; i < total_points_; ++i) {
    float t = i / (total_points_ * 1.0f);
    int index = i * kFloatsPerBar;

    for (int v = 0; v < kVerticesPerBar; ++v) {
      int vertex_index = v * kFloatsPerVertex + index;
      bar_data_[vertex_index] = 2.0f * t - 1.0f;
      bar_data_[vertex_index + 1] = -1.0f;
    }

    int bar_index = i * kTriangleIndicesPerBar;
    int bar_vertex_index = i * kVerticesPerBar;
    bar_indices_[bar_index] = bar_vertex_index;
    bar_indices_[bar_index + 1] = bar_vertex_index + 1;
    bar_indices_[bar_index + 2] = bar_vertex_index + 2;
    bar_indices_[bar_index + 3] = bar_vertex_index + 1;
    bar_indices_[bar_index + 4] = bar_vertex_index + 2;
    bar_indices_[bar_index + 5] = bar_vertex_index + 3;

    int corner_index = i * kCornerFloatsPerBar;
    bar_corner_data_[corner_index] = 0.0f;
    bar_corner_data_[corner_index + 1] = 1.0f;
    bar_corner_data_[corner_index + 2] = 1.0f;
    bar_corner_data_[corner_index + 3] = 1.0f;
    bar_corner_data_[corner_index + 4] = 0.0f;
    bar_corner_data_[corner_index + 5] = 0.0f;
    bar_corner_data_[corner_index + 6] = 1.0f;
    bar_corner_data_[corner_index + 7] = 0.0f;
  }
}

BarRenderer::~BarRenderer() { }

void BarRenderer::init(OpenGlWrapper& open_gl) {
  OpenGlComponent::init(open_gl);

  open_gl.context.extensions.glGenBuffers(1, &bar_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, bar_buffer_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(kFloatsPerBar * total_points_ * sizeof(float));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, bar_data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &bar_corner_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, bar_corner_buffer_);

  GLsizeiptr corner_size = static_cast<GLsizeiptr>(kCornerFloatsPerBar * total_points_ * sizeof(float));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, corner_size, bar_corner_data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &bar_indices_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bar_indices_buffer_);

  GLsizeiptr bar_size = static_cast<GLsizeiptr>(kTriangleIndicesPerBar * total_points_ * sizeof(int));
  open_gl.context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, bar_size, bar_indices_.get(), GL_STATIC_DRAW);

  if (vertical_)
    shader_ = open_gl.shaders->getShaderProgram(Shaders::kBarVerticalVertex, Shaders::kBarFragment);
  else
    shader_ = open_gl.shaders->getShaderProgram(Shaders::kBarHorizontalVertex, Shaders::kBarFragment);

  shader_->use();
  color_uniform_ = getUniform(open_gl, *shader_, "color");
  dimensions_uniform_ = getUniform(open_gl, *shader_, "dimensions");
  offset_uniform_ = getUniform(open_gl, *shader_, "offset");
  scale_uniform_ = getUniform(open_gl, *shader_, "scale");
  width_percent_uniform_ = getUniform(open_gl, *shader_, "width_percent");
  position_ = getAttribute(open_gl, *shader_, "position");
  corner_ = getAttribute(open_gl, *shader_, "corner");
}

void BarRenderer::drawBars(OpenGlWrapper& open_gl) {
  if (!setViewPort(open_gl))
    return;

  if (shader_ == nullptr)
    init(open_gl);

  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);
  if (additive_blending_)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  else
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (dirty_) {
    dirty_ = false;
    setBarSizes();
    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, bar_buffer_);

    GLsizeiptr vert_size = static_cast<GLsizeiptr>(kFloatsPerBar * total_points_ * sizeof(float));
    open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, bar_data_.get(), GL_STATIC_DRAW);
    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  shader_->use();

  color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(),
                      color_.getFloatBlue(), color_.getFloatAlpha());
  dimensions_uniform_->set(getWidth(), getHeight());
  offset_uniform_->set(offset_);
  scale_uniform_->set(scale_);
  float min_width = 4.0f / getWidth();
  width_percent_uniform_->set(std::max(min_width, bar_width_ * scale_ * 2.0f / num_points_));
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, bar_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bar_indices_buffer_);

  open_gl.context.extensions.glVertexAttribPointer(position_->attributeID, kFloatsPerVertex, GL_FLOAT,
                                                   GL_FALSE, kFloatsPerVertex * sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(position_->attributeID);

  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, bar_corner_buffer_);
  open_gl.context.extensions.glVertexAttribPointer(corner_->attributeID, kCornerFloatsPerVertex, GL_FLOAT,
                                                  GL_FALSE, kCornerFloatsPerVertex * sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(corner_->attributeID);

  glDrawElements(GL_TRIANGLES, kTriangleIndicesPerBar * total_points_, GL_UNSIGNED_INT, nullptr);

  open_gl.context.extensions.glDisableVertexAttribArray(position_->attributeID);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}

void BarRenderer::render(OpenGlWrapper& open_gl, bool animate) {
  drawBars(open_gl);
}

void BarRenderer::destroy(OpenGlWrapper& open_gl) {
  OpenGlComponent::destroy(open_gl);

  shader_ = nullptr;
  position_ = nullptr;
  corner_ = nullptr;
  color_uniform_ = nullptr;
  dimensions_uniform_ = nullptr;
  offset_uniform_ = nullptr;
  scale_uniform_ = nullptr;
  width_percent_uniform_ = nullptr;
  open_gl.context.extensions.glDeleteBuffers(1, &bar_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &bar_indices_buffer_);

  bar_buffer_ = 0;
  bar_indices_buffer_ = 0;
}

void BarRenderer::setBarSizes() {
  if (vertical_) {
    for (int i = 0; i < total_points_; ++i) {
      int index = i * kFloatsPerBar;
      float height = fabsf(yAt(i) - bottomAt(i)) * 0.5f * display_scale_;

      bar_data_[index + 2] = height;
      bar_data_[index + kFloatsPerVertex + 2] = height;
      bar_data_[index + 2 * kFloatsPerVertex + 2] = height;
      bar_data_[index + 3 * kFloatsPerVertex + 2] = height;
    }
  }
  else {
    for (int i = 0; i < total_points_; ++i) {
      int index = i * kFloatsPerBar;
      float width = fabsf(xAt(i) - rightAt(i)) * 0.5f * display_scale_;

      bar_data_[index + 2] = width;
      bar_data_[index + kFloatsPerVertex + 2] = width;
      bar_data_[index + 2 * kFloatsPerVertex + 2] = width;
      bar_data_[index + 3 * kFloatsPerVertex + 2] = width;
    }
  }
}

void BarRenderer::setPowerScale(bool power_scale) {
  if (power_scale == power_scale_)
    return;

  bool old_power_scale = power_scale_;
  for (int i = 1; i < num_points_; ++i) {
    power_scale_ = old_power_scale;
    float old_y = scaledYAt(i);
    power_scale_ = power_scale;
    setScaledY(i, old_y);
  }

  dirty_ = true;
}

void BarRenderer::setSquareScale(bool square_scale) {
  if (square_scale == square_scale_)
    return;

  bool old_square_scale = square_scale_;
  for (int i = 0; i < num_points_; ++i) {
    square_scale_ = old_square_scale;
    float old_y = scaledYAt(i);
    square_scale_ = square_scale;
    setScaledY(i, old_y);
  }

  dirty_ = true;
}
