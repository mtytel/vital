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

#include "default_look_and_feel.h"
#include "skin.h"
#include "fonts.h"
#include "paths.h"
#include "synth_slider.h"
#include "synth_section.h"

DefaultLookAndFeel::DefaultLookAndFeel() {
  setColour(PopupMenu::backgroundColourId, Colour(0xff111111));
  setColour(PopupMenu::textColourId, Colour(0xffcccccc));
  setColour(PopupMenu::headerTextColourId, Colour(0xffffffff));
  setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xff8458b7));
  setColour(PopupMenu::highlightedTextColourId, Colour(0xffffffff));
  setColour(BubbleComponent::backgroundColourId, Colour(0xff111111));
  setColour(BubbleComponent::outlineColourId, Colour(0xff333333));
  setColour(TooltipWindow::textColourId, Colour(0xffdddddd));
}

void DefaultLookAndFeel::fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& text_editor) {
  if (width <= 0 || height <= 0)
    return;

  float rounding = 5.0f;
  SynthSection* parent = text_editor.findParentComponentOfClass<SynthSection>();
  if (parent)
    rounding = parent->findValue(Skin::kWidgetRoundedCorner);

  g.setColour(text_editor.findColour(Skin::kTextEditorBackground, true));
  g.fillRoundedRectangle(0, 0, width, height, rounding);
  g.setColour(text_editor.findColour(Skin::kTextEditorBorder, true));
  g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, rounding, 1.0f);
}

void DefaultLookAndFeel::drawPopupMenuBackground(Graphics& g, int width, int height) {
  g.setColour(findColour(PopupMenu::backgroundColourId));
  g.fillRoundedRectangle(0, 0, width, height, kPopupMenuBorder);
  g.setColour(findColour(BubbleComponent::outlineColourId));
  g.drawRoundedRectangle(0.5f, 0.5f, width - 1.0f, height - 1.0f, kPopupMenuBorder, 1.0f);
}

void DefaultLookAndFeel::drawScrollbar(Graphics& g, ScrollBar& scroll_bar, int x, int y, int width, int height,
                                       bool vertical, int thumb_position, int thumb_size,
                                       bool mouse_over, bool mouse_down) {
  if (thumb_size >= height)
    return;

  bool right_aligned = dynamic_cast<LeftAlignedScrollBar*>(&scroll_bar) == nullptr;

  int draw_width = width / 2 - 2;
  if (mouse_down || mouse_over)
    draw_width = width - 2;

  int draw_times = 2;
  if (mouse_down)
    draw_times = 4;

  int draw_x = 1;
  if (right_aligned)
    draw_x = width - 1 - draw_width;

  g.setColour(scroll_bar.findColour(Skin::kLightenScreen, true));
  for (int i = 0 ; i < draw_times; ++i)
    g.fillRoundedRectangle(draw_x, thumb_position, draw_width, thumb_size, draw_width / 2.0f);
}

void DefaultLookAndFeel::drawComboBox(Graphics& g, int width, int height, const bool button_down,
                                      int button_x, int button_y, int button_w, int button_h, ComboBox& box) {
  static constexpr float kRoundness = 4.0f;
  g.setColour(findColour(BubbleComponent::backgroundColourId));
  g.fillRoundedRectangle(box.getLocalBounds().toFloat(), kRoundness);
  Path path = Paths::downTriangle();

  g.setColour(box.findColour(Skin::kTextComponentText, true));
  Rectangle<int> arrow_bounds = box.getLocalBounds().removeFromRight(height);
  g.fillPath(path, path.getTransformToScaleToFit(arrow_bounds.toFloat(), true));
}

void DefaultLookAndFeel::drawTickBox(Graphics& g, Component& component,
                                     float x, float y, float w, float h, bool ticked,
                                     bool enabled, bool mouse_over, bool button_down) {
  static constexpr float kBorderPercent = 0.15f;
  if (ticked)
    g.setColour(component.findColour(Skin::kIconButtonOn, true));
  else
    g.setColour(component.findColour(Skin::kLightenScreen, true));

  float border_width = h * kBorderPercent;
  g.fillRect(x + border_width, y + border_width, w - 2 * border_width, h - 2 * border_width);
}

void DefaultLookAndFeel::drawCallOutBoxBackground(CallOutBox& call_out_box, Graphics& g, const Path& path, Image&) {
  g.setColour(call_out_box.findColour(Skin::kBody, true));
  g.fillPath(path);

  g.setColour(call_out_box.findColour(Skin::kPopupBorder, true));
  g.strokePath(path, PathStrokeType(1.0f));
}

void DefaultLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& background_color,
                                              bool hover, bool down) {
  g.setColour(button.findColour(Skin::kPopupSelectorBackground, true));
  g.fillRoundedRectangle(button.getLocalBounds().toFloat(), 5.0f);
}

int DefaultLookAndFeel::getSliderPopupPlacement(Slider& slider) {
  SynthSlider* s_slider = dynamic_cast<SynthSlider*>(&slider);
  if (s_slider)
    return s_slider->getPopupPlacement();

  return LookAndFeel_V3::getSliderPopupPlacement(slider);
}

Font DefaultLookAndFeel::getPopupMenuFont() {
  return Fonts::instance()->proportional_regular().withPointHeight(14.0f);
}

Font DefaultLookAndFeel::getSliderPopupFont(Slider& slider) {
  return Fonts::instance()->proportional_regular().withPointHeight(14.0f);
}
