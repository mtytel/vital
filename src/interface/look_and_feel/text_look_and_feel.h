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
#include "default_look_and_feel.h"

class TextLookAndFeel : public DefaultLookAndFeel {
  public:
    virtual ~TextLookAndFeel() { }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float slider_t, float start_angle, float end_angle,
                          Slider& slider) override;

    void drawToggleButton(Graphics& g, ToggleButton& button, bool hover, bool is_down) override;

    void drawTickBox(Graphics& g, Component& component, float x, float y, float w, float h, bool ticked,
                     bool enabled, bool mouse_over, bool button_down) override;

    void drawLabel(Graphics& g, Label& label) override;

    void drawComboBox(Graphics& g, int width, int height, bool is_down,
                      int button_x, int button_y, int button_w, int button_h, ComboBox& box) override;

    static TextLookAndFeel* instance() {
      static TextLookAndFeel instance;
      return &instance;
    }

  private:
    TextLookAndFeel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextLookAndFeel)
};

