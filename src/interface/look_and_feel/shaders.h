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

#include "common.h"
#include <map>

class Shaders {
  public:
    enum VertexShader {
      kImageVertex,
      kPassthroughVertex,
      kScaleVertex,
      kRotaryModulationVertex,
      kLinearModulationVertex,
      kGainMeterVertex,
      kAnalogFilterResponseVertex,
      kCombFilterResponseVertex,
      kPositiveFlangeFilterResponseVertex,
      kNegativeFlangeFilterResponseVertex,
      kDigitalFilterResponseVertex,
      kDiodeFilterResponseVertex,
      kDirtyFilterResponseVertex,
      kFormantFilterResponseVertex,
      kLadderFilterResponseVertex,
      kPhaserFilterResponseVertex,
      kEqFilterResponseVertex,
      kLineVertex,
      kFillVertex,
      kBarHorizontalVertex,
      kBarVerticalVertex,
      kNumVertexShaders
    };

    enum FragmentShader {
      kImageFragment,
      kTintedImageFragment,
      kGainMeterFragment,
      kFilterResponseFragment,
      kColorFragment,
      kFadeSquareFragment,
      kCircleFragment,
      kRingFragment,
      kDiamondFragment,
      kRoundedCornerFragment,
      kRoundedRectangleFragment,
      kRoundedRectangleBorderFragment,
      kRotarySliderFragment,
      kRotaryModulationFragment,
      kHorizontalSliderFragment,
      kVerticalSliderFragment,
      kLinearModulationFragment,
      kModulationKnobFragment,
      kLineFragment,
      kFillFragment,
      kBarFragment,
      kNumFragmentShaders
    };

    Shaders(OpenGLContext& open_gl_context);

    GLuint getVertexShaderId(VertexShader shader) {
      if (vertex_shader_ids_[shader] == 0)
        vertex_shader_ids_[shader] = createVertexShader(open_gl_context_->extensions, shader);
      return vertex_shader_ids_[shader];
    }

    GLuint getFragmentShaderId(FragmentShader shader) {
      if (fragment_shader_ids_[shader] == 0)
        fragment_shader_ids_[shader] = createFragmentShader(open_gl_context_->extensions, shader);
      return fragment_shader_ids_[shader];
    }

    OpenGLShaderProgram* getShaderProgram(VertexShader vertex_shader, FragmentShader fragment_shader,
                                          const GLchar** varyings = nullptr);

  private:
    static const char* getVertexShader(VertexShader shader);
    static const char* getFragmentShader(FragmentShader shader);

    bool checkShaderCorrect(OpenGLExtensionFunctions& extensions, GLuint shader_id) const;
    GLuint createVertexShader(OpenGLExtensionFunctions& extensions, VertexShader shader) const;
    GLuint createFragmentShader(OpenGLExtensionFunctions& extensions, FragmentShader shader) const;

    OpenGLContext* open_gl_context_;
    GLuint vertex_shader_ids_[kNumVertexShaders];
    GLuint fragment_shader_ids_[kNumFragmentShaders];

    std::map<int, std::unique_ptr<OpenGLShaderProgram>> shader_programs_;
};

struct OpenGlWrapper {
  OpenGlWrapper(OpenGLContext& c) : context(c), shaders(nullptr), display_scale(1.0f) { }

  OpenGLContext& context;
  Shaders* shaders;
  float display_scale;
};