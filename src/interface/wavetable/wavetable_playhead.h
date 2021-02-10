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

class WavetablePlayhead : public SynthSection {
  public:
    static constexpr int kBigLineSkip = 16;
    static constexpr int kLineSkip = 4;

    class Listener {
      public:
        virtual ~Listener() { }
      
        virtual void playheadMoved(int new_position) = 0;
    };

    WavetablePlayhead(int num_positions);

    int position() { return position_; }
    void setPosition(int position);
    void setPositionQuad();

    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseEvent(const MouseEvent& event);

    void paintBackground(Graphics& g) override;
    void resized() override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setPadding(float padding) { padding_ = padding; setPositionQuad(); }

  protected:
    OpenGlQuad position_quad_;

    std::vector<Listener*> listeners_;
  
    float padding_;
    int num_positions_;
    int position_;
    int drag_start_x_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetablePlayhead)
};

