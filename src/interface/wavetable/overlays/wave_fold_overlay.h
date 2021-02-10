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
#include "wave_fold_modifier.h"

class WaveFoldOverlay : public WavetableComponentOverlay {
  public:
    WaveFoldOverlay();

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;

    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;

    void setWaveFoldModifier(WaveFoldModifier* wave_fold_modifier) {
      wave_fold_modifier_ = wave_fold_modifier;
      current_frame_ = nullptr;
    }

  protected:
    WaveFoldModifier* wave_fold_modifier_;
    WaveFoldModifier::WaveFoldModifierKeyframe* current_frame_;

    std::unique_ptr<SynthSlider> wave_fold_amount_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveFoldOverlay)
};

