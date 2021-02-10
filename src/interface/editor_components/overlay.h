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
#include "synth_section.h"

#include <set>

class OverlayBackgroundRenderer : public OpenGlComponent {
  public:
    static constexpr int kNumVertices = 4;
    static constexpr int kNumFloatsPerVertex = 2;
    static constexpr int kTotalFloats = kNumVertices * kNumFloatsPerVertex;
    static constexpr int kIndices = 6;

    OverlayBackgroundRenderer() {
      shader_ = nullptr;
      additive_blending_ = false;

      color_ = Colours::black;
      data_[0] = -1.0f;
      data_[1] = -1.0f;
      data_[2] = -1.0f;
      data_[3] = 1.0f;
      data_[4] = 1.0f;
      data_[5] = -1.0f;
      data_[6] = 1.0f;
      data_[7] = 1.0f;

      indices_[0] = 0;
      indices_[1] = 1;
      indices_[2] = 2;
      indices_[3] = 1;
      indices_[4] = 2;
      indices_[5] = 3;

      setInterceptsMouseClicks(false, false);
    }

    virtual ~OverlayBackgroundRenderer() { }

    virtual void init(OpenGlWrapper& open_gl) override {
      open_gl.context.extensions.glGenBuffers(1, &data_buffer_);
      open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, data_buffer_);

      GLsizeiptr vert_size = static_cast<GLsizeiptr>(kTotalFloats * sizeof(float));
      open_gl.context.extensions.glBufferData(GL_ARRAY_BUFFER, vert_size, data_, GL_STATIC_DRAW);

      open_gl.context.extensions.glGenBuffers(1, &indices_buffer_);
      open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

      GLsizeiptr bar_size = static_cast<GLsizeiptr>(kIndices * sizeof(int));
      open_gl.context.extensions.glBufferData(GL_ELEMENT_ARRAY_BUFFER, bar_size, indices_, GL_STATIC_DRAW);

      shader_ = open_gl.shaders->getShaderProgram(Shaders::kPassthroughVertex, Shaders::kColorFragment);
      shader_->use();
      color_uniform_ = getUniform(open_gl, *shader_, "color");
      position_ = getAttribute(open_gl, *shader_, "position");
    }

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      drawOverlay(open_gl);
    }

    void paintBackground(Graphics& g) override { }

    virtual void destroy(OpenGlWrapper& open_gl) override {
      shader_ = nullptr;
      position_ = nullptr;
      color_uniform_ = nullptr;
      open_gl.context.extensions.glDeleteBuffers(1, &data_buffer_);
      open_gl.context.extensions.glDeleteBuffers(1, &indices_buffer_);

      data_buffer_ = 0;
      indices_buffer_ = 0;
    }

    void setColor(const Colour& color) { color_ = color; }

    force_inline void setAdditiveBlending(bool additive_blending) { additive_blending_ = additive_blending; }

  protected:
    void drawOverlay(OpenGlWrapper& open_gl) {
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

      shader_->use();
      color_uniform_->set(color_.getFloatRed(), color_.getFloatGreen(),
                          color_.getFloatBlue(), color_.getFloatAlpha());

      open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, data_buffer_);
      open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer_);

      open_gl.context.extensions.glVertexAttribPointer(position_->attributeID, kNumFloatsPerVertex, GL_FLOAT,
                                                       GL_FALSE, kNumFloatsPerVertex * sizeof(float), nullptr);
      open_gl.context.extensions.glEnableVertexAttribArray(position_->attributeID);

      glDrawElements(GL_TRIANGLES, kIndices, GL_UNSIGNED_INT, nullptr);

      open_gl.context.extensions.glDisableVertexAttribArray(position_->attributeID);
      open_gl.context.extensions.glBindBuffer(GL_ARRAY_BUFFER, 0);
      open_gl.context.extensions.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
      
      glDisable(GL_BLEND);
      glDisable(GL_SCISSOR_TEST);
    }

    OpenGLShaderProgram* shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> color_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;

    Colour color_;
    bool additive_blending_;

    float data_[kTotalFloats];
    int indices_[kIndices];
    GLuint data_buffer_;
    GLuint indices_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlayBackgroundRenderer)
};

class Overlay : public SynthSection {
  public:
    class Listener {
      public:
        Listener() { }
        virtual ~Listener() { }

        virtual void overlayShown(Overlay* component) = 0;
        virtual void overlayHidden(Overlay* component) = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Listener)
    };

    Overlay(String name) : SynthSection(name), size_ratio_(1.0f) {
      setSkinOverride(Skin::kOverlay);
      addOpenGlComponent(&background_);
    }
    virtual ~Overlay() { }

    void setVisible(bool should_be_visible) override {
      for (Listener* listener : listeners_) {
        if (should_be_visible)
          listener->overlayShown(this);
        else
          listener->overlayHidden(this);
      }
      Component::setVisible(should_be_visible);
    }

    virtual void resized() override {
      background_.setColor(findColour(Skin::kOverlayScreen, true));
      background_.setBounds(getLocalBounds());
    }

    virtual void paintBackground(Graphics& g) override { paintOpenGlChildrenBackgrounds(g); }

    void addOverlayListener(Listener* listener) { listeners_.insert(listener); }
    void removeOverlayListener(Listener* listener) { listeners_.erase(listener); }
    void setSizeRatio(float ratio) override { size_ratio_ = ratio; }

  protected:
    float size_ratio_;
    std::set<Listener*> listeners_;
    OverlayBackgroundRenderer background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Overlay)
};

