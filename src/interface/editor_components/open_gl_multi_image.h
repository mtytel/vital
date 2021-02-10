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

#include "open_gl_component.h"

#include <mutex>

class OpenGlMultiImage : public OpenGlComponent {
  public:
    static constexpr int kNumVertices = 4;
    static constexpr int kNumFloatsPerVertex = 4;
    static constexpr int kNumFloatsPerQuad = kNumVertices * kNumFloatsPerVertex;
    static constexpr int kNumIndicesPerQuad = 6;

    OpenGlMultiImage(int max_images);
    virtual ~OpenGlMultiImage();

    virtual void init(OpenGlWrapper& open_gl) override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    virtual void destroy(OpenGlWrapper& open_gl) override;

    void paintBackground(Graphics& g) override { }

    void resized() override {
      OpenGlComponent::resized();
      dirty_ = true;
    }

    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

    void setOwnImage(Image& image) {
      mutex_.lock();
      owned_image_ = std::make_unique<Image>(image);
      setImage(owned_image_.get());
      mutex_.unlock();
    }

    void setImage(Image* image) {
      image_ = image;
      image_width_ = image->getWidth();
      image_height_ = image->getHeight();
    }

    void setNumQuads(int num_quads) { num_quads_ = num_quads; }

    void setColor(Colour color) { color_ = color; }

    inline void setQuad(int i, float x, float y, float w, float h) {
      int index = kNumFloatsPerQuad * i;
      data_[index] = x;
      data_[index + 1] = y;
      data_[index + 4] = x;
      data_[index + 5] = y + h;
      data_[index + 8] = x + w;
      data_[index + 9] = y + h;
      data_[index + 12] = x + w;
      data_[index + 13] = y;
      dirty_ = true;
    }

    int getImageWidth() { return image_width_; }
    int getImageHeight() { return image_height_; }

    void setAdditive(bool additive) { additive_blending_ = additive; }

  private:
    std::mutex mutex_;
    Image* image_;
    int image_width_;
    int image_height_;
    std::unique_ptr<Image> owned_image_;
    Colour color_;
    OpenGLTexture texture_;

    int max_quads_;
    int num_quads_;

    bool dirty_;
    bool additive_blending_;

    std::unique_ptr<float[]> data_;
    std::unique_ptr<int[]> indices_;

    OpenGLShaderProgram* image_shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> color_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> texture_coordinates_;

    GLuint vertex_buffer_;
    GLuint indices_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlMultiImage)
};

