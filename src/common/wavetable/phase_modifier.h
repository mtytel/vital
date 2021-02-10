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

class PhaseModifier : public WavetableComponent {
  public:
    enum PhaseStyle {
      kNormal,
      kEvenOdd,
      kHarmonic,
      kHarmonicEvenOdd,
      kClear,
      kNumPhaseStyles
    };

    class PhaseModifierKeyframe : public WavetableKeyframe {
      public:
        PhaseModifierKeyframe();
        virtual ~PhaseModifierKeyframe() { }

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;
        void render(vital::WaveFrame* wave_frame) override;
        json stateToJson() override;
        void jsonToState(json data) override;

        float getPhase() { return phase_; }
        float getMix() { return mix_; }
        void setPhase(float phase) { phase_ = phase; }
        void setMix(float mix) { mix_ = mix; }
        void setPhaseStyle(PhaseStyle style) { phase_style_ = style; }

      protected:
        float phase_;
        float mix_;
        PhaseStyle phase_style_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseModifierKeyframe)
    };

    PhaseModifier() : phase_style_(kNormal) { }
    virtual ~PhaseModifier() = default;

    virtual WavetableKeyframe* createKeyframe(int position) override;
    virtual void render(vital::WaveFrame* wave_frame, float position) override;
    virtual WavetableComponentFactory::ComponentType getType() override;
    virtual json stateToJson() override;
    virtual void jsonToState(json data) override;

    PhaseModifierKeyframe* getKeyframe(int index);

    void setPhaseStyle(PhaseStyle style) { phase_style_ = style; }
    PhaseStyle getPhaseStyle() const { return phase_style_; }

  protected:
    PhaseModifierKeyframe compute_frame_;
    PhaseStyle phase_style_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseModifier)
};

