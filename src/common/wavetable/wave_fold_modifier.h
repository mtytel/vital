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

class WaveFoldModifier : public WavetableComponent {
  public:
    class WaveFoldModifierKeyframe : public WavetableKeyframe {
      public:
        WaveFoldModifierKeyframe();
        virtual ~WaveFoldModifierKeyframe() { }

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;
        void render(vital::WaveFrame* wave_frame) override;
        json stateToJson() override;
        void jsonToState(json data) override;

        float getWaveFoldBoost() { return wave_fold_boost_; }
        void setWaveFoldBoost(float boost) { wave_fold_boost_ = boost; }

      protected:
        float wave_fold_boost_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveFoldModifierKeyframe)
    };

    WaveFoldModifier() { }
    virtual ~WaveFoldModifier() { }

    virtual WavetableKeyframe* createKeyframe(int position) override;
    virtual void render(vital::WaveFrame* wave_frame, float position) override;
    virtual WavetableComponentFactory::ComponentType getType() override;

    WaveFoldModifierKeyframe* getKeyframe(int index);

  protected:
    WaveFoldModifierKeyframe compute_frame_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveFoldModifier)
};

