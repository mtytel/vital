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

#include "phase_editor.h"

#include "skin.h"
#include "synth_constants.h"
#include "shaders.h"
#include "utils.h"

PhaseEditor::PhaseEditor() : OpenGlMultiQuad(kNumLines, Shaders::kColorFragment) {
  phase_ = 0.0f;
  max_tick_height_ = kDefaultHeightPercent;
  setInterceptsMouseClicks(true, false);
}

PhaseEditor::~PhaseEditor() { }

void PhaseEditor::mouseDown(const MouseEvent& e) {
  last_edit_position_ = e.getPosition();
}

void PhaseEditor::mouseUp(const MouseEvent& e) {
  updatePhase(e);

  for (Listener* listener : listeners_)
    listener->phaseChanged(phase_, true);
}

void PhaseEditor::mouseDrag(const MouseEvent& e) {
  updatePhase(e);
}

void PhaseEditor::updatePhase(const MouseEvent& e) {
  int difference = e.getPosition().x - last_edit_position_.x;
  phase_ += (2.0f * vital::kPi * difference) / getWidth();
  last_edit_position_ = e.getPosition();

  for (Listener* listener : listeners_)
    listener->phaseChanged(phase_, false);

  updatePositions();
}

void PhaseEditor::updatePositions() {
  float width = 2.0f / getWidth();
  for (int i = 0; i < kNumLines; ++i) {
    float phase = phase_ / (2.0f * vital::kPi) + (1.0f * i) / kNumLines;
    phase -= floorf(phase);

    float height = max_tick_height_ * 2.0f;
    for (int div = 2; div < kNumLines; div *= 2) {
      if (i % div)
        height /= 2.0f;
    }
    setQuad(i, 2.0f * phase - 1.0f, -1.0f, width, height);
  }
}

void PhaseEditor::setPhase(float phase) {
  phase_ = phase;
  updatePositions();
}
