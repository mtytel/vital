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

#include "text_selector.h"

#include "default_look_and_feel.h"
#include "lfo_section.h"
#include "fonts.h"
#include "skin.h"

TextSelector::TextSelector(String name) : SynthSlider(name), long_lookup_(nullptr) { }

void TextSelector::mouseDown(const juce::MouseEvent &e) {
  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseDown(e);
    return;
  }

  const std::string* lookup = string_lookup_;
  if (long_lookup_)
    lookup = long_lookup_;

  PopupItems options;
  for (int i = 0; i <= getMaximum(); ++i)
    options.addItem(i, lookup[i]);

  parent_->showPopupSelector(this, Point<int>(0, getHeight()), options, [=](int value) { setValue(value); });
}

void TextSelector::mouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    SynthSlider::mouseUp(e);
    return;
  }
}

void PaintPatternSelector::paint(Graphics& g) {
  static constexpr float kLineWidthHeightRatio = 0.05f;

  std::vector<std::pair<float, float>> pattern = LfoSection::getPaintPattern(getValue());
  int height = getHeight() - 2 * padding_ - 1;
  int width = getWidth() - 2 * padding_ - 1;
  float buffer = padding_ + 0.5f;
  Path path;
  path.startNewSubPath(buffer, buffer + height);
  for (auto& pair : pattern)
    path.lineTo(buffer + pair.first * width, buffer + (1.0f - pair.second) * height);

  path.lineTo(buffer + width, buffer + height);

  if (isActive()) {
    g.setColour(findColour(Skin::kWidgetSecondary1, true));
    g.fillPath(path);
    g.setColour(findColour(Skin::kWidgetSecondary2, true));
    g.fillPath(path);
  }
  else {
    g.setColour(findColour(Skin::kLightenScreen, true));
    g.fillPath(path);
  }

  if (isActive())
    g.setColour(findColour(Skin::kWidgetCenterLine, true));
  else
    g.setColour(findColour(Skin::kLightenScreen, true));

  int line_width = getHeight() * kLineWidthHeightRatio;
  line_width += (line_width + 1) % 2;
  g.strokePath(path, PathStrokeType(line_width, PathStrokeType::curved, PathStrokeType::rounded));
}