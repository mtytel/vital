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

#include "line_generator.h"
#include "open_gl_image.h"
#include "line_editor.h"
#include "synth_lfo.h"
#include "synth_module.h"

class SynthGuiInterface;

class LfoEditor : public LineEditor {
  public:
    static constexpr float kBoostDecay = 0.9f;
    static constexpr float kSpeedDecayMult = 5.0f;

    enum {
      kSetPhaseToPoint = kNumMenuOptions,
      kSetPhaseToPower,
      kSetPhaseToGrid,
      kImportLfo,
      kExportLfo,
    };

    LfoEditor(LineGenerator* lfo_source, String prefix,
              const vital::output_map& mono_modulations, const vital::output_map& poly_modulations);
    virtual ~LfoEditor();

    void parentHierarchyChanged() override;
    virtual void mouseDown(const MouseEvent& e) override;
    virtual void mouseDoubleClick(const MouseEvent& e) override;
    virtual void mouseUp(const MouseEvent& e) override;

    void respondToCallback(int point, int power, int result) override;
    void setPhase(float phase);

    void render(OpenGlWrapper& open_gl, bool animate) override;

  private:
    SynthGuiInterface* parent_;

    const vital::StatusOutput* wave_phase_;
    const vital::StatusOutput* frequency_;
    vital::poly_float last_phase_;
    vital::poly_float last_voice_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LfoEditor)
};

