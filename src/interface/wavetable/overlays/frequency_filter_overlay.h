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
#include "frequency_filter_modifier.h"

class TextSelector;

class FrequencyFilterOverlay : public WavetableComponentOverlay {
  public: 
    FrequencyFilterOverlay();

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;
    virtual bool setFrequencyAmplitudeBounds(Rectangle<int> bounds) override;

    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;
    void buttonClicked(Button* clicked_button) override;

    void setFilterModifier(FrequencyFilterModifier* frequency_modifier) {
      frequency_modifier_ = frequency_modifier;
      current_frame_ = nullptr;
    }

  protected:
    FrequencyFilterModifier* frequency_modifier_;
    FrequencyFilterModifier::FrequencyFilterModifierKeyframe* current_frame_;

    std::unique_ptr<SynthSlider> cutoff_;
    std::unique_ptr<SynthSlider> shape_;
    std::unique_ptr<OpenGlToggleButton> normalize_;
    std::unique_ptr<TextSelector> style_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyFilterOverlay)
};

