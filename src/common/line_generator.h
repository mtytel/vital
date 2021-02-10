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
#include "json/json.h"

using json = nlohmann::json;

class LineGenerator {
  public:
    static constexpr int kMaxPoints = 100;
    static constexpr int kDefaultResolution = 2048;
    static constexpr int kExtraValues = 3;

    static force_inline float smoothTransition(float t) {
      return 0.5f * sinf((t - 0.5f) * vital::kPi) + 0.5f;
    }

    LineGenerator(int resolution = kDefaultResolution);
    virtual ~LineGenerator() { }

    void setLoop(bool loop) { loop_ = loop; render(); }
    void setName(const std::string& name) { name_ = name; }
    void setLastBrowsedFile(const std::string& path) { last_browsed_file_ = path; }
    void setSmooth(bool smooth) { smooth_ = smooth; checkLineIsLinear(); render(); }
    void initLinear();
    void initTriangle();
    void initSquare();
    void initSin();
    void initSawUp();
    void initSawDown();
    void render();
    json stateToJson();
    static bool isValidJson(json data);
    void jsonToState(json data);
    float valueAtPhase(float phase);
    void checkLineIsLinear();

    float getValueBetweenPoints(float x, int index_from, int index_to);
    float getValueAtPhase(float phase);
    std::string getName() const { return name_; }
    std::string getLastBrowsedFile() const { return last_browsed_file_; }

    void addPoint(int index, std::pair<float, float> position);
    void addMiddlePoint(int index);
    void removePoint(int index);

    void flipHorizontal();
    void flipVertical();

    std::pair<float, float> lastPoint() const { return points_[num_points_ - 1]; }
    float lastPower() const { return powers_[num_points_ - 1]; }

    force_inline int resolution() const { return resolution_; }
    force_inline bool linear() const { return linear_; }
    force_inline bool smooth() const { return smooth_; }
    force_inline vital::mono_float* getBuffer() const { return buffer_.get() + 1; }
    force_inline vital::mono_float* getCubicInterpolationBuffer() const { return buffer_.get(); }

    force_inline std::pair<float, float> getPoint(int index) const {
      VITAL_ASSERT(index < kMaxPoints && index >= 0);
      return points_[index];
    }

    force_inline float getPower(int index) const {
      VITAL_ASSERT(index < kMaxPoints && index >= 0);
      return powers_[index];
    }

    force_inline int getNumPoints() const {
      return num_points_;
    }

    force_inline void setPoint(int index, std::pair<float, float> point) {
      VITAL_ASSERT(index < kMaxPoints && index >= 0);
      points_[index] = point;
      checkLineIsLinear();
    }

    force_inline void setPower(int index, float power) {
      VITAL_ASSERT(index < kMaxPoints && index >= 0);
      powers_[index] = power;
      checkLineIsLinear();
    }

    force_inline void setNumPoints(int num_points) {
      VITAL_ASSERT(num_points <= kMaxPoints && num_points >= 0);
      num_points_ = num_points;
      checkLineIsLinear();
    }

    int getRenderCount() const { return render_count_; }

  protected:
    std::string name_;
    std::string last_browsed_file_;
    std::pair<float, float> points_[kMaxPoints];
    float powers_[kMaxPoints];
    int num_points_;
    int resolution_;

    std::unique_ptr<vital::mono_float[]> buffer_;
    bool loop_;
    bool smooth_;
    bool linear_;
    int render_count_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LineGenerator)
};

