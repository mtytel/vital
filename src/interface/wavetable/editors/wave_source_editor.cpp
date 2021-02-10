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

#include "wave_source_editor.h"

#include "skin.h"
#include "default_look_and_feel.h"
#include "synth_constants.h"
#include "shaders.h"
#include "utils.h"

namespace {
  void waveSourceCallback(int result, WaveSourceEditor* editor) {
    if (editor == nullptr)
      return;

    if (result == WaveSourceEditor::kClear)
      editor->clear();
    else if (result == WaveSourceEditor::kFlipHorizontal)
      editor->flipHorizontal();
    else if (result == WaveSourceEditor::kFlipVertical)
      editor->flipVertical();
  }
}

WaveSourceEditor::WaveSourceEditor(int size) :
    OpenGlLineRenderer(size, true),
    grid_lines_(2 * WavetableComponentOverlay::kMaxGrid, Shaders::kColorFragment),
    grid_circles_(kNumCircles, Shaders::kCircleFragment), hover_circle_(Shaders::kCircleFragment), editing_line_(2) {
  grid_lines_.setTargetComponent(this);
  grid_circles_.setTargetComponent(this);
  hover_circle_.setTargetComponent(this);
  hover_circle_.setQuad(0, -2.0f, -2.0f, 0.0f, 0.0f);
  addAndMakeVisible(&editing_line_);
  editing_line_.setInterceptsMouseClicks(false, false);

  setEditable(false);
  setFit(true);
  horizontal_grid_ = 0;
  vertical_grid_ = 0;
  editing_ = false;
  dragging_audio_file_ = false;

  values_ = std::make_unique<float[]>(size);

  current_mouse_position_ = Point<int>(INT_MAX / 2, INT_MAX / 2);
}

WaveSourceEditor::~WaveSourceEditor() {
}

void WaveSourceEditor::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kWidgetBackground, true));

  Colour lighten = findColour(Skin::kLightenScreen, true);
  grid_lines_.setColor(lighten);
  grid_circles_.setColor(lighten.withMultipliedAlpha(0.5f));
  hover_circle_.setColor(lighten);
  editing_line_.setColor(lighten);
}

void WaveSourceEditor::resized() {
  float width = getWidth();
  float line_width = findValue(Skin::kWidgetLineWidth);
  setLineWidth(line_width);
  editing_line_.setLineWidth(line_width);

  int num_points = numPoints();
  for (int i = 0; i < numPoints(); ++i)
    setXAt(i, i * width / (num_points - 1.0f));

  setLineValues();

  OpenGlLineRenderer::resized();
  editing_line_.setBounds(getLocalBounds());
  setGridPositions();
}

void WaveSourceEditor::mouseDown(const MouseEvent& e) {
  current_mouse_position_ = e.getPosition();
  if (e.mods.isPopupMenu()) {
    PopupItems options;
    options.addItem(kClear, "Clear");
    options.addItem(kFlipVertical, "Flip Vertical");
    options.addItem(kFlipHorizontal, "Flip Horizontal");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    parent->showPopupSelector(this, e.getPosition(), options,
                              [=](int selection) { waveSourceCallback(selection, this); });
  }
  else {
    last_edit_position_ = getSnappedPoint(current_mouse_position_);
    setHoverPosition();
    changeValues(e);
    editing_line_.setXAt(0, last_edit_position_.x);
    editing_line_.setYAt(0, last_edit_position_.y);
    editing_line_.setXAt(1, current_mouse_position_.x);
    editing_line_.setYAt(1, current_mouse_position_.y);
    editing_ = true;
  }
}

void WaveSourceEditor::mouseUp(const MouseEvent& e) {
  editing_ = false;
  current_mouse_position_ = e.getPosition();
  int index = getHoveredIndex(snapToGrid(current_mouse_position_));

  for (Listener* listener : listeners_)
    listener->valuesChanged(index, index, true);
}

void WaveSourceEditor::mouseMove(const MouseEvent& e) {
  current_mouse_position_ = e.getPosition();
  setHoverPosition();
}

void WaveSourceEditor::mouseDrag(const MouseEvent& e) {
  current_mouse_position_ = e.getPosition();
  changeValues(e);
  last_edit_position_ = snapToGrid(current_mouse_position_);
  setHoverPosition();
  editing_line_.setXAt(0, last_edit_position_.x);
  editing_line_.setYAt(0, last_edit_position_.y);
  editing_line_.setXAt(1, current_mouse_position_.x);
  editing_line_.setYAt(1, current_mouse_position_.y);
}

void WaveSourceEditor::mouseExit(const MouseEvent& e) {
  current_mouse_position_ = Point<int>(-getWidth(), 0);
  setHoverPosition();
}

int WaveSourceEditor::getHoveredIndex(Point<int> position) {
  int index = floorf(numPoints() * (1.0f * position.x) / getWidth());
  return vital::utils::iclamp(index, 0, numPoints() - 1);
}

float WaveSourceEditor::getSnapRadius() {
  static float kGridProximity = 0.2f;

  if (horizontal_grid_ == 0 || vertical_grid_ == 0)
    return 0.0f;

  float grid_cell_width = getWidth() / (1.0f * horizontal_grid_);
  float grid_cell_height = getHeight() / (1.0f * vertical_grid_);
  return kGridProximity * std::min(grid_cell_height, grid_cell_width);
}

void WaveSourceEditor::setLineValues() {
  float adjust = getHeight() / 2.0f;
  for (int i = 0; i < numPoints(); ++i)
    setYAt(i, adjust * (1.0f - values_[i]));
}

void WaveSourceEditor::changeValues(const MouseEvent& e) {
  Point<int> mouse_position = snapToGrid(e.getPosition());
  int from_index = getHoveredIndex(last_edit_position_);
  int selected_index = getHoveredIndex(mouse_position);

  float x = mouse_position.x;
  float y = mouse_position.y;
  float x_delta = last_edit_position_.x - x;
  float y_delta = last_edit_position_.y - y;
  float slope = y_delta == 0 ? 0 : y_delta / x_delta;
  float next_x = getWidth() * (1.0f * selected_index) / numPoints();
  int direction = -1;

  if (selected_index < from_index) {
    direction = 1;
    next_x += getWidth() * 1.0f / numPoints();
  }
  float inc_x = next_x - x;

  for (int index = selected_index; index != from_index + direction; index += direction) {
    if (index >= 0 && index < numPoints()) {
      float new_value = -2.0f * y / getHeight() + 1.0f;
      values_[index] = vital::utils::clamp(new_value, -1.0f, 1.0f);
    }

    y += inc_x * slope;
    inc_x = direction * getWidth() * 1.0f / numPoints();
  }

  setLineValues();

  int min_index = std::min(from_index, selected_index);
  int max_index = std::max(from_index, selected_index);

  for (Listener* listener : listeners_)
    listener->valuesChanged(min_index, max_index, false);
}

Point<int> WaveSourceEditor::getSnappedPoint(Point<int> input) {
  if (horizontal_grid_ == 0 || vertical_grid_ == 0)
    return input;

  float x = horizontal_grid_ * (1.0f * input.x) / getWidth();
  float y = vertical_grid_ * (1.0f * input.y) / getHeight();
  float snapped_x = getWidth() * std::roundf(x) / horizontal_grid_;
  float snapped_y = getHeight() * std::roundf(y) / vertical_grid_;
  return Point<int>(snapped_x, snapped_y);
}

Point<int> WaveSourceEditor::snapToGrid(Point<int> input) {
  Point<int> snapped = getSnappedPoint(input);

  if (input.getDistanceFrom(snapped) > getSnapRadius())
    return last_edit_position_;

  return snapped;
}

void WaveSourceEditor::loadWaveform(float* waveform) {
  for (int i = 0; i < numPoints(); ++i)
    values_[i] = waveform[i];

  setLineValues();
}

void WaveSourceEditor::setEditable(bool editable) {
  setInterceptsMouseClicks(editable, editable);
  editable_ = editable;
}

void WaveSourceEditor::setGrid(int horizontal, int vertical) {
  horizontal_grid_ = horizontal;
  vertical_grid_ =  vertical;
  setGridPositions();
}

void WaveSourceEditor::setGridPositions() {
  float pixel_height = 2.0f / getHeight();
  float pixel_width = 2.0f / getWidth();

  int grid_index = 0;
  for (int i = 1; i < horizontal_grid_; ++i) {
    grid_lines_.setQuad(grid_index, i * 2.0f / horizontal_grid_ - 1.0f, -1.0f, pixel_width, 2.0f);
    grid_index++;
  }

  for (int i = 1; i < vertical_grid_; ++i) {
    grid_lines_.setQuad(grid_index, -1.0f, i * 2.0f / vertical_grid_ - 1.0f, 2.0f, pixel_height);
    grid_index++;
  }
  grid_lines_.setNumQuads(grid_index);

  int circle_index = 0;
  float circle_radius_x = getSnapRadius() * 2.0f / getWidth();
  float circle_radius_y = getSnapRadius() * 2.0f / getHeight();
  for (int h = 0; h <= horizontal_grid_; ++h) {
    for (int v = 0; v <= vertical_grid_; ++v) {
      float x = h * 2.0f / horizontal_grid_ - 1.0f;
      float y = v * 2.0f / vertical_grid_ - 1.0f;

      grid_circles_.setQuad(circle_index, x - circle_radius_x, y - circle_radius_y,
                            circle_radius_x * 2.0f, circle_radius_y * 2.0f);
      circle_index++;
    }
  }

  grid_circles_.setNumQuads(circle_index);
}


void WaveSourceEditor::setHoverPosition() {
  float circle_radius_x = getSnapRadius() * 2.0f / getWidth();
  float circle_radius_y = getSnapRadius() * 2.0f / getHeight();

  Point<int> position;
  if (editing_)
    position = last_edit_position_;
  else
    position = getSnappedPoint(current_mouse_position_);
  hover_circle_.setQuad(0, position.x * 2.0f / getWidth() - 1.0f - circle_radius_x,
                           1.0f - position.y * 2.0f / getHeight() - circle_radius_y,
                           2.0f * circle_radius_x, 2.0f * circle_radius_y);
}

void WaveSourceEditor::audioFileLoaded(const File& file) {
  dragging_audio_file_ = false;
}

void WaveSourceEditor::fileDragEnter(const StringArray& files, int x, int y) {
  dragging_audio_file_ = true;
}

void WaveSourceEditor::fileDragExit(const StringArray& files) {
  dragging_audio_file_ = false;
}

void WaveSourceEditor::clear() {
  for (int index = 0; index < numPoints(); ++index)
    values_[index] = 0.0f;

  setLineValues();

  for (Listener* listener : listeners_)
    listener->valuesChanged(0, numPoints() - 1, true);
}

void WaveSourceEditor::flipVertical() {
  for (int index = 0; index < numPoints(); ++index)
    values_[index] = -values_[index];

  setLineValues();

  for (Listener* listener : listeners_)
    listener->valuesChanged(0, numPoints() - 1, true);
}

void WaveSourceEditor::flipHorizontal() {
  for (int index = 0; index < numPoints() / 2; ++index) {
    int other_index = numPoints() - index - 1;
    float other_value = values_[other_index];
    values_[other_index] = values_[index];
    values_[index] = other_value;
  }

  setLineValues();

  for (Listener* listener : listeners_)
    listener->valuesChanged(0, numPoints() - 1, true);
}
