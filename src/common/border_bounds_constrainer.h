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

class FullInterface;

class BorderBoundsConstrainer : public ComponentBoundsConstrainer {
  public:
    BorderBoundsConstrainer() : ComponentBoundsConstrainer(), gui_(nullptr) { }

    virtual void checkBounds(Rectangle<int>& bounds, const Rectangle<int>& previous,
                             const Rectangle<int>& limits,
                             bool stretching_top, bool stretching_left,
                             bool stretching_bottom, bool stretching_right) override;

    virtual void resizeStart() override;
    virtual void resizeEnd() override;

    void setBorder(const BorderSize<int>& border) { border_ = border; }
    void setGui(FullInterface* gui) { gui_ = gui; }

  protected:
    FullInterface* gui_;
    BorderSize<int> border_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BorderBoundsConstrainer)
};

