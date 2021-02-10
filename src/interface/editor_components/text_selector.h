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

class TextSelector : public SynthSlider {
  public:
    TextSelector(String name);

    virtual void mouseDown(const MouseEvent& e) override;
    virtual void mouseUp(const MouseEvent& e) override;
    bool shouldShowPopup() override { return false; }

    void setLongStringLookup(const std::string* lookup) { long_lookup_ = lookup; }

  protected:
    const std::string* long_lookup_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextSelector)
};

class PaintPatternSelector : public TextSelector {
  public:
    PaintPatternSelector(String name) : TextSelector(name), padding_(0) { }

    void paint(Graphics& g) override;

    void setPadding(int padding) { padding_ = padding;   }

  private:
    int padding_;
};

