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

#include "curve_look_and_feel.h"

#include "skin.h"
#include "fonts.h"
#include "poly_utils.h"
#include "futils.h"
#include "synth_slider.h"

CurveLookAndFeel::CurveLookAndFeel() { }

void CurveLookAndFeel::drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                                        float slider_t, float start_angle, float end_angle,
                                        Slider& slider) {
  bool active = true;
  bool bipolar = false;
  SynthSlider* s_slider = dynamic_cast<SynthSlider*>(&slider);
  if (s_slider) {
    active = s_slider->isActive();
    bipolar = s_slider->isBipolar();
  }

  float rounding = 0.0f;
  float short_side = std::min(width, height);
  float max_width = short_side;
  SynthSection* section = slider.findParentComponentOfClass<SynthSection>();
  if (section) {
    rounding = section->findValue(Skin::kWidgetRoundedCorner);
    max_width = std::min(max_width, section->findValue(Skin::kKnobArcSize));
  }

  int inset = rounding / sqrtf(2.0f) + (short_side - max_width) / 2.0f;
  drawCurve(g, slider, x + inset, y + inset, width - 2.0f * inset, height - 2.0f * inset, active, bipolar);
}

void CurveLookAndFeel::drawCurve(Graphics& g, Slider& slider, int x, int y, int width, int height,
                                 bool active, bool bipolar) {
  static constexpr int kResolution = 16;
  static constexpr float kLineWidth = 2.0f;
  PathStrokeType stroke(kLineWidth, PathStrokeType::beveled, PathStrokeType::rounded);

  float curve_width = std::min(width, height);
  float x_offset = (width - curve_width) / 2.0f;
  float power = -slider.getValue();
  Path path;
  float start_x = x + x_offset + kLineWidth / 2.0f;
  float start_y = y + height - kLineWidth / 2.0f;
  path.startNewSubPath(start_x, start_y);
  float active_width = curve_width - kLineWidth;
  float active_height = curve_width - kLineWidth;

  if (bipolar) {
    float half_width = active_width / 2.0f;
    float half_height = active_height / 2.0f;
    for (int i = 0; i < kResolution / 2; ++i) {
      float t = 2 * (i + 1.0f) / kResolution;
      float power_t = vital::futils::powerScale(t, -power);
      path.lineTo(start_x + t * half_width, start_y - power_t * half_height);
    }
    for (int i = 0; i < kResolution / 2; ++i) {
      float t = 2 * (i + 1.0f) / kResolution;
      float power_t = vital::futils::powerScale(t, power);
      path.lineTo(start_x + t * half_width + half_width, start_y - power_t * half_height - half_height);
    }
  }
  else {
    for (int i = 0; i < kResolution; ++i) {
      float t = (i + 1.0f) / kResolution;
      float power_t = vital::futils::powerScale(t, power);
      path.lineTo(start_x + t * active_width, start_y - power_t * active_height);
    }
  }

  Colour line = slider.findColour(Skin::kRotaryArc, true);
  if (!active)
    line = slider.findColour(Skin::kWidgetPrimaryDisabled, true);

  g.setColour(line);
  g.strokePath(path, stroke);
}

