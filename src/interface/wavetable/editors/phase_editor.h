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

#include "open_gl_multi_quad.h"
#include "common.h"

class PhaseEditor : public OpenGlMultiQuad {
  public:
    static constexpr int kNumLines = 16;
    static constexpr float kDefaultHeightPercent = 0.2f;

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void phaseChanged(float phase, bool mouse_up) = 0;
    };

    PhaseEditor();
    virtual ~PhaseEditor();

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      OpenGlMultiQuad::render(open_gl, animate);
    }

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void updatePhase(const MouseEvent& e);
    void updatePositions();

    void addListener(Listener* listener) { listeners_.push_back(listener); }

    void setPhase(float phase);
    void setMaxTickHeight(float height) { max_tick_height_ = height; }

  private:
    std::vector<Listener*> listeners_;
    Point<int> last_edit_position_;
  
    float phase_;
    float max_tick_height_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseEditor)
};
