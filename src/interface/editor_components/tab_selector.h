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

#include "open_gl_image_component.h"

class TabSelector : public Slider {
  public:
    static constexpr float kDefaultFontHeightPercent = 0.26f;
    TabSelector(String name);

    void paint(Graphics& g) override;

    void mouseEvent(const MouseEvent& e);
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void setNames(std::vector<std::string> names) { names_ = names; }
    void setFontHeightPercent(float percent) { font_height_percent_ = percent; }
    float getFontHeightPercent() { return font_height_percent_; }

    void setActive(bool active) { active_ = active; }

    virtual void valueChanged() override {
      Slider::valueChanged();
      redoImage();
    }

    OpenGlImageComponent* getImageComponent() { return &image_component_; }
    void redoImage() { image_component_.redrawImage(true); }

  private:
    int getTabX(int position);

    OpenGlImageComponent image_component_;

    float font_height_percent_;
    bool active_;
    std::vector<std::string> names_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabSelector)
};

