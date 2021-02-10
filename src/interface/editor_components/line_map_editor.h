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

class LineMapEditor : public LineEditor {
  public:
    static constexpr float kTailDecay = 0.93f;

    LineMapEditor(LineGenerator* line_source, String name);
    virtual ~LineMapEditor();

    void parentHierarchyChanged() override;

    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    void setAnimate(bool animate) { animate_ = animate; }

  private:
    const vital::StatusOutput* raw_input_;
    bool animate_;
    vital::poly_float last_phase_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LineMapEditor)
};

