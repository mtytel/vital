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
#include "wavetable_component.h"

class WaveLineSource : public WavetableComponent {
  public:
    static constexpr int kDefaultLinePoints = 4;

    class WaveLineSourceKeyframe : public WavetableKeyframe {
      public:
        WaveLineSourceKeyframe();
        virtual ~WaveLineSourceKeyframe() = default;

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;
        void render(vital::WaveFrame* wave_frame) override;
        json stateToJson() override;
        void jsonToState(json data) override;

        inline std::pair<float, float> getPoint(int index) const { return line_generator_.getPoint(index); }
        inline float getPower(int index) const { return line_generator_.getPower(index); }
        inline void setPoint(std::pair<float, float> point, int index) { line_generator_.setPoint(index, point); }
        inline void setPower(float power, int index) { line_generator_.setPower(index, power); }
        inline void removePoint(int index) { line_generator_.removePoint(index); }
        inline void addMiddlePoint(int index) { line_generator_.addMiddlePoint(index); }
        inline int getNumPoints() const { return line_generator_.getNumPoints(); }
        inline void setSmooth(bool smooth) { line_generator_.setSmooth(smooth); }

        void setPullPower(float power) { pull_power_ = power; }
        float getPullPower() const { return pull_power_; }
        const LineGenerator* getLineGenerator() const { return &line_generator_; }
        LineGenerator* getLineGenerator() { return &line_generator_; }

      protected:
        LineGenerator line_generator_;
        float pull_power_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveLineSourceKeyframe)
    };

    WaveLineSource() : num_points_(kDefaultLinePoints), compute_frame_() { }
    virtual ~WaveLineSource() = default;

    virtual WavetableKeyframe* createKeyframe(int position) override;
    virtual void render(vital::WaveFrame* wave_frame, float position) override;
    virtual WavetableComponentFactory::ComponentType getType() override;
    virtual json stateToJson() override;
    virtual void jsonToState(json data) override;

    void setNumPoints(int num_points);
    int numPoints() { return num_points_; }
    WaveLineSourceKeyframe* getKeyframe(int index);

  protected:
    int num_points_;
    WaveLineSourceKeyframe compute_frame_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveLineSource)
};

