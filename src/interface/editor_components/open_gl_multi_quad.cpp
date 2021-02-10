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

#include "open_gl_multi_quad.h"

#include "common.h"
#include "shaders.h"

OpenGlMultiQuad::OpenGlMultiQuad(int max_quads, Shaders::FragmentShader shader) :
    target_component_(nullptr), scissor_component_(nullptr), fragment_shader_(shader),
    max_quads_(max_quads), num_quads_(max_quads), draw_when_not_visible_(false),
    active_(true), dirty_(false), max_arc_(2.0f), thumb_amount_(0.5f), start_pos_(0.0f), 
    current_alpha_mult_(1.0f), alpha_mult_(1.0f), additive_blending_(false),
    current_thickness_(1.0f), thickness_(1.0f), rounding_(5.0f), shader_(nullptr) {
  static const int triangles[] = {
    0, 1, 2,
    2, 3, 0
  };

  data_ = std::make_unique<float[]>(max_quads_ * kNumFloatsPerQuad);
  indices_ = std::make_unique<int[]>(max_quads_ * kNumIndicesPerQuad);
  vertex_buffer_ = 0;
  indices_buffer_ = 0;

  mod_color_ = Colours::transparentBlack;

  for (int i = 0; i < max_quads_; ++i) {
    setCoordinates(i, -1.0f, -1.0f, 2.0f, 2.0f);
    setShaderValue(i, 1.0f);

    for (int j = 0; j < kNumIndicesPerQuad; ++j)
      indices_[i * kNumIndicesPerQuad + j] = triangles[j] + i * kNumVertices;
  }

  setInterceptsMouseClicks(false, false);
}

OpenGlMultiQuad::~OpenGlMultiQuad() { }

void OpenGlMultiQuad::init(OpenGlWrapper& open_gl) {
  open_gl.context.extensions.glGenBuffers(1, &vertex_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(max_quads_ * kNumFloatsPerQuad * sizeof(float));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &indices_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

  GLsizeiptr bar_size = static_cast<GLsizeiptr>(max_quads_ * kNumIndicesPerQuad * sizeof(int));
  open_gl.context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, bar_size, indices_.get(), GL_STATIC_DRAW);

  shader_ = open_gl.shaders->getShaderProgram(Shaders::kPassthroughVertex, fragment_shader_);
  shader_->use();
  color_uniform_ = getUniform(open_gl, *shader_, "color");
  alt_color_uniform_ = getUniform(open_gl, *shader_, "alt_color");
  mod_color_uniform_ = getUniform(open_gl, *shader_, "mod_color");
  background_color_uniform_ = getUniform(open_gl, *shader_, "background_color");
  thumb_color_uniform_ = getUniform(open_gl, *shader_, "thumb_color");
  position_ = getAttribute(open_gl, *shader_, "position");
  dimensions_ = getAttribute(open_gl, *shader_, "dimensions");
  coordinates_ = getAttribute(open_gl, *shader_, "coordinates");
  shader_values_ = getAttribute(open_gl, *shader_, "shader_values");
  thickness_uniform_ = getUniform(open_gl, *shader_, "thickness");
  rounding_uniform_ = getUniform(open_gl, *shader_, "rounding");
  max_arc_uniform_ = getUniform(open_gl, *shader_, "max_arc");
  thumb_amount_uniform_ = getUniform(open_gl, *shader_, "thumb_amount");
  start_pos_uniform_ = getUniform(open_gl, *shader_, "start_pos");
  alpha_mult_uniform_ = getUniform(open_gl, *shader_, "alpha_mult");
}

void OpenGlMultiQuad::destroy(OpenGlWrapper& open_gl) {
  shader_ = nullptr;
  position_ = nullptr;
  dimensions_ = nullptr;
  coordinates_ = nullptr;
  shader_values_ = nullptr;
  color_uniform_ = nullptr;
  alt_color_uniform_ = nullptr;
  mod_color_uniform_ = nullptr;
  thumb_color_uniform_ = nullptr;
  thickness_uniform_ = nullptr;
  rounding_uniform_ = nullptr;
  max_arc_uniform_ = nullptr;
  thumb_amount_uniform_ = nullptr;
  start_pos_uniform_ = nullptr;
  alpha_mult_uniform_ = nullptr;
  open_gl.context.extensions.glDeleteBuffers(1, &vertex_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &indices_buffer_);

  vertex_buffer_ = 0;
  indices_buffer_ = 0;
}

void OpenGlMultiQuad::render(OpenGlWrapper& open_gl, bool animate) {
  Component* component = target_component_ ? target_component_ : this;
  if (!active_ || (!draw_when_not_visible_ && !component->isVisible()) || !setViewPort(component, open_gl))
    return;

  Component* scissor_component = scissor_component_;
  if (scissor_component)
    setScissor(scissor_component, open_gl);

  if (current_alpha_mult_ == 0.0f && alpha_mult_ == 0.0f)
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
    
    for (int i = 0; i < num_quads_; ++i)
      setDimensions(i, getQuadWidth(i), getQuadHeight(i), component->getWidth(), component->getHeight());

    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

    GLsizeiptr vert_size = static_cast<GLsizeiptr>(kNumFloatsPerQuad * max_quads_ * sizeof(float));
    open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, data_.get(), GL_STATIC_DRAW);
    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  shader_->use();

  if (alpha_mult_ > current_alpha_mult_)
    current_alpha_mult_ = std::min(alpha_mult_, current_alpha_mult_ + kAlphaInc);
  else
    current_alpha_mult_ = std::max(alpha_mult_, current_alpha_mult_ - kAlphaInc);

  float alpha_color_mult = 1.0f;
  if (alpha_mult_uniform_)
    alpha_mult_uniform_->set(current_alpha_mult_);
  else
    alpha_color_mult = current_alpha_mult_;

  color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(),
                      color_.getFloatBlue(), alpha_color_mult * color_.getFloatAlpha());

  if (alt_color_uniform_) {
    if (alt_color_.getFloatAlpha()) {
      alt_color_uniform_->set(alt_color_.getFloatRed(), alt_color_.getFloatGreen(),
                              alt_color_.getFloatBlue(), alt_color_.getFloatAlpha());
    }
    else
      alt_color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(), color_.getFloatBlue(), 0.0f);
  }
  if (mod_color_uniform_) {
    if (mod_color_.getFloatAlpha()) {
      mod_color_uniform_->set(mod_color_.getFloatRed(), mod_color_.getFloatGreen(),
                              mod_color_.getFloatBlue(), mod_color_.getFloatAlpha());
    }
    else
      mod_color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(), color_.getFloatBlue(), 0.0f);
  }
  if (background_color_uniform_) {
    if (background_color_.getFloatAlpha()) {
      background_color_uniform_->set(background_color_.getFloatRed(), background_color_.getFloatGreen(),
                                     background_color_.getFloatBlue(), background_color_.getFloatAlpha());
    }
    else
      background_color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(), color_.getFloatBlue(), 0.0f);
  }

  if (thumb_color_uniform_) {
    thumb_color_uniform_->set(thumb_color_.getFloatRed(), thumb_color_.getFloatGreen(),
                              thumb_color_.getFloatBlue(), thumb_color_.getFloatAlpha());
  }

  if (thumb_amount_uniform_)
    thumb_amount_uniform_->set(thumb_amount_);

  if (start_pos_uniform_)
    start_pos_uniform_->set(start_pos_);

  if (thickness_uniform_) {
    current_thickness_ = current_thickness_ + kThicknessDecay * (thickness_ - current_thickness_);
    thickness_uniform_->set(current_thickness_);
  }
  if (rounding_uniform_)
    rounding_uniform_->set(rounding_);
  if (max_arc_uniform_)
    max_arc_uniform_->set(max_arc_);

  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

  open_gl.context.extensions.glVertexAttribPointer(position_->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, kNumFloatsPerVertex * sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(position_->attributeID);
  if (dimensions_) {
    open_gl.context.extensions.glVertexAttribPointer(dimensions_->attributeID, 2, GL_FLOAT,
                                                     GL_FALSE, kNumFloatsPerVertex * sizeof(float),
                                                     (GLvoid*)(2 * sizeof(float)));
    open_gl.context.extensions.glEnableVertexAttribArray(dimensions_->attributeID);
  }
  if (coordinates_) {
    open_gl.context.extensions.glVertexAttribPointer(coordinates_->attributeID, 2, GL_FLOAT,
                                                     GL_FALSE, kNumFloatsPerVertex * sizeof(float),
                                                     (GLvoid*)(4 * sizeof(float)));
    open_gl.context.extensions.glEnableVertexAttribArray(coordinates_->attributeID);
  }
  if (shader_values_) {
    open_gl.context.extensions.glVertexAttribPointer(shader_values_->attributeID, 4, GL_FLOAT,
                                                     GL_FALSE, kNumFloatsPerVertex * sizeof(float),
                                                     (GLvoid*)(6 * sizeof(float)));
    open_gl.context.extensions.glEnableVertexAttribArray(shader_values_->attributeID);
  }

  glDrawElements(GL_TRIANGLES, num_quads_ * kNumIndicesPerQuad, GL_UNSIGNED_INT, nullptr);

  open_gl.context.extensions.glDisableVertexAttribArray(position_->attributeID);
  if (dimensions_)
    open_gl.context.extensions.glDisableVertexAttribArray(dimensions_->attributeID);
  if (coordinates_)
    open_gl.context.extensions.glDisableVertexAttribArray(coordinates_->attributeID);
  if (shader_values_)
    open_gl.context.extensions.glDisableVertexAttribArray(shader_values_->attributeID);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}
