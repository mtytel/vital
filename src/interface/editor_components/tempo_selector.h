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
#include "synth_slider.h"

class TempoSelector : public SynthSlider {
  public:
    enum MenuId {
      kSeconds,
      kTempo,
      kTempoDotted,
      kTempoTriplet,
      kKeytrack
    };

    TempoSelector(String name);

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void paint(Graphics& g) override;
    void valueChanged() override;

    void setFreeSlider(Slider* slider);
    void setTempoSlider(Slider* slider);
    void setKeytrackTransposeSlider(Slider* slider);
    void setKeytrackTuneSlider(Slider* slider);

    bool isKeytrack() const { return getValue() + 1 == kKeytrack; }

  private:
    Slider* free_slider_;
    Slider* tempo_slider_;
    Slider* keytrack_transpose_slider_;
    Slider* keytrack_tune_slider_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoSelector)
};

