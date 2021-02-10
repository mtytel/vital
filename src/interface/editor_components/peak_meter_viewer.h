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
#include "synth_module.h"

class PeakMeterViewer : public OpenGlComponent {
  public:
    PeakMeterViewer(bool left);
    virtual ~PeakMeterViewer();

    void resized() override;

    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void draw(OpenGlWrapper& open_gl);
    void destroy(OpenGlWrapper& open_gl) override;
    void paintBackground(Graphics& g) override;

  private:
    static constexpr int kNumPositions = 8;
    static constexpr int kNumTriangleIndices = 6;
    void updateVertices();
    void updateVerticesMemory();

    const vital::StatusOutput* peak_output_;
    const vital::StatusOutput* peak_memory_output_;

    OpenGLShaderProgram* shader_;
    std::unique_ptr<OpenGLShaderProgram::Attribute> position_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> color_from_;
    std::unique_ptr<OpenGLShaderProgram::Uniform> color_to_;

    float clamped_;
    float position_vertices_[kNumPositions];
    int position_triangles_[kNumTriangleIndices];
    GLuint vertex_buffer_;
    GLuint triangle_buffer_;
    bool left_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakMeterViewer)
};

