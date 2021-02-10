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

class OpenGlImage {
  public:
    OpenGlImage();
    virtual ~OpenGlImage();

    void init(OpenGlWrapper& open_gl);
    void drawImage(OpenGlWrapper& open_gl);
    void destroy(OpenGlWrapper& open_gl);

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

    void setColor(Colour color) { color_ = color; }

    inline void setPosition(float x, float y, int index) {
      position_vertices_[index] = x;
      position_vertices_[index + 1] = y;
      dirty_ = true;
    }
    inline void setTopLeft(float x, float y) { setPosition(x, y, 0); }
    inline void setBottomLeft(float x, float y) { setPosition(x, y, 4); }
    inline void setBottomRight(float x, float y) { setPosition(x, y, 8); }
    inline void setTopRight(float x, float y) { setPosition(x, y, 12); }

    int getImageWidth() { return image_width_; }
    int getImageHeight() { return image_height_; }

    void setAdditive(bool additive) { additive_ = additive; }
    void setUseAlpha(bool use_alpha) { use_alpha_ = use_alpha; }
    void setScissor(bool scissor) { scissor_ = scissor; }

  private:
    std::mutex mutex_;
    bool dirty_;

    Image* image_;
    int image_width_;
    int image_height_;
    std::unique_ptr<Image> owned_image_;
    Colour color_;
    OpenGLTexture texture_;
    bool additive_;
    bool use_alpha_;
    bool scissor_;

    OpenGLShaderProgram* image_shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> image_color_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> image_position_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> texture_coordinates_;

    std::unique_ptr<float[]> position_vertices_;
    std::unique_ptr<int[]> position_triangles_;
    GLuint vertex_buffer_;
    GLuint triangle_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlImage)
};

