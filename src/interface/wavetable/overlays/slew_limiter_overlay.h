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

#include "wavetable_component_overlay.h"
#include "slew_limit_modifier.h"

class SlewLimiterOverlay : public WavetableComponentOverlay {
  public:
    SlewLimiterOverlay();

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;

    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;

    void setSlewLimitModifier(SlewLimitModifier* slew_modifier) {
      slew_modifier_ = slew_modifier;
      current_frame_ = nullptr;
    }

  protected:
    SlewLimitModifier* slew_modifier_;
    SlewLimitModifier::SlewLimitModifierKeyframe* current_frame_;

    std::unique_ptr<SynthSlider> up_slew_limit_;
    std::unique_ptr<SynthSlider> down_slew_limit_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlewLimiterOverlay)
};

