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

#include "wavetable_component_overlay.h"

#include "wavetable_overlay_factory.h"
#include "fonts.h"
#include "skin.h"

void WavetableComponentOverlay::ControlsBackground::setPositions() {
  static constexpr float kTextHeightRatio = 0.6f;
  static constexpr int kMaxLines = WavetableComponentOverlay::ControlsBackground::kMaxLines;

  if (findParentComponentOfClass<SynthGuiInterface>() == nullptr)
    return;

  background_.setColor(findColour(Skin::kBody, true));
  border_.setColor(findColour(Skin::kWidgetPrimary1, true));
  Colour lighten = findColour(Skin::kLightenScreen, true);
  Colour text = findColour(Skin::kBodyText, true);
  lines_.setColor(lighten);
  title_backgrounds_.setColor(lighten);
  float width = getWidth();
  float line_width = 2.0f / width;
  for (int i = 0; i < line_positions_.size() && i < kMaxLines; ++i)
    lines_.setQuad(i, line_positions_[i] * 2.0f / width - 1.0f, -1.0f, line_width, 2.0f);

  lines_.setNumQuads(std::min<int>(static_cast<int>(line_positions_.size()), kMaxLines));

  int num_titles = static_cast<int>(titles_.size());
  int num_positions = static_cast<int>(line_positions_.size());
  int title_height = getHeight() * WavetableComponentOverlay::kTitleHeightRatio;
  float gl_title_height = title_height * 2.0f / getHeight();

  for (int i = 0; i < num_titles && i - 1 < num_positions; ++i) {
    std::string title = titles_[i];
    title_texts_[i]->setColor(text);
    title_texts_[i]->setTextSize(title_height * kTextHeightRatio);
    title_texts_[i]->setText(title);
    title_texts_[i]->setActive(true);

    int start_position = 0;
    int end_position = getWidth();
    if (i > 0)
      start_position = line_positions_[i - 1];
    if (i < line_positions_.size())
      end_position = line_positions_[i];

    title_texts_[i]->setBounds(start_position, 0, end_position - start_position, title_height);
    if (title.empty())
      title_backgrounds_.setQuad(i, -2.0f, -2.0f, 0.0f, 0.0f);
    else {
      title_backgrounds_.setQuad(i, start_position * 2.0f / getWidth() - 1.0f, 1.0f - gl_title_height,
                                 (end_position - start_position) * 2.0f / getWidth(), gl_title_height);
    }

    title_texts_[i]->redrawImage(true);
  }

  title_backgrounds_.setNumQuads(num_titles);
  for (int i = num_titles; i <= kMaxLines; ++i) {
    title_texts_[i]->setActive(false);
  }
}

void WavetableComponentOverlay::setEditBounds(Rectangle<int> bounds) {
  edit_bounds_ = bounds;
  int x = edit_bounds_.getX() + (edit_bounds_.getWidth() - controls_width_) / 2;
  controls_background_.setBounds(x, edit_bounds_.getY(), controls_width_, edit_bounds_.getHeight());
  controls_background_.repaint();

  repaint();
}

void WavetableComponentOverlay::resetOverlay() {
  current_component_ = nullptr;
  WavetableOverlayFactory::setOverlayOwner(this, nullptr);
}

void WavetableComponentOverlay::setComponent(WavetableComponent* component) {
  current_component_ = component;
  WavetableOverlayFactory::setOverlayOwner(this, component);
}

void WavetableComponentOverlay::notifyChanged(bool mouse_up) {
  if (mouse_up) {
    for (WavetableComponentOverlay::Listener* listener : listeners_)
      listener->frameDoneEditing();
  }
  else {
    for (WavetableComponentOverlay::Listener* listener : listeners_)
      listener->frameChanged();
  }
}

float WavetableComponentOverlay::getTitleHeight() {
  return edit_bounds_.getWidth() * kTitleHeightForWidth;
}

int WavetableComponentOverlay::getDividerX() {
  return edit_bounds_.getX() + kDividerPoint * edit_bounds_.getWidth();
}

int WavetableComponentOverlay::getWidgetHeight() {
  return edit_bounds_.getWidth() * kWidgetHeightForWidth;
}

int WavetableComponentOverlay::getWidgetPadding() {
  return getWidgetHeight() / 2.0f;
}
