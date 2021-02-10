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

class WaveWarpModifier : public WavetableComponent {
  public:
    class WaveWarpModifierKeyframe : public WavetableKeyframe {
      public:
        WaveWarpModifierKeyframe();
        virtual ~WaveWarpModifierKeyframe() { }

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;
        void render(vital::WaveFrame* wave_frame) override;
        json stateToJson() override;
        void jsonToState(json data) override;

        float getHorizontalPower() { return horizontal_power_; }
        float getVerticalPower() { return vertical_power_; }

        void setHorizontalPower(float horizontal_power) { horizontal_power_ = horizontal_power; }
        void setVerticalPower(float vertical_power) { vertical_power_ = vertical_power; }

        void setHorizontalAsymmetric(bool horizontal_asymmetric) { horizontal_asymmetric_ = horizontal_asymmetric; }
        void setVerticalAsymmetric(bool vertical_asymmetric) { vertical_asymmetric_ = vertical_asymmetric; }

      protected:
        float horizontal_power_;
        float vertical_power_;

        bool horizontal_asymmetric_;
        bool vertical_asymmetric_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveWarpModifierKeyframe)
    };

    WaveWarpModifier() : horizontal_asymmetric_(false), vertical_asymmetric_(false) { }
    virtual ~WaveWarpModifier() = default;

    virtual WavetableKeyframe* createKeyframe(int position) override;
    virtual void render(vital::WaveFrame* wave_frame, float position) override;
    virtual WavetableComponentFactory::ComponentType getType() override;
    virtual json stateToJson() override;
    virtual void jsonToState(json data) override;

    void setHorizontalAsymmetric(bool horizontal_asymmetric) { horizontal_asymmetric_ = horizontal_asymmetric; }
    void setVerticalAsymmetric(bool vertical_asymmetric) { vertical_asymmetric_ = vertical_asymmetric; }

    bool getHorizontalAsymmetric() const { return horizontal_asymmetric_; }
    bool getVerticalAsymmetric() const { return vertical_asymmetric_; }

    WaveWarpModifierKeyframe* getKeyframe(int index);

  protected:
    WaveWarpModifierKeyframe compute_frame_;
    bool horizontal_asymmetric_;
    bool vertical_asymmetric_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveWarpModifier)
};

