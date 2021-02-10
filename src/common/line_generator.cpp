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

#include "line_generator.h"
#include "poly_utils.h"
#include "futils.h"

LineGenerator::LineGenerator(int resolution) : points_(), powers_(), num_points_(2), resolution_(resolution),
                                               loop_(false), smooth_(false), linear_(true), render_count_(0) {
  buffer_ = std::make_unique<vital::mono_float[]>(resolution + kExtraValues);
  initLinear();
}

void LineGenerator::initLinear() {
  points_[0] = { 0.0f, 1.0f };
  points_[1] = { 1.0f, 0.0f };
  powers_[0] = 0.0f;
  powers_[1] = 0.0f;
  num_points_ = 2;
  linear_ = true;
  name_ = "Linear";
  smooth_ = false;
  render();
}

void LineGenerator::initTriangle() {
  points_[0] = { 0.0f, 1.0f };
  points_[1] = { 0.5f, 0.0f };
  points_[2] = { 1.0f, 1.0f };
  powers_[0] = 0.0f;
  powers_[1] = 0.0f;
  powers_[2] = 0.0f;
  num_points_ = 3;
  linear_ = false;
  name_ = "Triangle";
  smooth_ = false;
  render();
}

void LineGenerator::initSquare() {
  num_points_ = 5;
  points_[0] = { 0.0f, 1.0f };
  points_[1] = { 0.0f, 0.0f };
  points_[2] = { 0.5f, 0.0f };
  points_[3] = { 0.5f, 1.0f };
  points_[4] = { 1.0f, 1.0f };

  for (int i = 0; i < num_points_; ++i)
    powers_[i] = 0.0f;

  linear_ = false;
  name_ = "Square";
  smooth_ = false;
  render();
}

void LineGenerator::initSin() {
  points_[0] = { 0.0f, 1.0f };
  points_[1] = { 0.5f, 0.0f };
  points_[2] = { 1.0f, 1.0f };
  powers_[0] = 0.0f;
  powers_[1] = 0.0f;
  powers_[2] = 0.0f;
  num_points_ = 3;
  linear_ = false;
  name_ = "Sin";
  smooth_ = true;
  render();
}

void LineGenerator::initSawUp() {
  num_points_ = 3;
  points_[0] = { 0.0f, 1.0f };
  points_[1] = { 1.0f, 0.0f };
  points_[2] = { 1.0f, 1.0f };
  powers_[0] = 0.0f;
  powers_[1] = 0.0f;
  powers_[2] = 0.0f;
  linear_ = false;
  name_ = "Saw Up";
  smooth_ = false;
  render();
}

void LineGenerator::initSawDown() {
  num_points_ = 3;
  points_[0] = { 0.0f, 0.0f };
  points_[1] = { 1.0f, 1.0f };
  points_[2] = { 1.0f, 0.0f };
  powers_[0] = 0.0f;
  powers_[1] = 0.0f;
  powers_[2] = 0.0f;
  linear_ = false;
  name_ = "Saw Down";
  smooth_ = false;
  render();
}

json LineGenerator::stateToJson() {
  json point_data;
  json power_data;

  for (int i = 0; i < num_points_; ++i) {
    std::pair<float, float> point = points_[i];
    point_data.push_back(point.first);
    point_data.push_back(point.second);
    power_data.push_back(powers_[i]);
  }

  json data;
  data["num_points"] = num_points_;
  data["points"] = point_data;
  data["powers"] = power_data;
  data["name"] = name_;
  data["smooth"] = smooth_;
  return data;
}

bool LineGenerator::isValidJson(json data) {
  if (!data.count("num_points") || !data.count("points") || !data.count("powers"))
    return false;

  json point_data = data["points"];
  json power_data = data["powers"];
  return point_data.is_array() && power_data.is_array();
}

void LineGenerator::jsonToState(json data) {
  num_points_ = data["num_points"];
  json point_data = data["points"];
  json power_data = data["powers"];
  name_ = "";
  if (data.count("name"))
    name_ = data["name"].get<std::string>();

  smooth_ = false;
  if (data.count("smooth"))
    smooth_ = data["smooth"];

  int num_read = kMaxPoints;
  num_read = std::min(num_read, static_cast<int>(power_data.size()));

  for (int i = 0; i < num_read; ++i) {
    points_[i] = std::pair<float, float>(point_data[2 * i], point_data[2 * i + 1]);
    powers_[i] = power_data[i];
  }

  checkLineIsLinear();
  render();
}

void LineGenerator::render() {
  render_count_++;

  int point_index = 0;
  std::pair<float, float> last_point = points_[point_index];
  float current_power = 0.0f;
  std::pair<float, float> current_point = points_[point_index];
  if (loop_) {
    last_point = points_[num_points_ - 1];
    last_point.first -= 1.0f;
    current_power = powers_[num_points_ - 1];
  }
  for (int i = 0; i < resolution_; ++i) {
    float x = i / (resolution_ - 1.0f);
    float t = 1.0f;
    if (current_point.first > last_point.first)
      t = (x - last_point.first) / (current_point.first - last_point.first);

    if (smooth_)
      t = smoothTransition(t);

    t = vital::utils::clamp(vital::futils::powerScale(t, current_power), 0.0f, 1.0f);
    
    float y = last_point.second + t * (current_point.second - last_point.second);
    buffer_[i + 1] = 1.0f - y;

    while (x > current_point.first && point_index < num_points_) {
      current_power = powers_[point_index % num_points_];
      point_index++;
      last_point = current_point;
      current_point = points_[point_index % num_points_];
      if (point_index >= num_points_) {
        current_point.first += 1.0f;
        break;
      }
    }
  }

  if (loop_) {
    buffer_[0] = buffer_[resolution_];
    buffer_[resolution_ + 1] = buffer_[1];
    buffer_[resolution_ + 2] = buffer_[2];
  }
  else {
    buffer_[0] = buffer_[1];
    buffer_[resolution_ + 1] = buffer_[resolution_];
    buffer_[resolution_ + 2] = buffer_[resolution_];
  }
}

float LineGenerator::valueAtPhase(float phase) {
  float scaled_phase = vital::utils::clamp(phase, 0.0f, 1.0f) * resolution_;
  int index = scaled_phase;
  return vital::utils::interpolate(buffer_[index + 1], buffer_[index + 2], scaled_phase - index);
}

void LineGenerator::checkLineIsLinear() {
  linear_ = !smooth_ && num_points_ == 2 && powers_[0] == 0.0f &&
            points_[0] == std::pair<float, float>(0.0f, 1.0f) &&
            points_[1] == std::pair<float, float>(1.0f, 0.0f);
}

float LineGenerator::getValueBetweenPoints(float x, int index_from, int index_to) {
  VITAL_ASSERT(index_from >= 0 && index_to < num_points_);

  std::pair<float, float> first = points_[index_from];
  std::pair<float, float> second = points_[index_to];
  float power = powers_[index_from];

  float width = second.first - first.first;
  if (width <= 0.0f)
    return second.second;

  float t = (x - first.first) / width;
  if (smooth_)
    t = smoothTransition(t);

  t = vital::utils::clamp(vital::futils::powerScale(t, power), 0.0f, 1.0f);
  return t * (second.second - first.second) + first.second;
}

float LineGenerator::getValueAtPhase(float phase) {
  for (int i = 0; i < num_points_ - 1; ++i) {
    if (points_[i].first <= phase && points_[i + 1].first >= phase)
      return getValueBetweenPoints(phase, i, i + 1);
  }

  return lastPoint().second;
}

void LineGenerator::addPoint(int index, std::pair<float, float> position) {
  for (int i = num_points_; i > index; --i) {
    points_[i] = points_[i - 1];
    powers_[i] = powers_[i - 1];
  }

  num_points_++;
  points_[index] = position;
  powers_[index] = 0.0f;
  checkLineIsLinear();
}

void LineGenerator::addMiddlePoint(int index) {
  VITAL_ASSERT(index > 0);

  float x = (points_[index - 1].first + points_[index].first) * 0.5f;
  float y = getValueBetweenPoints(x, index - 1, index);
  addPoint(index, { x, y });
}

void LineGenerator::removePoint(int index) {
  num_points_--;
  for (int i = index; i < num_points_; ++i) {
    points_[i] = points_[i + 1];
    powers_[i] = powers_[i + 1];
  }
  checkLineIsLinear();
}

void LineGenerator::flipHorizontal() {
  for (int i = 0; i < (num_points_ + 1) / 2; ++i) {
    float tmp_x = 1.0f - points_[i].first;
    float tmp_y = points_[i].second;
    points_[i].first = 1.0f - points_[num_points_ - i - 1].first;
    points_[i].second = points_[num_points_ - i - 1].second;
    points_[num_points_ - i - 1].first = tmp_x;
    points_[num_points_ - i - 1].second = tmp_y;
  }
  for (int i = 0; i < num_points_ / 2; ++i) {
    float tmp_power = powers_[i];
    powers_[i] = -powers_[num_points_ - i - 2];
    powers_[num_points_ - i - 2] = -tmp_power;
  }
  render();
  checkLineIsLinear();
}

void LineGenerator::flipVertical() {
  for (int i = 0; i < num_points_; ++i)
    points_[i].second = 1.0f - points_[i].second;

  render();
  checkLineIsLinear();
}
