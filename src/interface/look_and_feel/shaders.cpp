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

#include "shaders.h"

#include "JuceHeader.h"

#include "open_gl_component.h"
#include "synth_constants.h"

#if JUCE_OPENGL_ES || OPENGL_ES
  #define HIGHP "highp"
  #define MEDIUMP "mediump"
  #define LOWP "lowp"
#else
  #define HIGHP
  #define MEDIUMP
  #define LOWP
#endif

#define FILTER_RESPONSE_UNIFORMS \
    "uniform " MEDIUMP " float midi_cutoff;\n" \
    "uniform " MEDIUMP " float resonance;\n" \
    "uniform " MEDIUMP " float drive;\n" \
    "uniform " MEDIUMP " float mix;\n" \
    "uniform " MEDIUMP " float db24;\n" \
    "uniform " MEDIUMP " float stage0;\n" \
    "uniform " MEDIUMP " float stage1;\n" \
    "uniform " MEDIUMP " float stage2;\n" \
    "uniform " MEDIUMP " float stage3;\n" \
    "uniform " MEDIUMP " float stage4;\n"

#define FILTER_RESPONSE_CONSTANTS \
    "const " MEDIUMP " float kMinMidiNote = 8.0;\n" \
    "const " MEDIUMP " float kPi = 3.14159265359;\n" \
    "const " MEDIUMP " float kMaxMidiNote = 137.0;\n" \
    "const " MEDIUMP " float kMidi0Frequency = 8.1757989156;\n" \
    "const " MEDIUMP " float kMinDb = -30.0;\n" \
    "const " MEDIUMP " float kMaxDb = 20.0;\n" \

#define RESPONSE_TOOLS \
    "vec2 complexMultiply(vec2 a, vec2 b) {\n" \
    "    return vec2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);\n" \
    "}\n" \
    "vec2 complexReciprocal(vec2 num) {\n" \
    "    " MEDIUMP " vec2 conjugate = vec2(num.x, -num.y);\n" \
    "    " MEDIUMP " vec2 denominator = complexMultiply(num, conjugate);\n" \
    "    return vec2(conjugate.x / denominator.x, conjugate.y / denominator.x);\n" \
    "}\n" \
    "vec2 complexDivide(vec2 a, vec2 b) {\n" \
    "    " MEDIUMP " vec2 conjugate = vec2(b.x, -b.y);\n" \
    "    " MEDIUMP " vec2 num = complexMultiply(a, conjugate);\n" \
    "    " MEDIUMP " vec2 den = complexMultiply(b, conjugate);\n" \
    "    return vec2(num.x / den.x, num.y / den.x);\n" \
    "}\n" \
    "vec2 onePoleInvertResponse(float cutoff) {\n" \
    "    return vec2(1.0, cutoff);\n" \
    "}\n" \
    "vec2 onePoleResponse(float cutoff) {\n" \
    "    return complexReciprocal(vec2(1.0, cutoff));\n" \
    "}\n" \
    "float magnitudeToDb(float magnitude) {\n" \
    "    return 8.685889638065037 * log(magnitude);\n" \
    "}\n" \
    "float getCutoffRatio(float x, float midi_cutoff) {\n" \
    "    " MEDIUMP " float percent = 0.5 * (x + 1.0);\n" \
    "    " MEDIUMP " float midi_note = kMinMidiNote + percent * (kMaxMidiNote - kMinMidiNote);\n" \
    "    return pow(2.0, min((midi_note - midi_cutoff) / 12.0, 8.0));\n" \
    "}\n" \
    "float getFrequencyForX(float x) {\n" \
    "    " MEDIUMP " float percent = 0.5 * (x + 1.0);\n" \
    "    " MEDIUMP " float midi_note = kMinMidiNote + percent * (kMaxMidiNote - kMinMidiNote);\n" \
    "    return kMidi0Frequency * pow(2.0, midi_note / 12.0);\n" \
    "}\n" \
    "float getYForResponse(vec2 response) {\n" \
    "    " MEDIUMP " float magnitude_response = length(response);\n" \
    "    " MEDIUMP " float db = magnitudeToDb(magnitude_response);\n" \
    "    return 2.0 * (db - kMinDb) / (kMaxDb - kMinDb) - 1.0;\n" \
    "}\n" \
    "vec4 computePosition(vec4 start_position, vec2 response) {\n" \
    "    " MEDIUMP " vec4 result = start_position;\n" \
    "    result.y = getYForResponse(response);\n" \
    "    return result;\n" \
    "}\n"

namespace {
  const char* kImageVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "attribute " MEDIUMP " vec2 tex_coord_in;\n"
      "\n"
      "varying " MEDIUMP " vec2 tex_coord_out;\n"
      "\n"
      "void main() {\n"
      "    tex_coord_out = tex_coord_in;\n"
      "    gl_Position = vec4(position.xy, 1.0, 1.0);\n"
      "}\n";

  const char* kImageFragmentShader =
      "varying " MEDIUMP " vec2 tex_coord_out;\n"
      "\n"
      "uniform sampler2D image;\n"
      "\n"
      "void main() {\n"
      "    gl_FragColor = texture2D(image, tex_coord_out);\n"
      "}\n";

  const char* kTintedImageFragmentShader =
      "varying " MEDIUMP " vec2 tex_coord_out;\n"
      "\n"
      "uniform sampler2D image;\n"
      "uniform " MEDIUMP " vec4 color;\n"
      "\n"
      "void main() {\n"
      "    " MEDIUMP " vec4 image_color = texture2D(image, tex_coord_out);\n"
      "    image_color.r *= color.r;\n"
      "    image_color.g *= color.g;\n"
      "    image_color.b *= color.b;\n"
      "    image_color.a *= color.a;\n"
      "    gl_FragColor = image_color;\n"
    "}\n";

  const char* kPassthroughVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "attribute " MEDIUMP " vec2 dimensions;\n"
      "attribute " MEDIUMP " vec2 coordinates;\n"
      "attribute " MEDIUMP " vec4 shader_values;\n"
      "\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "\n"
      "void main() {\n"
      "    dimensions_out = dimensions;\n"
      "    coordinates_out = coordinates;\n"
      "    shader_values_out = shader_values;\n"
      "    gl_Position = position;\n"
      "}\n";

  const char* kScaleVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "uniform " MEDIUMP " vec2 scale;\n"
      "\n"
      "void main() {\n"
      "    gl_Position = position;\n"
      "    gl_Position.x = gl_Position.x * scale.x;\n"
      "    gl_Position.y = gl_Position.y * scale.y;\n"
      "    gl_Position.z = 0.0;\n"
      "    gl_Position.a = 1.0;\n"
      "}\n";

  const char* kRotaryModulationVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "attribute " MEDIUMP " vec2 coordinates;\n"
      "attribute " MEDIUMP " vec4 range;\n"
      "attribute " MEDIUMP " float meter_radius;\n"
      "\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "varying " MEDIUMP " vec4 range_out;\n"
      "varying " MEDIUMP " float meter_radius_out;\n"
      "\n"
      "void main() {\n"
      "    coordinates_out = coordinates;\n"
      "    range_out = range;\n"
      "    meter_radius_out = meter_radius;\n"
      "    gl_Position = position;\n"
      "}\n";

  const char* kLinearModulationVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "attribute " MEDIUMP " vec2 coordinates;\n"
      "attribute " MEDIUMP " vec4 range;\n"
      "\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "varying " MEDIUMP " vec4 range_out;\n"
      "\n"
      "void main() {\n"
      "    coordinates_out = coordinates;\n"
      "    range_out = range;\n"
      "    gl_Position = position;\n"
      "}\n";

  const char* kGainMeterVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "\n"
      "varying " MEDIUMP " vec2 position_out;\n"
      "\n"
      "void main() {\n"
      "    gl_Position = position;\n"
      "    position_out = position.xz;\n"
      "}\n";

  const char* kGainMeterFragmentShader =
      "varying " MEDIUMP " vec2 position_out;\n"
      "uniform " MEDIUMP " vec4 color_from;\n"
      "uniform " MEDIUMP " vec4 color_to;\n"
      "void main() {\n"
      "    " MEDIUMP " float t = (position_out.x + 1.0) / 2.0;\n"
      "    gl_FragColor = color_to * t + color_from * (1.0 - t);\n"
      "}\n";

  const char* kAnalogFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " vec2 one_pole = onePoleResponse(getCutoffRatio(position.x, midi_cutoff));\n"
      "    " MEDIUMP " vec2 low = complexMultiply(one_pole, one_pole);\n"
      "    " MEDIUMP " vec2 band = one_pole - low;\n"
      "    " MEDIUMP " vec2 high = vec2(1.0, 0.0) - one_pole - band;\n"
      "    " MEDIUMP " vec2 two_stage_pre = stage3 * low + stage1 * band + stage4 * high;\n"
      "    " MEDIUMP " vec2 two_stage = stage0 * low + stage1 * band + stage2 * high;\n"
      "    " MEDIUMP " vec2 feedback = complexMultiply(one_pole, vec2(1.0, 0.0) - one_pole);\n"
      "    " MEDIUMP " vec2 denominator_pre = vec2(1.0, 0.0) - feedback;\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0, 0.0) - resonance * feedback;\n"
      "    " MEDIUMP " vec2 response_pre = complexDivide(two_stage_pre, denominator_pre);\n"
      "    " MEDIUMP " vec2 response_res = complexDivide(two_stage, denominator);\n"
      "    " MEDIUMP " vec2 response = drive * response_res;\n"
      "    response = response + db24 * (complexMultiply(response_pre, response) - response);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kCombFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "const " MEDIUMP " float kMaxCycles = 6.0;\n"
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float ratio = getCutoffRatio(position.x, midi_cutoff);\n"
      "    " MEDIUMP " float ratio_diff = getCutoffRatio(position.x + 0.02, midi_cutoff) - ratio;\n"
      "    " MEDIUMP " float max_step = step(kMaxCycles, ratio);\n"
      "    " MEDIUMP " vec2 tick = resonance * vec2(cos(2.0 * kPi * ratio), -sin(2.0 * kPi * ratio));\n"
      "    " MEDIUMP " vec2 low_pass = onePoleResponse(getCutoffRatio(position.x, stage2));\n"
      "    " MEDIUMP " vec2 high_pass = vec2(1.0, 0.0) - low_pass;\n"
      "    " MEDIUMP " vec2 one_pole = stage0 * low_pass + stage1 * high_pass;\n"
      "    " MEDIUMP " vec2 high_pass2 = vec2(1.0, 0.0) - onePoleResponse(getCutoffRatio(position.x, stage3));\n"
      "    " MEDIUMP " vec2 filter_input = vec2(1.0 - 0.5 * abs(resonance), 0.0);\n"
      "    " MEDIUMP " filter_input = complexMultiply(complexMultiply(filter_input, one_pole), high_pass2);\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0, 0.0) - complexMultiply(complexMultiply(tick, one_pole), high_pass2);\n"
      "    " MEDIUMP " float round_value = complexMultiply(one_pole, high_pass2).x * abs(resonance);\n"
      "    " MEDIUMP " vec2 denominator_round = vec2(1.0 - round_value, 0.0);\n"
      "    " MEDIUMP " vec2 denominator_round_down = vec2(1.0 + round_value, 0.0);\n"
      "    " MEDIUMP " float max_step_mult = 1.0 - position.y;\n"
      "    " MEDIUMP " vec2 max_step_denominator = max_step_mult * denominator_round + (1.0 - max_step_mult) * denominator_round_down;\n"
      "    denominator = max_step * max_step_denominator + (1.0 - max_step) * denominator;\n"
      "    " MEDIUMP " float denominator_round_step = 1.0 - max(max_step, step(ratio_diff, length(denominator)));\n"
      "    denominator = denominator_round_step * denominator_round + (1.0 - denominator_round_step) * denominator;\n"
      "    " MEDIUMP " vec2 response = complexDivide(drive * filter_input, denominator);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kPositiveFlangeFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "const " MEDIUMP " float kMaxCycles = 8.0;\n"
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float ratio = getCutoffRatio(position.x, midi_cutoff);\n"
      "    " MEDIUMP " float ratio_diff = getCutoffRatio(position.x + 0.02, midi_cutoff) - ratio;\n"
      "    " MEDIUMP " float max_step = step(kMaxCycles, ratio);\n"
      "    " MEDIUMP " vec2 delay = vec2(cos(2.0 * kPi * ratio), -sin(2.0 * kPi * ratio));\n"
      "    " MEDIUMP " vec2 tick = resonance * delay;\n"
      "    " MEDIUMP " vec2 low_pass = onePoleResponse(getCutoffRatio(position.x, stage2));\n"
      "    " MEDIUMP " vec2 high_pass = vec2(1.0, 0.0) - low_pass;\n"
      "    " MEDIUMP " vec2 one_pole = stage0 * low_pass + stage1 * high_pass;\n"
      "    " MEDIUMP " vec2 high_pass2 = vec2(1.0, 0.0) - onePoleResponse(getCutoffRatio(position.x, stage3));\n"
      "    " MEDIUMP " vec2 filter_input = vec2(0.70710678119, 0.0);\n"
      "    " MEDIUMP " vec2 delay_input = complexMultiply(complexMultiply(filter_input, one_pole), high_pass2);\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0, 0.0) - tick;\n"
      "    " MEDIUMP " vec2 round_value = complexMultiply(one_pole, high_pass2) * resonance;\n"
      "    " MEDIUMP " vec2 denominator_round = complexMultiply(delay, vec2(1.0, 0.0) - round_value);\n"
      "    " MEDIUMP " vec2 denominator_round_down = complexMultiply(-delay, vec2(1.0, 0.0) + round_value);\n"
      "    " MEDIUMP " float max_step_mult = 1.0 - position.y;\n"
      "    " MEDIUMP " vec2 max_step_denominator = max_step_mult * denominator_round + (1.0 - max_step_mult) * denominator_round_down;\n"
      "    denominator = max_step * max_step_denominator + (1.0 - max_step) * denominator;\n"
      "    " MEDIUMP " float denominator_round_step = 1.0 - max(max_step, step(ratio_diff, length(denominator)));\n"
      "    denominator = denominator_round_step * denominator_round + (1.0 - denominator_round_step) * denominator;\n"
      "    " MEDIUMP " vec2 response = filter_input * drive + complexMultiply(complexDivide(delay_input, denominator), delay);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kNegativeFlangeFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "const " MEDIUMP " float kMaxCycles = 8.0;\n"
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float ratio = getCutoffRatio(position.x, midi_cutoff + 12.0);\n"
      "    " MEDIUMP " float max_step = step(kMaxCycles, ratio);\n"
      "    " MEDIUMP " vec2 delay = vec2(cos(2.0 * kPi * ratio), -sin(2.0 * kPi * ratio));\n"
      "    " MEDIUMP " vec2 tick = -resonance * delay;\n"
      "    " MEDIUMP " vec2 low_pass = onePoleResponse(getCutoffRatio(position.x, stage2));\n"
      "    " MEDIUMP " vec2 high_pass = vec2(1.0, 0.0) - low_pass;\n"
      "    " MEDIUMP " vec2 one_pole = stage0 * low_pass + stage1 * high_pass;\n"
      "    " MEDIUMP " vec2 high_pass2 = vec2(1.0, 0.0) - onePoleResponse(getCutoffRatio(position.x, stage3));\n"
      "    " MEDIUMP " vec2 filter_input = vec2(0.70710678119, 0.0);\n"
      "    " MEDIUMP " vec2 delay_input = complexMultiply(complexMultiply(filter_input, one_pole), high_pass2);\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0, 0.0) - complexMultiply(tick, complexMultiply(one_pole, high_pass2));\n"
      "    " MEDIUMP " vec2 round_value = -complexMultiply(one_pole, high_pass2) * resonance;\n"
      "    " MEDIUMP " vec2 denominator_round = complexMultiply(delay, vec2(1.0, 0.0) - round_value);\n"
      "    " MEDIUMP " vec2 denominator_round_down = complexMultiply(-delay, vec2(1.0, 0.0) + round_value);\n"
      "    " MEDIUMP " float max_step_mult = 1.0 - position.y;\n"
      "    " MEDIUMP " vec2 max_step_denominator = max_step_mult * denominator_round + (1.0 - max_step_mult) * denominator_round_down;\n"
      "    denominator = max_step * max_step_denominator + (1.0 - max_step) * denominator;\n"
      "    " MEDIUMP " vec2 response = filter_input * drive - complexMultiply(complexDivide(delay_input, denominator), delay);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kDigitalFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float g = getCutoffRatio(position.x, midi_cutoff);\n"
      "    " MEDIUMP " vec2 g2 = vec2(g * g, 0.0);\n"
      "    " MEDIUMP " vec2 denominator = g2 + vec2(0.0, g * resonance) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator = -stage0 * vec2(1.0, 0.0) + stage1 * vec2(0.0, g) + stage2 * g2;\n"
      "    " MEDIUMP " vec2 numerator_pre = -stage3 * vec2(1.0, 0.0) + stage1 * vec2(0.0, g) + stage4 * g2;\n"
      "    " MEDIUMP " vec2 response = complexDivide(numerator, denominator);\n"
      "    " MEDIUMP " vec2 pre_denominator = g2 + vec2(0.0, g) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 pre_response = complexDivide(numerator_pre, pre_denominator);\n"
      "    response = response + db24 * (complexMultiply(response, pre_response) - response);\n"
      "    response *= drive;\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kDiodeFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float ratio = getCutoffRatio(position.x, midi_cutoff);\n"
      "    " MEDIUMP " vec2 one_pole = onePoleResponse(ratio);\n"
      "    " MEDIUMP " vec2 high_pass_one_pole = onePoleResponse(ratio / stage0);\n"
      "    " MEDIUMP " vec2 high = vec2(1.0, 0.0) - high_pass_one_pole * 2.0 + complexMultiply(high_pass_one_pole, high_pass_one_pole);\n"
      "    " MEDIUMP " vec2 high_feedback = complexMultiply(high_pass_one_pole, vec2(1.0, 0.0) - high_pass_one_pole);\n"
      "    " MEDIUMP " vec2 high_denominator = vec2(1.0, 0.0) - high_feedback;\n"
      "    " MEDIUMP " vec2 high_pass_response = complexDivide(high, high_denominator);\n"
      "    high_pass_response = vec2(1.0, 0.0) + db24 * (high_pass_response + vec2(-1.0, 0.0));\n"
      "    " MEDIUMP " vec2 loop = complexMultiply(one_pole, one_pole);\n"
      "    " MEDIUMP " vec2 series = 0.125 * complexMultiply(loop, loop);\n"
      "    " MEDIUMP " vec2 chain = complexDivide(series, vec2(1.0, 0.0) + series - loop);"
      "    " MEDIUMP " vec2 numerator = drive * chain;\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0, 0.0) + resonance * chain;\n"
      "    " MEDIUMP " vec2 response = complexDivide(numerator, denominator);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response = complexMultiply(response, high_pass_response);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kDirtyFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " vec2 one_pole = onePoleResponse(getCutoffRatio(position.x, midi_cutoff));\n"
      "    " MEDIUMP " vec2 low = complexMultiply(one_pole, one_pole);\n"
      "    " MEDIUMP " vec2 band = one_pole - low;\n"
      "    " MEDIUMP " vec2 high = vec2(1.0, 0.0) - one_pole - band;\n"
      "    " MEDIUMP " vec2 two_stage_pre = stage3 * low + stage1 * band + stage4 * high;\n"
      "    " MEDIUMP " vec2 two_stage = stage0 * low + stage1 * band + stage2 * high;\n"
      "    " MEDIUMP " vec2 feedback = complexMultiply(one_pole, vec2(1.0, 0.0) - one_pole);\n"
      "    " MEDIUMP " vec2 denominator_pre = vec2(1.0, 0.0) - feedback;\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0 / resonance, 0.0) - feedback;\n"
      "    " MEDIUMP " vec2 resonance_loop = complexDivide(band, denominator);\n"
      "    " MEDIUMP " vec2 response_pre = complexDivide(two_stage_pre, denominator_pre);\n"
      "    " MEDIUMP " vec2 response_res = complexMultiply(two_stage, vec2(1.0, 0.0) + resonance_loop);\n"
      "    " MEDIUMP " vec2 response = drive * response_res;\n"
      "    response = response + db24 * (complexMultiply(response_pre, response) - response);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kFormantFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "uniform " MEDIUMP " vec4 formant_cutoff;\n"
      "uniform " MEDIUMP " vec4 formant_resonance;\n"
      "uniform " MEDIUMP " vec4 low;\n"
      "uniform " MEDIUMP " vec4 band;\n"
      "uniform " MEDIUMP " vec4 high;\n"
      "uniform " MEDIUMP " float sample_rate;\n"
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float g0 = getCutoffRatio(position.x, formant_cutoff[0]);\n"
      "    " MEDIUMP " vec2 g0_sqr = vec2(g0 * g0, 0.0);\n"
      "    " MEDIUMP " vec2 denominator0 = g0_sqr + vec2(0.0, g0 * formant_resonance[0]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator0 = -low[0] * vec2(1.0, 0.0) + band[0] * vec2(0.0, g0) + high[0] * g0_sqr;\n"
      "    " MEDIUMP " vec2 response0 = complexDivide(numerator0, denominator0);\n"
      "    " MEDIUMP " float g1 = getCutoffRatio(position.x, formant_cutoff[1]);\n"
      "    " MEDIUMP " vec2 g1_sqr = vec2(g1 * g1, 0.0);\n"
      "    " MEDIUMP " vec2 denominator1 = g1_sqr + vec2(0.0, g1 * formant_resonance[1]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator1 = -low[1] * vec2(1.0, 0.0) + band[1] * vec2(0.0, g1) + high[1] * g1_sqr;\n"
      "    " MEDIUMP " vec2 response1 = complexDivide(numerator1, denominator1);\n"
      "    " MEDIUMP " float g2 = getCutoffRatio(position.x, formant_cutoff[2]);\n"
      "    " MEDIUMP " vec2 g2_sqr = vec2(g2 * g2, 0.0);\n"
      "    " MEDIUMP " vec2 denominator2 = g2_sqr + vec2(0.0, g2 * formant_resonance[2]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator2 = -low[2] * vec2(1.0, 0.0) + band[2] * vec2(0.0, g2) + high[2] * g2_sqr;\n"
      "    " MEDIUMP " vec2 response2 = complexDivide(numerator2, denominator2);\n"
      "    " MEDIUMP " float g3 = getCutoffRatio(position.x, formant_cutoff[3]);\n"
      "    " MEDIUMP " vec2 g3_sqr = vec2(g3 * g3, 0.0);\n"
      "    " MEDIUMP " vec2 denominator3 = g3_sqr + vec2(0.0, g3 * formant_resonance[3]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator3 = -low[3] * vec2(1.0, 0.0) + band[3] * vec2(0.0, g3) + high[3] * g3_sqr;\n"
      "    " MEDIUMP " vec2 response3 = complexDivide(numerator3, denominator3);\n"
      "    " MEDIUMP " vec2 response = response0 + response1 + response2 + response3;\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kLadderFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " vec2 one_pole_invert = onePoleInvertResponse(getCutoffRatio(position.x, midi_cutoff));\n"
      "    " MEDIUMP " vec2 two_pole_invert = complexMultiply(one_pole_invert, one_pole_invert);\n"
      "    " MEDIUMP " vec2 three_pole_invert = complexMultiply(one_pole_invert, two_pole_invert);\n"
      "    " MEDIUMP " vec2 four_pole_invert = complexMultiply(one_pole_invert, three_pole_invert);\n"
      "    " MEDIUMP " vec2 numerator = drive * (stage0 * four_pole_invert + stage1 * three_pole_invert + \n"
      "                              stage2 * two_pole_invert + stage3 * one_pole_invert + \n"
      "                              vec2(stage4, 0.0));\n"
      "    " MEDIUMP " vec2 denominator = four_pole_invert + vec2(resonance, 0.0);\n"
      "    " MEDIUMP " vec2 response = complexDivide(numerator, denominator);\n"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";
  
  const char* kPhaserFilterResponseVertexShader =
      "in " MEDIUMP " vec2 position;\n"
      "out " MEDIUMP " float response_out;\n"
      FILTER_RESPONSE_UNIFORMS
      FILTER_RESPONSE_CONSTANTS
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float g = getCutoffRatio(position.x, midi_cutoff);\n"
      "    " MEDIUMP " vec2 one_pole = onePoleResponse(g);\n"
      "    " MEDIUMP " vec2 all_pass = vec2(1.0, 0.0) - 2.0 * one_pole;\n"
      "    " MEDIUMP " vec2 half_peak = complexMultiply(all_pass, all_pass);\n"
      "    " MEDIUMP " vec2 peak1 = complexMultiply(half_peak, half_peak);\n"
      "    " MEDIUMP " vec2 peak3 = complexMultiply(peak1, peak1);\n"
      "    " MEDIUMP " vec2 peak5 = complexMultiply(peak3, peak1);\n"
      "    " MEDIUMP " vec2 chain = stage0 * peak1 + stage1 * peak3 + stage2 * peak5;\n"
      "    " MEDIUMP " float invert_mult = 1.0 - 2.0 * db24;\n"
      "    " MEDIUMP " vec2 feedback_chain = complexMultiply(chain, onePoleResponse(0.05 * g));\n"
      "    feedback_chain = complexMultiply(feedback_chain, vec2(1.0, 0.0) - onePoleResponse(20.0 * g));\n"
      "    " MEDIUMP " vec2 denominator = vec2(1.0, 0.0) - invert_mult * resonance * feedback_chain;\n"
      "    " MEDIUMP " vec2 phase_response = complexDivide(chain, denominator);\n"
      "    " MEDIUMP " vec2 response = vec2(0.5, 0.0) + 0.5 * invert_mult * phase_response;"
      "    response = mix * response + vec2(1.0 - mix, 0.0);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kEqFilterResponseVertexShader =
      "in " MEDIUMP " float position;\n"
      "out " MEDIUMP " float response_out;\n"
      "uniform " MEDIUMP " vec3 midi_cutoff;\n"
      "uniform " MEDIUMP " vec3 resonance;\n"
      "uniform " MEDIUMP " vec3 low_amount;\n"
      "uniform " MEDIUMP " vec3 band_amount;\n"
      "uniform " MEDIUMP " vec3 high_amount;\n"
      "const " MEDIUMP " float kMinMidiNote = 8.0;\n"
      "const " MEDIUMP " float kSampleRate = 100000.0;\n"
      "const " MEDIUMP " float kPi = 3.14159265359;\n"
      "const " MEDIUMP " float kMaxMidiNote = 136.0;\n"
      "const " MEDIUMP " float kMidi0Frequency = 8.1757989156;\n"
      "const " MEDIUMP " float kMinDb = -1.0;\n"
      "const " MEDIUMP " float kMaxDb = 1.0;\n"
      RESPONSE_TOOLS
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float g0 = getCutoffRatio(position, midi_cutoff[0]);\n"
      "    " MEDIUMP " vec2 g0_sqr = vec2(g0 * g0, 0.0);\n"
      "    " MEDIUMP " vec2 denominator0 = g0_sqr + vec2(0.0, g0 * resonance[0]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator0 = -low_amount[0] * vec2(1.0, 0.0) + band_amount[0] * vec2(0.0, g0) + high_amount[0] * g0_sqr;\n"
      "    " MEDIUMP " float g1 = getCutoffRatio(position, midi_cutoff[1]);\n"
      "    " MEDIUMP " vec2 g1_sqr = vec2(g1 * g1, 0.0);\n"
      "    " MEDIUMP " vec2 denominator1 = g1_sqr + vec2(0.0, g1 * resonance[1]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator1 = -low_amount[1] * vec2(1.0, 0.0) + band_amount[1] * vec2(0.0, g1) + high_amount[1] * g1_sqr;\n"
      "    " MEDIUMP " float g2 = getCutoffRatio(position, midi_cutoff[2]);\n"
      "    " MEDIUMP " vec2 g2_sqr = vec2(g2 * g2, 0.0);\n"
      "    " MEDIUMP " vec2 denominator2 = g2_sqr + vec2(0.0, g2 * resonance[2]) + vec2(-1.0, 0.0);\n"
      "    " MEDIUMP " vec2 numerator2 = -low_amount[2] * vec2(1.0, 0.0) + band_amount[2] * vec2(0.0, g2) + high_amount[2] * g2_sqr;\n"
      "    " MEDIUMP " vec2 numerator = complexMultiply(numerator0, complexMultiply(numerator1, numerator2));\n"
      "    " MEDIUMP " vec2 denominator = complexMultiply(denominator0, complexMultiply(denominator1, denominator2));\n"
      "    " MEDIUMP " vec2 response = complexDivide(numerator, denominator);\n"
      "    response_out = getYForResponse(response);\n"
      "}\n";

  const char* kColorFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "void main() {\n"
      "    gl_FragColor = color;\n"
      "}\n";

  const char* kFadeSquareFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "void main() {\n"
      "    float alpha1 = clamp((dimensions_out.x - abs(coordinates_out.x) * dimensions_out.x) * 0.5, 0.0, 1.0);\n"
      "    float alpha2 = clamp((dimensions_out.y - abs(coordinates_out.y) * dimensions_out.y) * 0.5, 0.0, 1.0);\n"
      "    gl_FragColor = color;\n"
      "    gl_FragColor.a = color.a * alpha1 * alpha2 * shader_values_out.x;\n"
      "}\n";

  const char* kCircleFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    float delta_center = length(coordinates_out) * 0.5 * dimensions_out.x;\n"
      "    float alpha = clamp(dimensions_out.x * 0.5 - delta_center, 0.0, 1.0);\n"
      "    gl_FragColor = color;\n"
      "    gl_FragColor.a = color.a * alpha;\n"
      "}\n";

  const char* kRingFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    float full_radius = 0.5 * dimensions_out.x;\n"
      "    float delta_center = length(coordinates_out) * full_radius;\n"
      "    float alpha_out = clamp(full_radius - delta_center, 0.0, 1.0);\n"
      "    float alpha_in = clamp(delta_center - full_radius + thickness + 1.0, 0.0, 1.0);\n"
      "    gl_FragColor = color * alpha_in + (1.0 - alpha_in) * alt_color;\n"
      "    gl_FragColor.a = gl_FragColor.a * alpha_out;\n"
      "}\n";

  const char* kRoundedCornerFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    float delta_center = length(coordinates_out * dimensions_out);\n"
      "    float alpha = clamp(delta_center - dimensions_out.x + 0.5, 0.0, 1.0);\n"
      "    gl_FragColor = color;\n"
      "    gl_FragColor.a = color.a * alpha;\n"
      "}\n";

  const char* kRoundedRectangleFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "uniform " MEDIUMP " float rounding;\n"
      "void main() {\n"
      "    vec2 center_offset = abs(coordinates_out) * dimensions_out - dimensions_out;\n"
      "    float delta_center = length(max(center_offset + vec2(rounding, rounding), vec2(0.0, 0.0)));\n"
      "    float alpha = clamp((rounding - delta_center) * 0.5 + 0.5, 0.0, 1.0);\n"
      "    gl_FragColor = color;\n"
      "    gl_FragColor.a = color.a * alpha;\n"
      "}\n";

  const char* kDiamondFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    float full_radius = 0.5 * dimensions_out.x;\n"
      "    float delta_center = (abs(coordinates_out.x) + abs(coordinates_out.y)) * full_radius;\n"
      "    float alpha_out = clamp(full_radius - delta_center, 0.0, 1.0);\n"
      "    float alpha_in = clamp(delta_center - full_radius + thickness + 1.0, 0.0, 1.0);\n"
      "    gl_FragColor = color * alpha_in + (1.0 - alpha_in) * alt_color;\n"
      "    gl_FragColor.a = gl_FragColor.a * alpha_out;\n"
      "}\n";

  const char* kRoundedRectangleBorderFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "uniform " MEDIUMP " float rounding;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "uniform " MEDIUMP " float alpha_mult;\n"
      "void main() {\n"
      "    vec2 center_offset = abs(coordinates_out) * dimensions_out - dimensions_out;\n"
      "    float delta_center = length(max(center_offset + vec2(rounding, rounding), vec2(0.0, 0.0)));\n"
      "    float inside_rounding = rounding + 2.0 * thickness;\n"
      "    float delta_center_inside = length(max(center_offset + vec2(inside_rounding, inside_rounding), vec2(0.0, 0.0)));\n"
      "    float border_delta = (rounding - delta_center) * 0.5;\n"
      "    float inside_border_delta = (rounding - delta_center_inside) * 0.5;\n"
      "    float alpha = clamp(border_delta + 0.5, 0.0, 1.0) * clamp(-inside_border_delta + 0.5, 0.0, 1.0);\n"
      "    gl_FragColor = color;\n"
      "    gl_FragColor.a = color.a * alpha_mult * alpha;\n"
      "}\n";

  const char* kRotarySliderFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "uniform " MEDIUMP " vec4 thumb_color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "uniform " MEDIUMP " float thumb_amount;\n"
      "uniform " MEDIUMP " float start_pos;\n"
      "uniform " MEDIUMP " float max_arc;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    " MEDIUMP " float rads = atan(coordinates_out.x, coordinates_out.y);\n"
      "    float full_radius = 0.5 * dimensions_out.x;\n"
      "    float delta_center = length(coordinates_out) * full_radius;\n"
      "    float center_arc = full_radius - thickness * 0.5 - 0.5;\n"
      "    float delta_arc = delta_center - center_arc;\n"
      "    float distance_arc = abs(delta_arc);\n"
      "    float dist_curve_left = max(center_arc * (rads - max_arc), 0.0);\n"
      "    float dist_curve = max(center_arc * (-rads - max_arc), dist_curve_left);\n"
      "    float alpha = clamp(thickness * 0.5 - length(vec2(distance_arc, dist_curve)) + 0.5, 0.0, 1.0);\n"
      "    float delta_rads = rads - shader_values_out.x;\n"
      "    float color_step1 = step(0.0, delta_rads);\n"
      "    float color_step2 = step(0.0, start_pos - rads);\n"
      "    float color_step = abs(color_step2 - color_step1);\n"
      "    gl_FragColor = alt_color * color_step + color * (1.0 - color_step);\n"
      "    gl_FragColor.a = gl_FragColor.a * alpha;\n"
      "    float thumb_length = full_radius * thumb_amount;\n"
      "    float thumb_x = sin(delta_rads) * delta_center;\n"
      "    float thumb_y = cos(delta_rads) * delta_center - center_arc;\n"
      "    float adjusted_thumb_y = min(thumb_y + thumb_length, 0.0);\n"
      "    float outside_arc_step = step(0.0, thumb_y);\n"
      "    float thumb_y_distance = thumb_y * outside_arc_step + adjusted_thumb_y * (1.0 - outside_arc_step);\n"
      "    float thumb_distance = length(vec2(thumb_x, thumb_y_distance));\n"
      "    float thumb_alpha = clamp(thickness * 0.5 - thumb_distance + 0.5, 0.0, 1.0);\n"
      "    gl_FragColor = gl_FragColor * (1.0 - thumb_alpha) + thumb_color * thumb_alpha;\n"
      "}\n";

  const char* kRotaryModulationFragmentShader =
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "uniform " MEDIUMP " vec4 mod_color;\n"
      "uniform " MEDIUMP " float alpha_mult;\n"
      "uniform " MEDIUMP " float start_pos;\n"
      "const " MEDIUMP " float kPi = 3.14159265359;\n" \
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float full_radius = dimensions_out.x * 0.5;\n"
      "    " MEDIUMP " float dist = length(coordinates_out) * full_radius;\n"
      "    " MEDIUMP " float inner_radius = full_radius - thickness;\n"
      "    " MEDIUMP " float dist_outer_amp = clamp((full_radius - dist) * 0.5 + 0.5, 0.0, 1.0);\n"
      "    " MEDIUMP " float dist_amp = dist_outer_amp * clamp((dist - inner_radius) * 0.5 + 0.5, 0.0, 1.0);\n"
      "    " MEDIUMP " float rads = mod(atan(coordinates_out.x, coordinates_out.y) + kPi + start_pos, 2.0 * kPi) - kPi;\n"
      "    " MEDIUMP " float rads_amp_low = clamp(full_radius * 0.5 * (rads - shader_values_out.x) + 1.0, 0.0, 1.0);\n"
      "    " MEDIUMP " float rads_amp_high = clamp(full_radius * 0.5 * (shader_values_out.y - rads) + 1.0, 0.0, 1.0);\n"
      "    " MEDIUMP " float rads_amp_low_stereo = clamp(full_radius * 0.5 * (rads - shader_values_out.z) + 0.5, 0.0, 1.0);\n"
      "    " MEDIUMP " float rads_amp_high_stereo = clamp(full_radius * 0.5 * (shader_values_out.a - rads) + 0.5, 0.0, 1.0);\n"
      "    " MEDIUMP " float alpha = rads_amp_low * rads_amp_high;\n"
      "    " MEDIUMP " float alpha_stereo = rads_amp_low_stereo * rads_amp_high_stereo;\n"
      "    " MEDIUMP " float alpha_center = min(alpha, alpha_stereo);\n"
      "    " MEDIUMP " vec4 color_left = (alpha - alpha_center) * color;\n"
      "    " MEDIUMP " vec4 color_right = (alpha_stereo - alpha_center) * alt_color;\n"
      "    " MEDIUMP " vec4 color_center = alpha_center * mod_color;\n"
      "    " MEDIUMP " vec4 out_color = color * (1.0 - alpha_stereo) + alt_color * alpha_stereo;\n"
      "    out_color = out_color * (1.0 - alpha_center) + color_center * alpha_center;\n"
      "    out_color.a = max(alpha, alpha_stereo) * alpha_mult * dist_amp;\n"
      "    gl_FragColor = out_color;\n"
      "}\n";

  const char* kHorizontalSliderFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "uniform " MEDIUMP " vec4 thumb_color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "uniform " MEDIUMP " float thumb_amount;\n"
      "uniform " MEDIUMP " float start_pos;\n"
      "uniform " MEDIUMP " float rounding;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    vec2 position = coordinates_out * dimensions_out;\n"
      "    vec2 center_offset = abs(position) - vec2(dimensions_out.x, thickness);\n"
      "    float delta_center = length(max(center_offset + vec2(rounding, rounding), vec2(0.0, 0.0)));\n"
      "    float alpha = clamp((rounding - delta_center) * 0.5 + 0.5, 0.0, 1.0);\n"
      "    float adjusted_value = shader_values_out.x * 2.0 - 1.0;\n"
      "    float delta_pos = coordinates_out.x - adjusted_value;\n"
      "    float color_step1 = step(0.001, delta_pos);\n"
      "    float color_step2 = step(0.001, start_pos - coordinates_out.x);\n"
      "    float color_step = abs(color_step2 - color_step1);\n"
      "    gl_FragColor = alt_color * color_step + color * (1.0 - color_step);\n"
      "    gl_FragColor.a = gl_FragColor.a * alpha;\n"
      "    vec2 thumb_center_offset = abs(position - vec2(adjusted_value * dimensions_out.x, 0.0)) - vec2(thumb_amount, thickness);\n"
      "    float thumb_delta_center = length(max(thumb_center_offset + vec2(rounding, rounding), vec2(0.0, 0.0)));\n"
      "    float thumb_alpha = clamp((rounding - thumb_delta_center) * 0.5 + 0.5, 0.0, 1.0) * alpha;\n"
      "    gl_FragColor = gl_FragColor * (1.0 - thumb_alpha) + thumb_color * thumb_alpha;\n"
      "}\n";

  const char* kVerticalSliderFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "uniform " MEDIUMP " vec4 thumb_color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "uniform " MEDIUMP " float thumb_amount;\n"
      "uniform " MEDIUMP " float start_pos;\n"
      "uniform " MEDIUMP " float rounding;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    vec2 position = coordinates_out * dimensions_out;\n"
      "    vec2 center_offset = abs(position) - vec2(thickness, dimensions_out.y);\n"
      "    float delta_center = length(max(center_offset + vec2(rounding, rounding), vec2(0.0, 0.0)));\n"
      "    float alpha = clamp((rounding - delta_center) * 0.5 + 0.5, 0.0, 1.0);\n"
      "    float adjusted_value = shader_values_out.x * 2.0 - 1.0;\n"
      "    float delta_pos = coordinates_out.y - adjusted_value;\n"
      "    float color_step1 = step(0.001, delta_pos);\n"
      "    float color_step2 = step(0.001, start_pos - coordinates_out.y);\n"
      "    float color_step = abs(color_step2 - color_step1);\n"
      "    gl_FragColor = color * color_step + alt_color * (1.0 - color_step);\n"
      "    gl_FragColor.a = gl_FragColor.a * alpha;\n"
      "    vec2 thumb_center_offset = abs(position - vec2(0.0, adjusted_value * dimensions_out.y)) - vec2(thickness, thumb_amount);\n"
      "    float thumb_delta_center = length(max(thumb_center_offset + vec2(rounding, rounding), vec2(0.0, 0.0)));\n"
      "    float thumb_alpha = clamp((rounding - thumb_delta_center) * 0.5 + 0.5, 0.0, 1.0) * alpha;\n"
      "    gl_FragColor = gl_FragColor * (1.0 - thumb_alpha) + thumb_color * thumb_alpha;\n"
      "}\n";

  const char* kLinearModulationFragmentShader =
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "uniform " MEDIUMP " vec4 mod_color;\n"
      "\n"
      "void main() {\n"
      "    " MEDIUMP " float position = coordinates_out.x * 0.5 + 0.5;\n"
      "    " MEDIUMP " float dist1 = clamp(200.0 * (position - shader_values_out.x), 0.0, 1.0);\n"
      "    " MEDIUMP " float dist2 = clamp(200.0 * (shader_values_out.y - position), 0.0, 1.0);\n"
      "    " MEDIUMP " float stereo_dist1 = clamp(200.0 * (position - shader_values_out.z), 0.0, 1.0);\n"
      "    " MEDIUMP " float stereo_dist2 = clamp(200.0 * (shader_values_out.a - position), 0.0, 1.0);\n"
      "    " MEDIUMP " float alpha = dist1 * dist2;\n"
      "    " MEDIUMP " float alpha_stereo = stereo_dist1 * stereo_dist2;\n"
      "    " MEDIUMP " float alpha_center = min(alpha, alpha_stereo);\n"
      "    " MEDIUMP " vec4 color_left = (alpha - alpha_center) * color;\n"
      "    " MEDIUMP " vec4 color_right = (alpha_stereo - alpha_center) * alt_color;\n"
      "    " MEDIUMP " vec4 color_center = alpha_center * mod_color;\n"
      "    " MEDIUMP " vec4 color = color_left + color_right + color_center;\n"
      "    color.a = max(alpha, alpha_stereo);\n"
      "    gl_FragColor = color;\n"
      "}\n";

  const char* kModulationKnobFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " vec4 alt_color;\n"
      "uniform " MEDIUMP " vec4 mod_color;\n"
      "uniform " MEDIUMP " vec4 background_color;\n"
      "uniform " MEDIUMP " vec4 thumb_color;\n"
      "varying " MEDIUMP " vec2 dimensions_out;\n"
      "uniform " MEDIUMP " float thickness;\n"
      "uniform " MEDIUMP " float alpha_mult;\n"
      "varying " MEDIUMP " vec4 shader_values_out;\n"
      "varying " MEDIUMP " vec2 coordinates_out;\n"
      "void main() {\n"
      "    float rads = atan(coordinates_out.x, -coordinates_out.y);\n"
      "    float full_radius = 0.5 * dimensions_out.x;\n"
      "    float delta_center = length(coordinates_out) * full_radius;\n"
      "    float circle_alpha = clamp(full_radius - delta_center, 0.0, 1.0);\n"
      "    float delta_rads = rads - shader_values_out.x;\n"
      "    float color_amount = clamp(delta_rads * max(delta_center, 1.0) * 1.6, 0.0, 1.0);\n"
      "    gl_FragColor = alt_color * color_amount + color * (1.0 - color_amount);\n"
      "    gl_FragColor.a = gl_FragColor.a * circle_alpha;\n"
      "    float center_arc = full_radius - thickness * 0.5 - 0.5;\n"
      "    float delta_arc = delta_center - center_arc;\n"
      "    float distance_arc = abs(delta_arc);\n"
      "    float thumb_alpha = clamp(thickness * 0.5 - distance_arc + 0.5, 0.0, 1.0);\n"
      "    gl_FragColor = gl_FragColor * (1.0 - thumb_alpha) + thumb_color * thumb_alpha;\n"
      "    float mod_alpha1 = clamp(full_radius * 0.48 - delta_center, 0.0, 1.0) * mod_color.a;\n"
      "    float mod_alpha2 = clamp(full_radius * 0.35 - delta_center, 0.0, 1.0) * mod_color.a;\n"
      "    gl_FragColor = gl_FragColor * (1.0 - mod_alpha1) + background_color * mod_alpha1;\n"
      "    gl_FragColor = gl_FragColor * (1.0 - mod_alpha2) + mod_color * mod_alpha2;\n"
      "    gl_FragColor.a = gl_FragColor.a * alpha_mult;\n"
      "}\n";

  const char* kFilterFragmentShader =
      "uniform " MEDIUMP " vec4 color_from;\n"
      "uniform " MEDIUMP " vec4 color_to;\n"
      "uniform " MEDIUMP " float line_width;\n"
      "uniform " MEDIUMP " float boost;\n"
      "varying " MEDIUMP " float depth_out;\n"
      "varying " MEDIUMP " float distance;\n"
      "void main() {\n"
      "    " MEDIUMP " vec4 color = color_to * distance + color_from * (1.0 - distance);\n"
      "    " MEDIUMP " float dist_from_edge = min(depth_out, 1.0 - depth_out);\n"
      "    " MEDIUMP " float mult = 1.0 + boost * max(dist_from_edge - 2.0 / line_width, 0.0);\n"
      "    " MEDIUMP " vec4 result = mult * color;\n"
      "    " MEDIUMP " float scale = line_width * dist_from_edge;\n"
      "    result.a = scale / 2.0;\n"
      "    gl_FragColor = result;\n"
      "}\n";

  const char* kLineFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "uniform " MEDIUMP " float line_width;\n"
      "uniform " MEDIUMP " float boost;\n"
      "varying " MEDIUMP " float depth_out;\n"
      "void main() {\n"
      "    " MEDIUMP " float dist_from_edge = min(depth_out, 1.0 - depth_out);\n"
      "    " MEDIUMP " float mult = 1.0 + boost * max(dist_from_edge - 2.0 / line_width, 0.0);\n"
      "    " MEDIUMP " vec4 result = mult * color;\n"
      "    " MEDIUMP " float scale = line_width * dist_from_edge;\n"
      "    result.a = result.a * scale / 2.0;\n"
      "    gl_FragColor = result;\n"
      "}\n";

  const char* kFillFragmentShader =
      "uniform " MEDIUMP " vec4 color_from;\n"
      "uniform " MEDIUMP " vec4 color_to;\n"
      "varying " MEDIUMP " float boost;\n"
      "varying " MEDIUMP " float distance;\n"
      "void main() {\n"
      "    " MEDIUMP " float delta = abs(distance);\n"
      "    " MEDIUMP " vec4 base_color = color_to * delta + color_from * (1.0 - delta);\n"
      "    gl_FragColor = base_color;\n"
      "    gl_FragColor.a = (boost + 1.0) * base_color.a;\n"
    "}\n";

  const char* kLineVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "uniform " MEDIUMP " vec2 scale;\n"
      "out " MEDIUMP " float depth_out;\n"
      "\n"
      "void main() {\n"
      "    depth_out = position.z;\n"
      "    gl_Position = position;\n"
      "    gl_Position.x = position.x * scale.x;\n"
      "    gl_Position.y = position.y * scale.y;\n"
      "    gl_Position.z = 0.0;\n"
      "    gl_Position.w = 1.0;\n"
      "}\n";

  const char* kFillVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "uniform " MEDIUMP " vec2 scale;\n"
      "uniform " MEDIUMP " float center_position;\n"
      "uniform " MEDIUMP " float boost_amount;\n"
      "out " MEDIUMP " float distance;\n"
      "out " MEDIUMP " float boost;\n"
      "\n"
      "void main() {\n"
      "    distance = (position.y - center_position) / (1.0 - center_position);\n"
      "    boost = boost_amount * position.z;\n"
      "    gl_Position = position;\n"
      "    gl_Position.x = gl_Position.x * scale.x;\n"
      "    gl_Position.y = gl_Position.y * scale.y;\n"
      "    gl_Position.z = 0.0;\n"
      "    gl_Position.a = 1.0;\n"
      "}\n";

  const char* kBarFragmentShader =
      "uniform " MEDIUMP " vec4 color;\n"
      "varying " MEDIUMP " vec2 corner_out;\n"
      "varying " MEDIUMP " vec2 size;\n"
      "void main() {\n"
      "    " MEDIUMP " float alpha_x = min(corner_out.x * size.x, (1.0 - corner_out.x) * size.x);\n"
      "    " MEDIUMP " float alpha_y = min(corner_out.y * size.y, (1.0 - corner_out.y) * size.y);\n"
      "    gl_FragColor = color;\n"
      "    gl_FragColor.a = gl_FragColor.a * min(1.0, min(alpha_x, alpha_y));\n"
      "}\n";

  const char* kBarHorizontalVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "attribute " MEDIUMP " vec2 corner;\n"
      "uniform " MEDIUMP " float offset;\n"
      "uniform " MEDIUMP " float scale;\n"
      "uniform " MEDIUMP " float width_percent;\n"
      "uniform " MEDIUMP " vec2 dimensions;\n"
      "out " MEDIUMP " vec2 corner_out;\n"
      "out " MEDIUMP " vec2 size;\n"
      "void main()\n"
      "{\n"
      "    gl_Position = position;\n"
      "    size.x = position.z * dimensions.x / 2.0;\n"
      "    size.y = width_percent * dimensions.y / 2.0;\n"
      "    gl_Position.x = scale * (position.x + 1.0) - 1.0;\n"
      "    corner_out = corner;\n"
      "    gl_Position = gl_Position + vec4(0.0, offset - width_percent * corner.y, 0.0, 0.0);\n"
      "    gl_Position.z = 0.0;\n"
      "    gl_Position.w = 1.0;\n"
      "}\n";

  const char* kBarVerticalVertexShader =
      "attribute " MEDIUMP " vec4 position;\n"
      "attribute " MEDIUMP " vec2 corner;\n"
      "uniform " MEDIUMP " float offset;\n"
      "uniform " MEDIUMP " float scale;\n"
      "uniform " MEDIUMP " float width_percent;\n"
      "uniform " MEDIUMP " vec2 dimensions;\n"
      "out " MEDIUMP " vec2 corner_out;\n"
      "out " MEDIUMP " vec2 size;\n"
      "void main()\n"
      "{\n"
      "    gl_Position = position;\n"
      "    size.x = width_percent * dimensions.x / 2.0;\n"
      "    size.y = position.z * dimensions.y / 2.0;\n"
      "    gl_Position.x = scale * (position.x + 1.0) - 1.0;\n"
      "    corner_out = corner;\n"
      "    gl_Position = gl_Position + vec4(offset + width_percent * corner.x, 0.0, 0.0, 0.0);\n"
      "    gl_Position.z = 0.0;\n"
      "    gl_Position.w = 1.0;\n"
      "}\n";

  inline String translateFragmentShader(const String& code) {
  #if OPENGL_ES
    return String("#version 300 es\n") + "out mediump vec4 fragColor;\n" +
      code.replace("varying", "in").replace("texture2D", "texture").replace("gl_FragColor", "fragColor");
  #else
    return OpenGLHelpers::translateFragmentShaderToV3(code);
  #endif
  }

  inline String translateVertexShader(const String& code) {
  #if OPENGL_ES
    return String("#version 300 es\n") + code.replace("attribute", "in").replace("varying", "out");
  #else
    return OpenGLHelpers::translateVertexShaderToV3(code);
  #endif
  }
} // namespace

OpenGLShaderProgram* Shaders::getShaderProgram(VertexShader vertex_shader, FragmentShader fragment_shader,
                                               const GLchar** varyings) {
  int shader_program_index = vertex_shader * kNumFragmentShaders + fragment_shader;
  if (shader_programs_.count(shader_program_index))
    return shader_programs_.at(shader_program_index).get();

  shader_programs_[shader_program_index] = std::make_unique<OpenGLShaderProgram>(*open_gl_context_);
  OpenGLShaderProgram* result = shader_programs_[shader_program_index].get();
  GLuint program_id = result->getProgramID();
  open_gl_context_->extensions.glAttachShader(program_id, getVertexShaderId(vertex_shader));
  open_gl_context_->extensions.glAttachShader(program_id, getFragmentShaderId(fragment_shader));
  if (varyings)
    open_gl_context_->extensions.glTransformFeedbackVaryings(program_id, 1, varyings, GL_INTERLEAVED_ATTRIBS);

  result->link();
  return result;
}

const char* Shaders::getVertexShader(VertexShader shader) {
  switch (shader) {
    case kImageVertex:
      return kImageVertexShader;
    case kPassthroughVertex:
      return kPassthroughVertexShader;
    case kScaleVertex:
      return kScaleVertexShader;
    case kRotaryModulationVertex:
      return kRotaryModulationVertexShader;
    case kLinearModulationVertex:
      return kLinearModulationVertexShader;
    case kGainMeterVertex:
      return kGainMeterVertexShader;
    case kAnalogFilterResponseVertex:
      return kAnalogFilterResponseVertexShader;
    case kCombFilterResponseVertex:
      return kCombFilterResponseVertexShader;
    case kPositiveFlangeFilterResponseVertex:
      return kPositiveFlangeFilterResponseVertexShader;
    case kNegativeFlangeFilterResponseVertex:
      return kNegativeFlangeFilterResponseVertexShader;
    case kDigitalFilterResponseVertex:
      return kDigitalFilterResponseVertexShader;
    case kDiodeFilterResponseVertex:
      return kDiodeFilterResponseVertexShader;
    case kDirtyFilterResponseVertex:
      return kDirtyFilterResponseVertexShader;
    case kFormantFilterResponseVertex:
      return kFormantFilterResponseVertexShader;
    case kLadderFilterResponseVertex:
      return kLadderFilterResponseVertexShader;
    case kPhaserFilterResponseVertex:
      return kPhaserFilterResponseVertexShader;
    case kEqFilterResponseVertex:
      return kEqFilterResponseVertexShader;
    case kLineVertex:
      return kLineVertexShader;
    case kFillVertex:
      return kFillVertexShader;
    case kBarHorizontalVertex:
      return kBarHorizontalVertexShader;
    case kBarVerticalVertex:
      return kBarVerticalVertexShader;
    default:
      VITAL_ASSERT(false);
      return nullptr;
  }
}

const char* Shaders::getFragmentShader(FragmentShader shader) {
  switch (shader) {
    case kImageFragment:
      return kImageFragmentShader;
    case kTintedImageFragment:
      return kTintedImageFragmentShader;
    case kGainMeterFragment:
      return kGainMeterFragmentShader;
    case kFilterResponseFragment:
      return kFilterFragmentShader;
    case kLineFragment:
      return kLineFragmentShader;
    case kFillFragment:
      return kFillFragmentShader;
    case kBarFragment:
      return kBarFragmentShader;
    case kColorFragment:
      return kColorFragmentShader;
    case kFadeSquareFragment:
      return kFadeSquareFragmentShader;
    case kCircleFragment:
      return kCircleFragmentShader;
    case kRingFragment:
      return kRingFragmentShader;
    case kDiamondFragment:
      return kDiamondFragmentShader;
    case kRoundedCornerFragment:
      return kRoundedCornerFragmentShader;
    case kRoundedRectangleFragment:
      return kRoundedRectangleFragmentShader;
    case kRoundedRectangleBorderFragment:
      return kRoundedRectangleBorderFragmentShader;
    case kRotarySliderFragment:
      return kRotarySliderFragmentShader;
    case kRotaryModulationFragment:
      return kRotaryModulationFragmentShader;
    case kHorizontalSliderFragment:
      return kHorizontalSliderFragmentShader;
    case kVerticalSliderFragment:
      return kVerticalSliderFragmentShader;
    case kLinearModulationFragment:
      return kLinearModulationFragmentShader;
    case kModulationKnobFragment:
      return kModulationKnobFragmentShader;
    default:
      VITAL_ASSERT(false);
      return nullptr;
  }
}

bool Shaders::checkShaderCorrect(OpenGLExtensionFunctions& extensions, GLuint shader_id) const {
  GLint status = GL_FALSE;
  extensions.glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);

  if (status != GL_FALSE)
    return true;

  GLchar info[16384];
  GLsizei info_length = 0;
  extensions.glGetShaderInfoLog(shader_id, sizeof(info), &info_length, info);
  DBG(String(info, (size_t)info_length));
  return false;
}

GLuint Shaders::createVertexShader(OpenGLExtensionFunctions& extensions, VertexShader shader) const {
  GLuint shader_id = extensions.glCreateShader(GL_VERTEX_SHADER);
  String code_string = translateVertexShader(getVertexShader(shader));
  const GLchar* code = code_string.toRawUTF8();
  extensions.glShaderSource(shader_id, 1, &code, nullptr);
  extensions.glCompileShader(shader_id);

  VITAL_ASSERT(checkShaderCorrect(extensions, shader_id));
  return shader_id;
}

GLuint Shaders::createFragmentShader(OpenGLExtensionFunctions& extensions, FragmentShader shader) const {
  GLuint shader_id = extensions.glCreateShader(GL_FRAGMENT_SHADER);
  String code_string = translateFragmentShader(getFragmentShader(shader));
  const GLchar* code = code_string.toRawUTF8();
  extensions.glShaderSource(shader_id, 1, &code, nullptr);
  extensions.glCompileShader(shader_id);

  VITAL_ASSERT(checkShaderCorrect(extensions, shader_id));
  return shader_id;
}

Shaders::Shaders(OpenGLContext& open_gl_context) : open_gl_context_(&open_gl_context),
                                                   vertex_shader_ids_(), fragment_shader_ids_() { }
