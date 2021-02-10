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

#include "wavetable_keyframe.h"

#include "utils.h"
#include "wavetable_component.h"

float WavetableKeyframe::linearTween(float point_from, float point_to, float t) {
  return vital::utils::interpolate(point_from, point_to, t);
}

float WavetableKeyframe::cubicTween(float point_prev, float point_from, float point_to, float point_next,
                                    float range_prev, float range, float range_next, float t) {
  float slope_from = 0.0f;
  float slope_to = 0.0f;
  if (range_prev > 0.0f)
    slope_from = (point_to - point_prev) / (1.0f + range_prev / range);
  if (range_next > 0.0f)
    slope_to = (point_next - point_from) / (1.0f + range_next / range);
  float delta = point_to - point_from;

  float movement = linearTween(point_from, point_to, t);
  float smooth = t * (1.0f - t) * ((1.0f - t) * (slope_from - delta) + t * (delta - slope_to));
  return movement + smooth;
}

int WavetableKeyframe::index() {
  return owner()->indexOf(this);
}

json WavetableKeyframe::stateToJson() {
  return { { "position", position_ } };
}

void WavetableKeyframe::jsonToState(json data) {
  position_ = data["position"];
}
