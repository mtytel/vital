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

#include "bar_editor.h"

#include "default_look_and_feel.h"
#include "utils.h"

namespace {
  void barEditorCallback(int result, BarEditor* bar_editor) {
    if (bar_editor == nullptr)
      return;

    if (result == BarEditor::kClear)
      bar_editor->clear();
    else if (result == BarEditor::kClearRight)
      bar_editor->clearRight();
    else if (result == BarEditor::kClearLeft)
      bar_editor->clearLeft();
    else if (result == BarEditor::kClearEven)
      bar_editor->clearEven();
    else if (result == BarEditor::kClearOdd)
      bar_editor->clearOdd();
    else if (result == BarEditor::kRandomize)
      bar_editor->randomize();
  }
}

void BarEditor::mouseMove(const MouseEvent& e) {
  current_mouse_position_ = e.getPosition();
}

void BarEditor::mouseDown(const MouseEvent& e) {
  current_mouse_position_ = e.getPosition();
  last_edit_position_ = current_mouse_position_;

  if (e.mods.isPopupMenu()) {
    PopupItems options;
    options.addItem(kClear, "Clear");
    options.addItem(kClearLeft, "Clear Left");
    options.addItem(kClearRight, "Clear Right");
    options.addItem(kClearOdd, "Clear Odd");
    options.addItem(kClearEven, "Clear Even");
    options.addItem(kRandomize, "Randomize");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    parent->showPopupSelector(this, e.getPosition(), options,
                              [=](int selection) { barEditorCallback(selection, this); });
  }
  else {
    changeValues(e);
    editing_ = true;
  }
}

void BarEditor::mouseUp(const MouseEvent& e) {
  editing_ = false;
  current_mouse_position_ = e.getPosition();

  if (!e.mods.isPopupMenu()) {
    int index = getHoveredIndex(current_mouse_position_);

    for (Listener* listener : listeners_)
      listener->barsChanged(index, index, true);
  }
}

void BarEditor::mouseDrag(const MouseEvent& e) {
  current_mouse_position_ = e.getPosition();
  if (!e.mods.isPopupMenu()) {
    changeValues(e);
    last_edit_position_ = current_mouse_position_;
  }
}

void BarEditor::mouseExit(const MouseEvent& e) {
  current_mouse_position_ = Point<int>(-10, -10);
}

void BarEditor::randomize() {
  setY(0, -1.0f);
  for (int i = 1; i < num_points_; ++i)
    setY(i, random_generator_.next());

  for (Listener* listener : listeners_)
    listener->barsChanged(0, num_points_ - 1, true);
}

void BarEditor::clear() {
  for (int i = 0; i < num_points_; ++i)
    setY(i, clear_value_);

  for (Listener* listener : listeners_)
    listener->barsChanged(0, num_points_ - 1, true);
}

void BarEditor::clearRight() {
  int position = getHoveredIndex(last_edit_position_);
  for (int i = position + 1; i < num_points_; ++i)
    setY(i, clear_value_);

  for (Listener* listener : listeners_)
    listener->barsChanged(position + 1, num_points_ - 1, true);
}

void BarEditor::clearLeft() {
  int position = getHoveredIndex(last_edit_position_);
  for (int i = 0; i < position; ++i)
    setY(i, clear_value_);

  for (Listener* listener : listeners_)
    listener->barsChanged(0, position - 1, true);
}

void BarEditor::clearEven() {
  for (int i = 0; i < num_points_; i += 2)
    setY(i, clear_value_);

  for (Listener* listener : listeners_)
    listener->barsChanged(0, num_points_ - 1, true);
}

void BarEditor::clearOdd() {
  for (int i = 1; i < num_points_; i += 2)
    setY(i, clear_value_);

  for (Listener* listener : listeners_)
    listener->barsChanged(0, num_points_ - 1, true);
}

void BarEditor::changeValues(const MouseEvent& e) {
  Point<int> mouse_position = e.getPosition();

  int from_index = getHoveredIndex(last_edit_position_);
  int selected_index = getHoveredIndex(mouse_position);

  float x = mouse_position.x;
  float y = mouse_position.y;
  float x_delta = last_edit_position_.x - x;
  float y_delta = last_edit_position_.y - y;
  float slope = y_delta == 0 ? 0 : y_delta / x_delta;
  float next_x = getWidth() * (scale_ * selected_index) / num_points_;
  int direction = -1;

  if (selected_index < from_index) {
    direction = 1;
    next_x += getWidth() * scale_ / num_points_;
  }
  float inc_x = next_x - x;

  for (int index = selected_index; index != from_index + direction; index += direction) {
    if (index >= 0 && index < num_points_) {
      float new_value = -2.0f * y / getHeight() + 1.0f;
      setY(index, vital::utils::clamp(new_value, -1.0f, 1.0f));
    }

    y += inc_x * slope;
    inc_x = direction * scale_ * getWidth() * 1.0f / num_points_;
  }

  int min_index = std::min(from_index, selected_index);
  int max_index = std::max(from_index, selected_index);

  for (Listener* listener : listeners_)
    listener->barsChanged(min_index, max_index, false);

  dirty_ = true;
}

int BarEditor::getHoveredIndex(Point<int> position) {
  int index = floorf(num_points_ * (1.0f * position.x) / getWidth() / scale_);
  return vital::utils::iclamp(index, 0, num_points_ - 1);
}
