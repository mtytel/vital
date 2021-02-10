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
#include "wave_window_editor.h"
#include "wave_window_modifier.h"

class TextSelector;

class WaveWindowOverlay : public WavetableComponentOverlay, WaveWindowEditor::Listener {
  public:
    WaveWindowOverlay();
    
    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;
    virtual bool setTimeDomainBounds(Rectangle<int> bounds) override;

    void windowChanged(bool left, bool mouse_up) override;
    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;

    void setWaveWindowModifier(WaveWindowModifier* wave_window_modifier) {
      wave_window_modifier_ = wave_window_modifier;
      current_frame_ = nullptr;
    }

  protected:
    WaveWindowModifier* wave_window_modifier_;
    WaveWindowModifier::WaveWindowModifierKeyframe* current_frame_;
    std::unique_ptr<WaveWindowEditor> editor_;
    std::unique_ptr<TextSelector> window_shape_;
    std::unique_ptr<SynthSlider> left_position_;
    std::unique_ptr<SynthSlider> right_position_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveWindowOverlay)
};

