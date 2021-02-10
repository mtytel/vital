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

class FrequencyFilterModifier : public WavetableComponent {
  public:
    enum FilterStyle {
      kLowPass,
      kBandPass,
      kHighPass,
      kComb,
      kNumFilterStyles
    };

    class FrequencyFilterModifierKeyframe : public WavetableKeyframe {
      public:
        FrequencyFilterModifierKeyframe();
        virtual ~FrequencyFilterModifierKeyframe() { }

        void copy(const WavetableKeyframe* keyframe) override;
        void interpolate(const WavetableKeyframe* from_keyframe,
                         const WavetableKeyframe* to_keyframe, float t) override;
        void render(vital::WaveFrame* wave_frame) override;
        json stateToJson() override;
        void jsonToState(json data) override;

        float getMultiplier(float index);

        float getCutoff() { return cutoff_; }
        float getShape() { return shape_; }

        void setStyle(FilterStyle style) { style_ = style; }
        void setCutoff(float cutoff) { cutoff_ = cutoff; }
        void setShape(float shape) { shape_ = shape; }
        void setNormalize(bool normalize) { normalize_ = normalize; }

      protected:
        FilterStyle style_;
        bool normalize_;
        float cutoff_;
        float shape_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyFilterModifierKeyframe)
      };

      FrequencyFilterModifier() : style_(kLowPass), normalize_(true) { }
      virtual ~FrequencyFilterModifier() { }

      virtual WavetableKeyframe* createKeyframe(int position) override;
      virtual void render(vital::WaveFrame* wave_frame, float position) override;
      virtual WavetableComponentFactory::ComponentType getType() override;
      virtual json stateToJson() override;
      virtual void jsonToState(json data) override;

      FrequencyFilterModifierKeyframe* getKeyframe(int index);

      FilterStyle getStyle() { return style_; }
      bool getNormalize() { return normalize_; }

      void setStyle(FilterStyle style) { style_ = style; }
      void setNormalize(bool normalize) { normalize_ = normalize; }

    protected:
      FilterStyle style_;
      bool normalize_;
      FrequencyFilterModifierKeyframe compute_frame_;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyFilterModifier)
};

