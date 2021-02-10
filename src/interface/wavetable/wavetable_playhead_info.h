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

#include "wavetable_playhead.h"

class WavetablePlayheadInfo : public Component, public WavetablePlayhead::Listener {
  public: 
    WavetablePlayheadInfo() : playhead_position_(0) { }

    void paint(Graphics& g) override;
    void resized() override;
    void playheadMoved(int position) override;

  protected:  
    int playhead_position_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetablePlayheadInfo)
};

