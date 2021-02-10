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

#include "wave_line_source.h"

#include "futils.h"
#include "utils.h"
#include "poly_utils.h"
#include "wave_frame.h"
#include "wavetable_component_factory.h"

WaveLineSource::WaveLineSourceKeyframe::WaveLineSourceKeyframe() :
    line_generator_(vital::WaveFrame::kWaveformSize) {
  pull_power_ = 0.0f;
}

void WaveLineSource::WaveLineSourceKeyframe::copy(const WavetableKeyframe* keyframe) {
  const WaveLineSourceKeyframe* source = dynamic_cast<const WaveLineSourceKeyframe*>(keyframe);

  const LineGenerator* source_generator = source->getLineGenerator();
  line_generator_.setNumPoints(source_generator->getNumPoints());
  line_generator_.setSmooth(source_generator->smooth());
  for (int i = 0; i < line_generator_.getNumPoints(); ++i) {
    line_generator_.setPoint(i, source_generator->getPoint(i));
    line_generator_.setPower(i, source_generator->getPower(i));
  }
}

void WaveLineSource::WaveLineSourceKeyframe::interpolate(const WavetableKeyframe* from_keyframe,
                                                         const WavetableKeyframe* to_keyframe,
                                                         float t) {
  const WaveLineSourceKeyframe* from = dynamic_cast<const WaveLineSourceKeyframe*>(from_keyframe);
  const WaveLineSourceKeyframe* to = dynamic_cast<const WaveLineSourceKeyframe*>(to_keyframe);
  VITAL_ASSERT(from->getNumPoints() == to->getNumPoints());

  float relative_power = from->getPullPower() - to->getPullPower();
  float adjusted_t = vital::futils::powerScale(t, relative_power);

  const LineGenerator* from_generator = from->getLineGenerator();
  const LineGenerator* to_generator = to->getLineGenerator();
  int num_points = from_generator->getNumPoints();
  line_generator_.setNumPoints(num_points);
  line_generator_.setSmooth(from_generator->smooth());

  for (int i = 0; i < num_points; ++i) {
    std::pair<float, float> from_point = from_generator->getPoint(i);
    std::pair<float, float> to_point = to_generator->getPoint(i);
    line_generator_.setPoint(i, {
      linearTween(from_point.first, to_point.first, adjusted_t),
      linearTween(from_point.second, to_point.second, adjusted_t),
    });
    line_generator_.setPower(i, linearTween(from_generator->getPower(i), to_generator->getPower(i), adjusted_t));
  }
}

void WaveLineSource::WaveLineSourceKeyframe::render(vital::WaveFrame* wave_frame) {
  line_generator_.render();
  memcpy(wave_frame->time_domain, line_generator_.getBuffer(), vital::WaveFrame::kWaveformSize * sizeof(float));
  for (int i = 0; i < vital::WaveFrame::kWaveformSize; ++i)
    wave_frame->time_domain[i] = wave_frame->time_domain[i] * 2.0f - 1.0f;
  wave_frame->toFrequencyDomain();
}

json WaveLineSource::WaveLineSourceKeyframe::stateToJson() {
  json data = WavetableKeyframe::stateToJson();
  data["pull_power"] = pull_power_;
  data["line"] = line_generator_.stateToJson();
  return data;
}

void WaveLineSource::WaveLineSourceKeyframe::jsonToState(json data) {
  WavetableKeyframe::jsonToState(data);
  pull_power_ = 0.0f;
  if (data.count("pull_power"))
    pull_power_ = data["pull_power"];
  if (data.count("line"))
    line_generator_.jsonToState(data["line"]);
}

WavetableKeyframe* WaveLineSource::createKeyframe(int position) {
  WaveLineSourceKeyframe* keyframe = new WaveLineSourceKeyframe();
  interpolate(keyframe, position);
  return keyframe;
}

void WaveLineSource::render(vital::WaveFrame* wave_frame, float position) {
  interpolate(&compute_frame_, position);
  compute_frame_.render(wave_frame);
}

WavetableComponentFactory::ComponentType WaveLineSource::getType() {
  return WavetableComponentFactory::kLineSource;
}

json WaveLineSource::stateToJson() {
  json data = WavetableComponent::stateToJson();
  data["num_points"] = num_points_;
  return data;
}

void WaveLineSource::jsonToState(json data) {
  WavetableComponent::jsonToState(data);
  setNumPoints(data["num_points"]);
}

void WaveLineSource::setNumPoints(int num_points) {
  num_points_ = num_points;
}

WaveLineSource::WaveLineSourceKeyframe* WaveLineSource::getKeyframe(int index) {
  WavetableKeyframe* wavetable_keyframe = keyframes_[index].get();
  return dynamic_cast<WaveLineSource::WaveLineSourceKeyframe*>(wavetable_keyframe);
}
