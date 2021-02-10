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

class SlewLimitModifier : public WavetableComponent {
  public:
    class SlewLimitModifierKeyframe : public WavetableKeyframe {
      public:
        SlewLimitModifierKeyframe();
        virtual ~SlewLimitModifierKeyframe() { }

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;
        void render(vital::WaveFrame* wave_frame) override;
        json stateToJson() override;
        void jsonToState(json data) override;

        float getSlewUpLimit() { return slew_up_run_rise_; }
        float getSlewDownLimit() { return slew_down_run_rise_; }

        void setSlewUpLimit(float slew_up_limit) { slew_up_run_rise_ = slew_up_limit; }
        void setSlewDownLimit(float slew_down_limit) { slew_down_run_rise_ = slew_down_limit; }

      protected:
        float slew_up_run_rise_;
        float slew_down_run_rise_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlewLimitModifierKeyframe)
    };

    SlewLimitModifier() { }
    virtual ~SlewLimitModifier() { }

    virtual WavetableKeyframe* createKeyframe(int position) override;
    virtual void render(vital::WaveFrame* wave_frame, float position) override;
    virtual WavetableComponentFactory::ComponentType getType() override;

    SlewLimitModifierKeyframe* getKeyframe(int index);

  protected:
    SlewLimitModifierKeyframe compute_frame_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlewLimitModifier)
};

