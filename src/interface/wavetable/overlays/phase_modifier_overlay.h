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

#include "phase_editor.h"
#include "phase_modifier.h"
#include "wavetable_component_overlay.h"
#include "text_selector.h"

class PhaseModifierOverlay : public WavetableComponentOverlay,
                             public PhaseEditor::Listener,
                             TextEditor::Listener {
  public: 
    PhaseModifierOverlay();

    virtual void frameSelected(WavetableKeyframe* keyframe) override;
    virtual void frameDragged(WavetableKeyframe* keyframe, int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds) override;
    virtual bool setTimeDomainBounds(Rectangle<int> bounds) override;

    virtual void textEditorReturnKeyPressed(TextEditor& text_editor) override;
    virtual void textEditorFocusLost(TextEditor& text_editor) override;

    void phaseChanged(float phase, bool mouse_up) override;

    void sliderValueChanged(Slider* moved_slider) override;
    void sliderDragEnded(Slider* moved_slider) override;

    void setPhaseModifier(PhaseModifier* phase_modifier) {
      phase_modifier_ = phase_modifier;
      current_frame_ = nullptr;
    }

  protected:
    void setPhase(String phase_string);

    PhaseModifier* phase_modifier_;
    PhaseModifier::PhaseModifierKeyframe* current_frame_;
    std::unique_ptr<PhaseEditor> editor_;
    std::unique_ptr<PhaseEditor> slider_;
    std::unique_ptr<TextEditor> phase_text_;
    std::unique_ptr<TextSelector> phase_style_;
    std::unique_ptr<SynthSlider> mix_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseModifierOverlay)
};

