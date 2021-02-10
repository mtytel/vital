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

#include "open_gl_multi_image.h"

#include "common.h"
#include "shaders.h"

OpenGlMultiImage::OpenGlMultiImage(int max_quads) : image_(nullptr), image_width_(0), image_height_(0),
                                                    max_quads_(max_quads), num_quads_(max_quads),
                                                    dirty_(false), additive_blending_(false), image_shader_(nullptr) {
  static const int triangles[] = {
    0, 1, 2,
    2, 3, 0
  };

  data_ = std::make_unique<float[]>(max_quads_ * kNumFloatsPerQuad);
  indices_ = std::make_unique<int[]>(max_quads_ * kNumIndicesPerQuad);
  vertex_buffer_ = 0;
  indices_buffer_ = 0;

  for (int i = 0; i < max_quads_; ++i) {
    int index = i * kNumFloatsPerQuad;
    
    data_[index + 2] = 0.0f;
    data_[index + 3] = 1.0f;
    data_[index + 6] = 0.0f;
    data_[index + 7] = 0.0f;
    data_[index + 10] = 1.0f;
    data_[index + 11] = 0.0f;
    data_[index + 14] = 1.0f;
    data_[index + 15] = 1.0f;
    
    for (int j = 0; j < kNumIndicesPerQuad; ++j)
      indices_[i * kNumIndicesPerQuad + j] = triangles[j] + i * kNumVertices;
  }

  setInterceptsMouseClicks(false, false);
  setColor(Colours::white);
}

OpenGlMultiImage::~OpenGlMultiImage() {}

void OpenGlMultiImage::init(OpenGlWrapper& open_gl) {
  open_gl.context.extensions.glGenBuffers(1, &vertex_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(max_quads_ * kNumFloatsPerQuad * sizeof(float));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, data_.get(), GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &indices_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

  GLsizeiptr tri_size = static_cast<GLsizeiptr>(max_quads_ * kNumIndicesPerQuad * sizeof(int));
  open_gl.context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_size, indices_.get(), GL_STATIC_DRAW);

  image_shader_ = open_gl.shaders->getShaderProgram(Shaders::kImageVertex, Shaders::kTintedImageFragment);
  image_shader_->use();
  color_uniform_ = getUniform(open_gl, *image_shader_, "color");
  position_ = getAttribute(open_gl, *image_shader_, "position");
  texture_coordinates_ = getAttribute(open_gl, *image_shader_, "tex_coord_in");
}

void OpenGlMultiImage::destroy(OpenGlWrapper& open_gl) {
  texture_.release();

  image_shader_ = nullptr;
  position_ = nullptr;
  texture_coordinates_ = nullptr;
  color_uniform_ = nullptr;

  open_gl.context.extensions.glDeleteBuffers(1, &vertex_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &indices_buffer_);

  vertex_buffer_ = 0;
  indices_buffer_ = 0;
}

void OpenGlMultiImage::render(OpenGlWrapper& open_gl, bool animate) {
  if (!setViewPort(open_gl))
    return;

  if (image_) {
    texture_.loadImage(*image_);
    image_ = nullptr;
  }

  if (image_shader_ == nullptr)
    init(open_gl);

  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);
  if (additive_blending_)
    glBlendFunc(GL_ONE, GL_ONE);
  else
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  texture_.bind();
  open_gl.context.extensions.glActiveTexture(GL_TEXTURE0);

  if (dirty_) {
    dirty_ = false;
    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

    GLsizeiptr vert_size = static_cast<GLsizeiptr>(kNumFloatsPerQuad * max_quads_ * sizeof(float));
    open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, data_.get(), GL_STATIC_DRAW);
    open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  image_shader_->use();
  color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(),
                      color_.getFloatBlue(), color_.getFloatAlpha());

  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

  open_gl.context.extensions.glVertexAttribPointer(position_->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, kNumFloatsPerVertex * sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(position_->attributeID);
  open_gl.context.extensions.glVertexAttribPointer(texture_coordinates_->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, kNumFloatsPerVertex * sizeof(float),
                                                   (GLvoid*)(2 * sizeof(float)));
  open_gl.context.extensions.glEnableVertexAttribArray(texture_coordinates_->attributeID);

  glDrawElements(GL_TRIANGLES, num_quads_ * kNumIndicesPerQuad, GL_UNSIGNED_INT, nullptr);

  open_gl.context.extensions.glDisableVertexAttribArray(position_->attributeID);
  open_gl.context.extensions.glDisableVertexAttribArray(texture_coordinates_->attributeID);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
