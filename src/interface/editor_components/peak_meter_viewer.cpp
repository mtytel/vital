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

#include "peak_meter_viewer.h"

#include "skin.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include "utils.h"

namespace {
  constexpr float kClampDecay = 0.014f;
  constexpr float kMinDb = -80.0f;
  constexpr float kMaxDb = 6.0f;
} // namespace;

PeakMeterViewer::PeakMeterViewer(bool left) : shader_(nullptr), clamped_(0.0f), left_(left) {
  addRoundedCorners();
  peak_output_ = nullptr;
  peak_memory_output_ = nullptr;
  float position_vertices[kNumPositions] = {
    -1.0f, 1.0f,
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f, 1.0f,
  };
  memcpy(position_vertices_, position_vertices, kNumPositions * sizeof(float));

  int position_triangles[6] = {
    0, 1, 2,
    2, 3, 0
  };
  memcpy(position_triangles_, position_triangles, kNumTriangleIndices * sizeof(int));

  vertex_buffer_ = 0;
  triangle_buffer_ = 0;
}

PeakMeterViewer::~PeakMeterViewer() { }

void PeakMeterViewer::resized() {
  if (peak_output_ == nullptr || peak_memory_output_ == nullptr) {
    SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
    peak_output_ = parent->getSynth()->getStatusOutput("peak_meter");
    peak_memory_output_ = parent->getSynth()->getStatusOutput("peak_meter_memory");
  }

  OpenGlComponent::resized();
}

void PeakMeterViewer::init(OpenGlWrapper& open_gl) {
  OpenGlComponent::init(open_gl);

  open_gl.context.extensions.glGenBuffers(1, &vertex_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);

  GLsizeiptr vert_size = static_cast<GLsizeiptr>(static_cast<size_t>(kNumPositions * sizeof(float)));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size,
                                         position_vertices_, GL_STATIC_DRAW);

  open_gl.context.extensions.glGenBuffers(1, &triangle_buffer_);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle_buffer_);

  GLsizeiptr tri_size = static_cast<GLsizeiptr>(static_cast<size_t>(kNumTriangleIndices * sizeof(float)));
  open_gl.context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, tri_size,
                                         position_triangles_, GL_STATIC_DRAW);

  shader_ = open_gl.shaders->getShaderProgram(Shaders::kGainMeterVertex, Shaders::kGainMeterFragment);
  shader_->use();
  position_ = getAttribute(open_gl, *shader_, "position");
  color_from_ = getUniform(open_gl, *shader_, "color_from");
  color_to_ = getUniform(open_gl, *shader_, "color_to");
}

void PeakMeterViewer::updateVertices() {
  if (peak_output_ == nullptr)
    return;

  float val = peak_output_->value()[left_ ? 0 : 1];
  if (val > 1.0f)
    clamped_ = 1.0f;

  float db = vital::utils::magnitudeToDb(val);
  float t = std::max((db - kMinDb) / (kMaxDb - kMinDb), 0.0f);
  t *= t;
  float position = vital::utils::interpolate(-1.0f, 1.0f, t);
  position_vertices_[0] = -1.0f;
  position_vertices_[2] = -1.0f;
  position_vertices_[4] = position;
  position_vertices_[6] = position;
}

void PeakMeterViewer::updateVerticesMemory() {
  if (peak_memory_output_ == nullptr)
    return;

  float val = peak_memory_output_->value()[left_ ? 0 : 1];
  if (val > 1.0f)
    clamped_ = 1.0f;

  float db = vital::utils::magnitudeToDb(val);
  float t = std::max((db - kMinDb) / (kMaxDb - kMinDb), 0.0f);
  t *= t;
  float position = vital::utils::interpolate(-1.0f, 1.0f, t);
  float width = 2.0f / getWidth();
  position_vertices_[0] = position - width;
  position_vertices_[2] = position - width;
  position_vertices_[4] = position;
  position_vertices_[6] = position;
}

void PeakMeterViewer::render(OpenGlWrapper& open_gl, bool animate) {
  if (!animate || peak_output_ == nullptr)
    return;

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  
  setViewPort(open_gl);
  shader_->use();

  Colour color_from, color_to;
  if (clamped_ > 0.0f) {
    color_from = findColour(Skin::kWidgetAccent1, true);
    color_to = findColour(Skin::kWidgetAccent2, true);
  }
  else {
    color_from = findColour(Skin::kWidgetSecondary1, true);
    color_to = findColour(Skin::kWidgetSecondary2, true);
  }

  color_from_->set(color_from.getFloatRed(), color_from.getFloatGreen(),
                   color_from.getFloatBlue(), color_from.getFloatAlpha());
  color_to_->set(color_to.getFloatRed(), color_to.getFloatGreen(),
                 color_to.getFloatBlue(), color_to.getFloatAlpha());

  updateVertices();
  draw(open_gl);
  updateVerticesMemory();
  draw(open_gl);

  clamped_ = vital::utils::max(clamped_ - kClampDecay, 0.0f);

  float rounding = std::min(getHeight() / 3.0f, findValue(Skin::kWidgetRoundedCorner) * 0.5f);
  renderCorners(open_gl, animate, findColour(Skin::kBackground, true), rounding);
}

void PeakMeterViewer::draw(OpenGlWrapper& open_gl) {
  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  GLsizeiptr vert_size = static_cast<GLsizeiptr>(static_cast<size_t>(kNumPositions * sizeof(float)));
  open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size,
                                          position_vertices_, GL_STATIC_DRAW);

  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangle_buffer_);

  open_gl.context.extensions.glVertexAttribPointer(position_->attributeID, 2, GL_FLOAT,
                                                   GL_FALSE, 2 * sizeof(float), nullptr);
  open_gl.context.extensions.glEnableVertexAttribArray(position_->attributeID);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

  open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
  open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void PeakMeterViewer::destroy(OpenGlWrapper& open_gl) {
  OpenGlComponent::destroy(open_gl);

  shader_ = nullptr;
  position_ = nullptr;
  color_from_ = nullptr;
  color_to_ = nullptr;
  open_gl.context.extensions.glDeleteBuffers(1, &vertex_buffer_);
  open_gl.context.extensions.glDeleteBuffers(1, &triangle_buffer_);
}

void PeakMeterViewer::paintBackground(Graphics& g) {
  float t = -kMinDb / (kMaxDb - kMinDb);
  t *= t;
  float x = getWidth() * t;
  Colour background = findColour(Skin::kWidgetBackground, true);
  g.setColour(background.interpolatedWith(findColour(Skin::kBackground, true), 0.5f));
  g.fillRect(x, 0.0f, getWidth() - x, 1.0f * getHeight());

  g.setColour(background);
  g.fillRect(0.0f, 0.0f, x, 1.0f * getHeight());

  g.setColour(findColour(Skin::kLightenScreen, true));
  g.fillRect((int)x, 0, 1, getHeight());
}
