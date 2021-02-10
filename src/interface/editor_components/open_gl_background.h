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

class OpenGlBackground {
  public:
    OpenGlBackground();
    virtual ~OpenGlBackground();

    void updateBackgroundImage(Image background);
    virtual void init(OpenGlWrapper& open_gl);
    virtual void render(OpenGlWrapper& open_gl);
    virtual void destroy(OpenGlWrapper& open_gl);

    void lock() { mutex_.lock(); }
    void unlock() { mutex_.unlock(); }

    OpenGLShaderProgram* shader() { return image_shader_; }
    OpenGLShaderProgram::Uniform* texture_uniform() { return texture_uniform_.get(); }

    void bind(OpenGLContext& open_gl_context);
    void enableAttributes(OpenGLContext& open_gl_context);
    void disableAttributes(OpenGLContext& open_gl_context);

  private:
    OpenGLShaderProgram* image_shader_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> texture_uniform_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> texture_coordinates_;

    float vertices_[16];

    std::mutex mutex_;
    OpenGLTexture background_;
    bool new_background_;
    Image background_image_;

    GLuint vertex_buffer_;
    GLuint triangle_buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlBackground)
};

