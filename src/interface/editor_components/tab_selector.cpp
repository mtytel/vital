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

#include "tab_selector.h"

#include "fonts.h"
#include "skin.h"

TabSelector::TabSelector(String name) : Slider(name), font_height_percent_(kDefaultFontHeightPercent), active_(true) {
  image_component_.setComponent(this);
  image_component_.setScissor(true);

  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  setColour(Slider::backgroundColourId, Colour(0xff303030));
  setColour(Slider::textBoxOutlineColourId, Colour(0x00000000));
  setRange(0, 1, 1);
}

void TabSelector::paint(Graphics& g) {
  static constexpr float kLightHeightPercent = 0.08f;
  int selected = getValue();
  int num_types = getMaximum() - getMinimum() + 1;
  int from_highlight = getTabX(selected);
  int to_highlight = getTabX(selected + 1);
  int light_height = std::max<int>(getHeight() * kLightHeightPercent, 1);
  
  Colour highlight_color = findColour(Skin::kWidgetPrimary1, true);
  if (!active_)
    highlight_color = highlight_color.withSaturation(0.0f);

  g.setColour(findColour(Skin::kLightenScreen, true));
  g.fillRect(0, 0, getWidth(), light_height);

  g.setColour(highlight_color);
  g.fillRect(from_highlight, 0, to_highlight - from_highlight, light_height);

  g.setFont(Fonts::instance()->proportional_light().withPointHeight(getHeight() * font_height_percent_));
  for (int i = 0; i < num_types && i < names_.size(); ++i) {
    std::string name = names_[i];
    int from_x = getTabX(i);
    int to_x = getTabX(i + 1);
    if (i == selected)
      g.setColour(highlight_color);
    else
      g.setColour(findColour(Skin::kTextComponentText, true));

    g.drawText(name, from_x, 0, to_x - from_x, getHeight(), Justification::centred);
  }
}

void TabSelector::mouseEvent(const juce::MouseEvent &e) {
  float x = e.getPosition().getX();
  int index = x * (getMaximum() + 1) / getWidth();
  setValue(index);
}

void TabSelector::mouseDown(const juce::MouseEvent &e) {
  mouseEvent(e);
}

void TabSelector::mouseDrag(const juce::MouseEvent &e) {
  mouseEvent(e);
}

void TabSelector::mouseUp(const juce::MouseEvent &e) {
  mouseEvent(e);
}

int TabSelector::getTabX(int position) {
  int num_types = getMaximum() - getMinimum() + 1;
  return std::round(float((getWidth() + 1) * position) / num_types);
}

