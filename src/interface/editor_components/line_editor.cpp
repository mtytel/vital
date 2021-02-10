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

#include "line_editor.h"

#include "default_look_and_feel.h"
#include "full_interface.h"
#include "fonts.h"
#include "skin.h"
#include "shaders.h"
#include "synth_gui_interface.h"
#include "utils.h"

namespace {
  Point<float> pairToPoint(std::pair<float, float> pair) {
    return { pair.first, pair.second };
  }

  std::pair<float, float> pointToPair(Point<float> point) {
    return { point.x, point.y };
  }
} // namespace

LineEditor::LineEditor(LineGenerator* line_source) :
    OpenGlLineRenderer(kTotalPoints),
    drag_circle_(Shaders::kCircleFragment),
    hover_circle_(Shaders::kRingFragment),
    grid_lines_(kMaxGridSizeX + kMaxGridSizeY + 1),
    position_circle_(Shaders::kRingFragment),
    point_circles_(LineGenerator::kMaxPoints, Shaders::kRingFragment),
    power_circles_(LineGenerator::kMaxPoints, Shaders::kCircleFragment) {
  addAndMakeVisible(drag_circle_);
  addAndMakeVisible(hover_circle_);
  addAndMakeVisible(grid_lines_);
  addAndMakeVisible(position_circle_);
  addAndMakeVisible(point_circles_);
  addAndMakeVisible(power_circles_);

#if !defined(NO_TEXT_ENTRY)
  value_entry_ = std::make_unique<OpenGlTextEditor>("text_entry");
  value_entry_->setMonospace();
  value_entry_->setMultiLine(false);
  value_entry_->setScrollToShowCursor(false);
  value_entry_->addListener(this);
  value_entry_->setSelectAllWhenFocused(true);
  value_entry_->setKeyboardType(TextEditor::numericKeyboard);
  value_entry_->setJustification(Justification::centred);
  addChildComponent(value_entry_.get());
  value_entry_->setAlwaysOnTop(true);
  value_entry_->getImageComponent()->setAlwaysOnTop(true);
  value_entry_->setVisible(false);
#endif

  hover_circle_.setThickness(1.0f);

  active_ = true;
  model_ = line_source;
  size_ratio_ = 1.0f;
  loop_ = true;
  grid_size_x_ = 1;
  grid_size_y_ = 1;
  paint_ = false;
  temporary_paint_toggle_ = false;
  entering_phase_ = false;
  entering_index_ = -1;
  last_model_render_ = -1;
  paint_pattern_ = {
    std::pair<float, float>(0.0f, 1.0f),
    std::pair<float, float>(1.0f, 0.0f)
  };

  last_phase_ = 0.0f;

  setFill(true);
  setFillCenter(-1.0f);

  active_point_ = -1;
  active_power_ = -1;
  active_grid_section_ = -1;
  last_voice_ = -1.0f;
  last_last_voice_ = -1.0f;
  dragging_ = false;
  reset_positions_ = true;
  allow_file_loading_ = true;
  drag_circle_.setActive(false);
  hover_circle_.setActive(false);
  setWantsKeyboardFocus(true);
}

LineEditor::~LineEditor() { }

int LineEditor::getHoverPoint(Point<float> position) {
  position.x = unpadX(position.x);
  position.y = unpadY(position.y);

  float grab_radius = kGrabRadius * size_ratio_;
  float min_distance_squared = grab_radius * grab_radius;
  int hover_point = -1;

  int num_points = model_->getNumPoints();
  for (int i = 0; i < num_points; ++i) {
    std::pair<float, float> point_pair = model_->getPoint(i);
    Point<float> point_position(point_pair.first, point_pair.second);
    point_position.x *= getWidth();
    point_position.y *= getHeight();
    float distance_squared = position.getDistanceSquaredFrom(point_position);
    if (distance_squared < min_distance_squared) {
      min_distance_squared = distance_squared;
      hover_point = i;
    }
  }

  return hover_point;
}

int LineEditor::getHoverPower(Point<float> position) {
  position.x = unpadX(position.x);
  position.y = unpadY(position.y);

  float grab_radius = kGrabRadius * size_ratio_;
  float min_distance_squared = grab_radius * grab_radius;
  int hover_power = -1;

  int num_points = model_->getNumPoints();
  for (int i = 0; i < num_points; ++i) {
    if (powerActive(i)) {
      Point<float> power_position = getPowerPosition(i);
      power_position.x *= getWidth();
      power_position.y *= getHeight();
      float distance_squared = position.getDistanceSquaredFrom(power_position);
      if (distance_squared < min_distance_squared) {
        min_distance_squared = distance_squared;
        hover_power = i;
      }
    }
  }

  return hover_power;
}

float LineEditor::getSnapRadiusX() {
  static float kGridProximity = 0.02f;

  if (grid_size_x_ <= 1)
    return 0.0f;
  return kGridProximity;
}

float LineEditor::getSnapRadiusY() {
  static float kGridProximity = 0.04f;

  if (grid_size_y_ <= 1)
    return 0.0f;
  return kGridProximity;
}

float LineEditor::getSnappedX(float x) {
  return std::roundf(x * grid_size_x_) / grid_size_x_;
}

float LineEditor::getSnappedY(float y) {
  return std::roundf(y * grid_size_y_) / grid_size_y_;
}

void LineEditor::addPointAt(Point<float> position) {
  if (model_->getNumPoints() >= LineGenerator::kMaxPoints)
    return;

  int index = 0;
  float x = position.x;
  int num_points = model_->getNumPoints();
  for (; index < num_points; ++index) {
    if (model_->getPoint(index).first > x)
      break;
  }

  model_->addPoint(index, pointToPair(position));
  model_->render();
  resetPositions();

  for (Listener* listener : listeners_)
    listener->pointAdded(index, position);
}

void LineEditor::movePoint(int index, Point<float> position, bool snap) {
  float min_x = getMinX(index);
  float max_x = getMaxX(index);
  int last_point_index = model_->getNumPoints() - 1;

  Point<float> local_position(position.x / getWidth(), position.y / getHeight());

  local_position.x = vital::utils::clamp(local_position.x, min_x, max_x);
  local_position.y = vital::utils::clamp(local_position.y, 0.0f, 1.0f);
  if (snap && grid_size_x_ > 0) {
    float snap_radius = getSnapRadiusX();
    float snapped_x = vital::utils::clamp(getSnappedX(local_position.x), min_x, max_x);
    if (fabsf(snapped_x - local_position.x) < snap_radius)
      local_position.x = snapped_x;
  }
  if (snap && grid_size_y_ > 0) {
    float snap_radius = getSnapRadiusY();
    float snapped_y = getSnappedY(local_position.y);
    if (fabsf(snapped_y - local_position.y) < snap_radius)
      local_position.y = snapped_y;
  }

  if (loop_ && model_->getPoint(0).second == model_->getPoint(last_point_index).second) {
    if (index == 0)
      model_->setPoint(last_point_index, pointToPair({ 1.0f, local_position.y }));
    else if (index == last_point_index)
      model_->setPoint(0, pointToPair({ 0.0f, local_position.y }));
  }
  model_->setPoint(index, pointToPair(local_position));

  model_->render();
  resetPositions();

  for (Listener* listener : listeners_)
    listener->pointChanged(index, position, false);
}

void LineEditor::movePower(int index, Point<float> position, bool all, bool alternate) {
  float delta_change = (position.y - last_mouse_position_.y) / getHeight();
  int start = index;
  int end = index;
  if (all) {
    start = 0;
    end = model_->getNumPoints() - 2;
  }

  float alternate_mult = 1.0f;
  if (!alternate && model_->getPoint(index).second < model_->getPoint((index + 1) % model_->getNumPoints()).second)
    alternate_mult = -1.0f;

  for (int i = start; i <= end; ++i) {
    float from_y = model_->getPoint(i).second;
    float to_y = model_->getPoint((i + 1) % model_->getNumPoints()).second;
    if (from_y == to_y)
      continue;

    float max_power = vital::SynthLfo::kMaxPower;
    float power = model_->getPower(i);
    float delta_amount = delta_change * alternate_mult;
    if (from_y < to_y && alternate)
      delta_amount *= -1.0f;

    power += delta_amount * kPowerMouseMultiplier;
    model_->setPower(i, vital::utils::clamp(power, -max_power, max_power));
  }
  model_->render();
  resetPositions();

  for (Listener* listener : listeners_)
    listener->powersChanged(false);
}

void LineEditor::removePoint(int index) {
  VITAL_ASSERT(model_->getNumPoints() > 1);

  model_->removePoint(index);
  model_->render();

  resetPositions();

  for (Listener* listener : listeners_)
    listener->pointRemoved(index);
}

float LineEditor::getMinX(int index) {
  if (index == 0)
    return 0.0f;

  int last_point_index = model_->getNumPoints() - 1;
  if (index == last_point_index)
    return 1.0f;
  return model_->getPoint(index - 1).first;
}

float LineEditor::getMaxX(int index) {
  if (index == 0)
    return 0.0f;

  int last_point_index = model_->getNumPoints() - 1;
  if (index == last_point_index)
    return 1.0f;

  return model_->getPoint(index + 1).first;
}

void LineEditor::respondToCallback(int point, int power, int option) {
  if (option == kFlipHorizontal) {
    getModel()->flipHorizontal();
    resetPositions();
    for (Listener* listener : listeners_)
      listener->pointChanged(0, pairToPoint(model_->getPoint(0)), true);
  }
  else if (option == kFlipVertical) {
    getModel()->flipVertical();
    resetPositions();
    for (Listener* listener : listeners_)
      listener->pointChanged(0, pairToPoint(model_->getPoint(0)), true);
  }
  else if (option == kRemovePoint) {
    if (point > 0 && point < numPoints() - 1)
      removePoint(point);
  }
  else if (option == kResetPower) {
    if (power >= 0 && power < numPoints() - 1) {
      model_->setPower(power, 0.0f);
      model_->render();
      resetPositions();
    }
  }
  else if (option == kEnterPhase || option == kEnterValue) {
    entering_phase_ = option == kEnterPhase;
    entering_index_ = point;
    if (entering_index_ >= 0 && entering_index_ < model_->getNumPoints())
      showTextEntry();
  }
  else if (option == kCopy) {
    json json_data = model_->stateToJson();
    SystemClipboard::copyTextToClipboard(json_data.dump());
  }
  else if (option == kPaste) {
    String text = SystemClipboard::getTextFromClipboard();

    try {
      json parsed_json_state = json::parse(text.toStdString(), nullptr, false);
      if (LineGenerator::isValidJson(parsed_json_state))
        model_->jsonToState(parsed_json_state);
    }
    catch (const json::exception& e) {
      return;
    }

    for (Listener* listener : listeners_)
      listener->fileLoaded();

    resetPositions();
  }
  else if (option == kSave) {
    FullInterface* parent = findParentComponentOfClass<FullInterface>();
    if (parent == nullptr)
      return;

    parent->saveLfo(model_->stateToJson());
  }
  else if (option == kInit) {
    model_->initLinear();
    for (Listener* listener : listeners_)
      listener->fileLoaded();
    resetPositions();
  }
}

bool LineEditor::hasMatchingSystemClipboard() {
  String text = SystemClipboard::getTextFromClipboard();
  try {
    json parsed_json_state = json::parse(text.toStdString(), nullptr, false);
    return LineGenerator::isValidJson(parsed_json_state);
  }
  catch (const json::exception& e) {
    return false;
  }
}

void LineEditor::paintLine(const MouseEvent& e) {
  float percent_x = e.position.x / getWidth();
  float percent_y = std::max(0.0f, std::min(1.0f, e.position.y / getHeight()));
  active_grid_section_ = std::max<int>(0, std::min<int>(grid_size_x_ * percent_x, grid_size_x_ - 1));

  float from_x = (1.0f * active_grid_section_) / grid_size_x_;
  float to_x = (active_grid_section_ + 1.0f) / grid_size_x_;

  if (!e.mods.isAltDown() && grid_size_y_ > 0) {
    float snap_radius = getSnapRadiusY();
    float snapped_y = getSnappedY(percent_y);
    if (fabsf(snapped_y - percent_y) < snap_radius)
      percent_y = snapped_y;
  }

  int from_index = -1;
  int start_num_points = model_->getNumPoints();
  int to_index = start_num_points;
  for (int i = 0; i < start_num_points; ++i) {
    if (model_->getPoint(i).first < from_x)
      from_index = i;

    int reverse_index = start_num_points - i - 1;
    if (model_->getPoint(reverse_index).first > to_x)
      to_index = reverse_index;
  }

  std::vector<Point<float>> new_points;
  float from_intersect = model_->getValueAtPhase(from_x);
  float to_intersect = model_->getValueAtPhase(to_x);
  if (model_->getPoint(from_index + 1).first != from_x) {
    new_points.emplace_back(Point<float>(from_x, from_intersect));
    to_index++;
    start_num_points++;
  }
  from_index++;

  if (model_->getPoint(to_index - 1).first != to_x) {
    new_points.emplace_back(Point<float>(to_x, to_intersect));
    start_num_points++;
  }
  else
    to_index--;

  VITAL_ASSERT(from_index < to_index);

  int pattern_length = static_cast<int>(paint_pattern_.size());
  int delta_size = pattern_length - std::max(0, (to_index - from_index - 1));
  int num_points = start_num_points + delta_size;
  if (num_points >= LineGenerator::kMaxPoints)
    return;

  for (const Point<float>& new_point : new_points)
    addPointAt(new_point);

  model_->setNumPoints(num_points);

  if (delta_size > 0) {
    for (int i = num_points - 1; i >= std::max(delta_size, to_index); --i) {
      model_->setPoint(i, model_->getPoint(i - delta_size));
      model_->setPower(i, model_->getPower(i - delta_size));
    }

    for (Listener* listener : listeners_)
      listener->pointsAdded(from_index + 1, delta_size);
  }
  else if (delta_size < 0) {
    for (int i = from_index + 1; i < num_points; ++i) {
      model_->setPoint(i, model_->getPoint(i - delta_size));
      model_->setPower(i, model_->getPower(i - delta_size));
    }

    for (Listener* listener : listeners_)
      listener->pointsRemoved(from_index + 1, -delta_size);
  }

  for (int i = 0; i < pattern_length; ++i) {
    std::pair<float, float> pattern_point = paint_pattern_[i];
    float t = pattern_point.first;
    pattern_point.first = from_x * (1.0f - t) + to_x * t;
    pattern_point.second = 1.0f - pattern_point.second * (1.0 - percent_y);

    int index = from_index + 1 + i;
    model_->setPoint(index, pattern_point);
    model_->setPower(index, 0.0f);
  }

  model_->render();
  resetPositions();
}

void LineEditor::mouseDown(const MouseEvent& e) {
  enableTemporaryPaintToggle(e.mods.isCommandDown());
  if (e.mods.isPopupMenu()) {
    PopupItems options;

    int active_point = getActivePoint();
    if (active_point >= 0) {
      if (active_point > 0 && active_point < model_->getNumPoints() - 1) {
        options.addItem(kRemovePoint, "Remove Point");
        options.addItem(kEnterPhase, "Enter Point Phase");
      }
      options.addItem(kEnterValue, "Enter Point Value");
      options.addItem(-1, "");
    }
    int active_power = getActivePower();
    if (active_power >= 0)
      options.addItem(kResetPower, "Reset Power");

    if (allow_file_loading_) {
      options.addItem(kCopy, "Copy");
      if (hasMatchingSystemClipboard())
        options.addItem(kPaste, "Paste");

      options.addItem(kSave, "Save to LFOs");
      options.addItem(kInit, "Initialize");
    }

    options.addItem(kFlipHorizontal, "Flip Horizontal");
    options.addItem(kFlipVertical, "Flip Vertical");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    int point = active_point_;
    int power = active_power_;
    parent->showPopupSelector(this, e.getPosition(), options,
                              [=](int selection) { respondToCallback(point, power, selection); });
  }
  else if (isPainting())
    paintLine(e);
  else
    drawDown(e);
}

void LineEditor::drawDown(const MouseEvent& e) {
  last_mouse_position_ = e.position;
  int hover_point = getHoverPoint(e.position);
  if (hover_point >= 0) {
    active_point_ = hover_point;
    active_power_ = -1;
    dragging_ = true;
    resetPositions();
  }
  else {
    int hover_power = getHoverPower(e.position);
    if (hover_power >= 0) {
      active_power_ = hover_power;
      active_point_ = -1;
      dragging_ = true;
      resetPositions();
    }
  }
}

void LineEditor::mouseDoubleClick(const MouseEvent& e) {
  if (isPainting())
    return;

  int hover_point = getHoverPoint(e.position);
  int hover_power = getHoverPower(e.position);
  int num_points = model_->getNumPoints();

  if (hover_point >= 0) {
    if (hover_point == 0 || hover_point == num_points - 1)
      return;
    if (num_points <= 1)
      return;

    removePoint(hover_point);
  }
  else if (hover_power >= 0) {
    if (e.mods.isShiftDown()) {
      for (int i = 0; i < model_->getNumPoints() - 1; ++i)
        model_->setPower(i, 0.0f);
    }
    else
      model_->setPower(hover_power, 0.0f);
    model_->render();
    resetPositions();
  }
  else {
    if (num_points >= LineGenerator::kMaxPoints)
      return;

    Point<float> position = e.position;
    position.x /= getWidth();
    position.y /= getHeight();
    addPointAt(position);
  }
  active_point_ = getHoverPoint(e.position);
  active_power_ = -1;
  resetPositions();
}

void LineEditor::mouseMove(const MouseEvent& e) {
  enableTemporaryPaintToggle(e.mods.isCommandDown());

  if (isPainting()) {
    float percent_x = e.position.x / getWidth();
    int active_section = std::max<int>(0, std::min<int>(grid_size_x_ * percent_x, grid_size_x_ - 1));
    if (active_section != active_grid_section_) {
      active_grid_section_ = active_section;
      resetPositions();
    }
  }
  else {
    int hovered = getHoverPoint(e.position);
    int power_hover = -1;
    if (hovered < 0)
      power_hover = getHoverPower(e.position);

    if (active_point_ != hovered || active_power_ != power_hover) {
      active_point_ = hovered;
      active_power_ = power_hover;
      resetPositions();
    }
  }
}

void LineEditor::mouseDrag(const MouseEvent& e) {
  if (isPainting())
    paintLine(e);
  else
    drawDrag(e);

  last_mouse_position_ = e.position;
}

void LineEditor::drawDrag(const MouseEvent& e) {
  if (!dragging_)
    return;

  if (active_point_ >= 0)
    movePoint(active_point_, e.position, !e.mods.isAltDown());
  else if (active_power_ >= 0)
    movePower(active_power_, e.position, e.mods.isShiftDown(), e.mods.isAltDown());

  resetPositions();
}

void LineEditor::mouseUp(const MouseEvent& e) {
  if (!isPainting())
    drawUp(e);

  enableTemporaryPaintToggle(e.mods.isCommandDown());
}

void LineEditor::drawUp(const MouseEvent& e) {
  dragging_ = false;
  resetPositions();

  if (active_point_ >= 0) {
    for (Listener* listener : listeners_)
      listener->pointChanged(active_point_, pairToPoint(model_->getPoint(active_point_)), true);
  }
  else if (active_power_ >= 0) {
    for (Listener* listener : listeners_)
      listener->powersChanged(true);
  }
}

void LineEditor::mouseExit(const MouseEvent& e) {
  enableTemporaryPaintToggle(false);
  clearActiveMouseActions();
}

void LineEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  for (Listener* listener : listeners_)
    listener->lineEditorScrolled(e, wheel);
}

void LineEditor::clearActiveMouseActions() {
  dragging_ = false;
  active_point_ = -1;
  active_power_ = -1;
  active_grid_section_ = -1;
  
  resetPositions();
}

void LineEditor::resetWavePath() {
  int num_points = model_->getNumPoints();
  int intermediate_points = kDrawPoints - num_points;
  Point<float> prev_point = pairToPoint(model_->lastPoint());
  float power = model_->lastPower();
  prev_point.x -= 1.0f;

  float width = getWidth();
  float height = getHeight();

  int draw_index = 0;
  int point_index = 0;
  for (int i = 0; i < intermediate_points; ++i) {
    float t = i / (intermediate_points - 1.0f);

    while (point_index < num_points && t >= model_->getPoint(point_index).first) {
      prev_point = pairToPoint(model_->getPoint(point_index));
      power = model_->getPower(point_index);
      setXAt(kNumWrapPoints + draw_index, padX(width * model_->getPoint(point_index).first));
      setYAt(kNumWrapPoints + draw_index, padY(height * model_->getPoint(point_index).second));
      point_index++;
      draw_index++;
    }

    Point<float> next_point;
    if (point_index < num_points) {
      next_point = pairToPoint(model_->getPoint(point_index));
    }
    else {
      next_point = pairToPoint(model_->getPoint(0));
      next_point.x += 1.0f;
    }

    float x_distance = next_point.x - prev_point.x;
    float point_t = 0.0f;
    if (x_distance > 0.0f)
      point_t = (t - prev_point.x) / x_distance;

    if (model_->smooth())
      point_t = LineGenerator::smoothTransition(point_t);

    point_t = vital::futils::powerScale(point_t, power);

    float val = vital::utils::interpolate(prev_point.y, next_point.y, point_t);

    setXAt(kNumWrapPoints + draw_index, padX(width * t));
    setYAt(kNumWrapPoints + draw_index, padY(height * val));
    draw_index++;
  }

  float end_val = model_->getPoint(num_points - 1).second;

  for (; draw_index < kDrawPoints; ++draw_index) {
    setXAt(kNumWrapPoints + draw_index, padX(width));
    setYAt(kNumWrapPoints + draw_index, padY(height * end_val));
  }

  if (loop_) {
    for (int i = 0; i < kNumWrapPoints; ++i) {
      setXAt(i, xAt(kDrawPoints + i) - width);
      setYAt(i, yAt(kDrawPoints + i));
      setBoostLeft(i, boostLeftAt(kDrawPoints + i));
      setBoostRight(i, boostRightAt(kDrawPoints + i));

      setXAt(i + kDrawPoints + kNumWrapPoints, xAt(kNumWrapPoints + i) + width);
      setYAt(i + kDrawPoints + kNumWrapPoints, yAt(kNumWrapPoints + i));
      setBoostLeft(i + kDrawPoints + kNumWrapPoints, boostLeftAt(kNumWrapPoints + i));
      setBoostRight(i + kDrawPoints + kNumWrapPoints, boostRightAt(kNumWrapPoints + i));
    }
  }
  else {
    int last_index = kNumWrapPoints + kDrawPoints - 1;
    for (int i = 0; i < kNumWrapPoints; ++i) {
      setXAt(i, xAt(kNumWrapPoints));
      setYAt(i, yAt(kNumWrapPoints));
      setBoostLeft(i, boostLeftAt(kNumWrapPoints));
      setBoostRight(i, boostRightAt(kNumWrapPoints));

      setXAt(i + kDrawPoints + kNumWrapPoints, xAt(last_index));
      setYAt(i + kDrawPoints + kNumWrapPoints, yAt(last_index));
      setBoostLeft(i + kDrawPoints + kNumWrapPoints, boostLeftAt(last_index));
      setBoostRight(i + kDrawPoints + kNumWrapPoints, boostRightAt(last_index));
    }
  }
}

inline float LineEditor::padY(float y) {
  float pad = size_ratio_ * kPaddingY;
  return y * (getHeight() - 2.0f * pad) / getHeight() + pad;
}

inline float LineEditor::unpadY(float y) {
  float pad = size_ratio_ * kPaddingY;
  return (y - pad) * getHeight() / (getHeight() - 2.0f * pad);
}

inline float LineEditor::padX(float x) {
  if (loop_)
    return x;
  float pad = size_ratio_ * kPaddingX;
  return x * (getWidth() - 2.0f * pad) / getWidth() + pad;
}

inline float LineEditor::unpadX(float x) {
  if (loop_)
    return x;
  float pad = size_ratio_ * kPaddingX;
  return (x - pad) * getWidth() / (getWidth() - 2.0f * pad);
}

void LineEditor::init(OpenGlWrapper& open_gl) {  
  OpenGlLineRenderer::init(open_gl);
  drag_circle_.init(open_gl);
  hover_circle_.init(open_gl);
  grid_lines_.init(open_gl);
  point_circles_.init(open_gl);
  power_circles_.init(open_gl);
  position_circle_.init(open_gl);
}

void LineEditor::renderGrid(OpenGlWrapper& open_gl, bool animate) {
  grid_lines_.setColor(findColour(Skin::kLightenScreen, true));
  grid_lines_.render(open_gl, animate);
}

void LineEditor::renderPoints(OpenGlWrapper& open_gl, bool animate) {
  Colour center = findColour(Skin::kWidgetCenterLine, true);
  if (!active_)
    center = findColour(Skin::kWidgetPrimaryDisabled, true);

  Colour background = findColour(Skin::kWidgetBackground, true);
  point_circles_.setColor(center);
  point_circles_.setAltColor(background);
  point_circles_.render(open_gl, animate);

  power_circles_.setColor(center);
  power_circles_.render(open_gl, animate);

  drag_circle_.setColor(findColour(Skin::kWidgetAccent2, true));
  drag_circle_.render(open_gl, animate);

  hover_circle_.setColor(findColour(Skin::kWidgetAccent1, true));
  hover_circle_.render(open_gl, animate);
}

void LineEditor::render(OpenGlWrapper& open_gl, bool animate) {
  if (last_model_render_ != model_->getRenderCount()) {
    reset_positions_ = true;
    last_model_render_ = model_->getRenderCount();
  }
  setGlPositions();
  renderGrid(open_gl, animate);
  OpenGlLineRenderer::render(open_gl, animate);
  renderPoints(open_gl, animate);
  renderCorners(open_gl, animate);
}

Point<float> LineEditor::valuesToOpenGlPosition(float x, float y) {
  float padding_x = 2.0f * size_ratio_ * kPaddingX / getWidth();
  float padding_y = 2.0f * size_ratio_ * kPaddingY / getHeight();
  float adjusted_x = (x * 2.0f - 1.0f) * (1.0f - padding_x);
  float adjusted_y = (y * 2.0f - 1.0f) * (1.0f - padding_y);
  return Point<float>(adjusted_x, adjusted_y);
}

Point<float> LineEditor::getPowerPosition(int index) {
  VITAL_ASSERT(index >= 0 && index < model_->getNumPoints());
  Point<float> from = pairToPoint(model_->getPoint(index));
  Point<float> to;
  if (index < model_->getNumPoints() - 1)
    to = pairToPoint(model_->getPoint(index + 1));
  else {
    to = pairToPoint(model_->getPoint(0));
    to.x += 1.0f;
  }

  float x = (from.x + to.x) / 2.0f;
  if (x >= 1.0f)
    x -= 1.0f;
  float power_t = vital::futils::powerScale(0.5f, model_->getPower(index));
  float y = vital::utils::interpolate(from.y, to.y, power_t);
  return Point<float>(x, y);
}

bool LineEditor::powerActive(int index) {
  VITAL_ASSERT(index >= 0 && index < model_->getNumPoints());
  Point<float> delta;
  if (index < model_->getNumPoints() - 1)
    delta = pairToPoint(model_->getPoint(index + 1)) - pairToPoint(model_->getPoint(index));
  else
    delta = pairToPoint(model_->getPoint(0)) - pairToPoint(model_->lastPoint()) + Point<float>(1.0f, 0.0f);

  return getWidth() * delta.x >= kMinPointDistanceForPower && delta.y;
}

void LineEditor::drawPosition(OpenGlWrapper& open_gl, Colour color, float fraction_x) {
  static constexpr float kCenterFade = 0.2f;

  if (fraction_x == 0.0f)
     return;
  
  float fraction_y = model_->valueAtPhase(fraction_x);
  Point<float> point = valuesToOpenGlPosition(fraction_x, fraction_y);
  float x = point.x;
  float y = point.y;
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  
  int draw_width = getWidth();
  int draw_height = getHeight();
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  Colour background = findColour(Skin::kWidgetBackground, true);
  
  float position_height = 2.0f * size_ratio_ * kPositionWidth / draw_height;
  float position_width = 2.0f * size_ratio_ * kPositionWidth / draw_width;
  position_circle_.setQuad(0, x - position_width * 0.5f, y - position_height * 0.5f, position_width, position_height);
  position_circle_.setColor(color);
  position_circle_.setAltColor(color.interpolatedWith(background, kCenterFade));
  position_circle_.setThickness(size_ratio_ * kPositionWidth * kRingThickness * 0.5f);
  position_circle_.render(open_gl, true);
}

void LineEditor::setEditingCircleBounds() {
  Point<float> edit_position;
  if (active_point_ >= 0)
    edit_position = pairToPoint(model_->getPoint(active_point_));
  else if (active_power_ >= 0)
    edit_position = getPowerPosition(active_power_);
  else {
    drag_circle_.setActive(false);
    hover_circle_.setActive(false);
    return;
  }

  float width = getWidth();
  float height = getHeight();
  float x = padX(width * edit_position.x) * 2.0f / width - 1.0f;
  float y = 1.0f - padY(height * edit_position.y) * 2.0f / height;
  float hover_width = size_ratio_ * kGrabRadius * 4.0f / width;
  float hover_height = size_ratio_ * kGrabRadius * 4.0f / height;
  float drag_width = size_ratio_ * kDragRadius * 4.0f / width;
  float drag_height = size_ratio_ * kDragRadius * 4.0f / height;

  hover_circle_.setActive(!isPainting());
  hover_circle_.setQuad(0, x - hover_width * 0.5f, y - hover_height * 0.5f, hover_width, hover_height);

  drag_circle_.setActive(dragging_);
  if (dragging_)
    drag_circle_.setQuad(0, x - drag_width * 0.5f, y - drag_height * 0.5f, drag_width, drag_height);
}

void LineEditor::setGridPositions() {
  int grid_size_x = grid_size_x_;
  int grid_size_y = grid_size_y_;

  float width = 2.0f / getWidth();
  int index = 0;
  float x_scale = 1.0f - size_ratio_ * 2.0f * kPaddingX / getWidth();
  for (int i = 1; i < grid_size_x; ++i) {
    float x = i * 2.0f / grid_size_x - 1.0f;
    x *= x_scale;
    grid_lines_.setQuad(index, x - width * 0.5f, -1.0f, width, 2.0f);
    index++;
  }

  float height = 2.0f / getHeight();
  float y_scale = 1.0f - size_ratio_ * 2.0f * kPaddingY / getHeight();
  for (int i = 1; i < grid_size_y; ++i) {
    float y = i * 2.0f / grid_size_y - 1.0f;
    y *= y_scale;
    grid_lines_.setQuad(index, -1.0f, y - height * 0.5f, 2.0f, height);
    index++;
  }

  if (grid_size_x && isPainting() && active_grid_section_ >= 0) {
    int start_x = (active_grid_section_ * getWidth()) / grid_size_x + 1;
    int end_x = ((active_grid_section_ + 1) * getWidth()) / grid_size_x;
    grid_lines_.setQuad(index, start_x * width - 1.0f, -1.0f, (end_x - start_x) * width, 2.0f);
  }
  else
    grid_lines_.setQuad(index, -2.0f, -2.0f, 0.0f, 0.0f);
  grid_lines_.setNumQuads(grid_size_x + grid_size_y - 1);
}

void LineEditor::setPointPositions() {
  float width = getWidth();
  float height = getHeight();

  float position_width = size_ratio_ * kPositionWidth * 2.0f / width;
  float position_height = size_ratio_ * kPositionWidth * 2.0f / height;
  float power_width = size_ratio_ * kPowerWidth * 2.0f / width;
  float power_height = size_ratio_ * kPowerWidth * 2.0f / height;

  point_circles_.setThickness(size_ratio_ * kPositionWidth * kRingThickness * 0.5f);

  int num_points = model_->getNumPoints();
  point_circles_.setNumQuads(num_points);
  power_circles_.setNumQuads(num_points);
  for (int i = 0; i < num_points; ++i) {
    Point<float> point = pairToPoint(model_->getPoint(i));
    float x = padX(width * point.x) * 2.0f / width - 1.0f;
    float y = 1.0f - padY(height * point.y) * 2.0f / height;

    point_circles_.setQuad(i, x - position_width * 0.5f, y - position_height * 0.5f, position_width, position_height);

    if (powerActive(i)) {
      Point<float> power_position = getPowerPosition(i);
      x = padX(getWidth() * power_position.x) * 2.0f / width - 1.0f;
      y = 1.0f - padY(getHeight() * power_position.y) * 2.0f / height;
      power_circles_.setQuad(i, x - power_width * 0.5f, y - power_height * 0.5f, power_width, power_height);
    }
    else
      power_circles_.setQuad(i, -2.0f, -2.0f, power_width, power_width);
  }
}

void LineEditor::setGlPositions() {
  if (!reset_positions_)
    return;

  resetWavePath();
  reset_positions_ = false;

  setEditingCircleBounds();
  setGridPositions();
  setPointPositions();
}

void LineEditor::destroy(OpenGlWrapper& open_gl) {
  drag_circle_.destroy(open_gl);
  hover_circle_.destroy(open_gl);
  grid_lines_.destroy(open_gl);
  point_circles_.destroy(open_gl);
  power_circles_.destroy(open_gl);
  position_circle_.destroy(open_gl);
  OpenGlLineRenderer::destroy(open_gl);
}

void LineEditor::setPaint(bool paint) {
  paint_ = paint;
  active_point_ = -1;
  active_power_ = -1;
}

void LineEditor::hideTextEntry() {
  value_entry_->setVisible(false);
}

void LineEditor::showTextEntry() {
  static constexpr float kTextEntryHeight = 30.0f;
  static constexpr float kTextEntryWidth = 50.0f;

#if !defined(NO_TEXT_ENTRY)
  Point<float> point = pairToPoint(model_->getPoint(entering_index_));
  int entry_height = kTextEntryHeight * size_ratio_;
  int entry_width = kTextEntryWidth * size_ratio_;
  int x = std::min<int>(point.x * getWidth(), getWidth() - entry_width);
  int y = std::min<int>(point.y * getHeight(), getHeight() - entry_height);
  value_entry_->setBounds(x, y, entry_width, entry_height);
  if (entering_phase_)
    value_entry_->setText(String(point.x));
  else
    value_entry_->setText(String(1.0 - point.y));
  value_entry_->setVisible(true);
  value_entry_->grabKeyboardFocus();
#endif
}

void LineEditor::textEditorReturnKeyPressed(TextEditor& editor) {
  setSliderPositionFromText();
}

void LineEditor::textEditorFocusLost(TextEditor& editor) {
  setSliderPositionFromText();
}

void LineEditor::textEditorEscapeKeyPressed(TextEditor& editor) {
  hideTextEntry();
}

void LineEditor::setSliderPositionFromText() {
  if (value_entry_->getText().isEmpty() || entering_index_ < 0) {
    hideTextEntry();
    return;
  }

  float value = value_entry_->getText().getFloatValue();
  if (entering_phase_) {
    value = std::max(std::min(value, getMaxX(entering_index_)), getMinX(entering_index_));
    Point<float> point = pairToPoint(model_->getPoint(entering_index_));
    point.x = value;
    model_->setPoint(entering_index_, pointToPair(point));
  }
  else {
    value = std::max(std::min(value, 1.0f), 0.0f);
    Point<float> point = pairToPoint(model_->getPoint(entering_index_));
    float start_y = point.y;
    point.y = 1.0f - value;
    model_->setPoint(entering_index_, pointToPair(point));
    if (entering_index_ == 0 && pairToPoint(model_->getPoint(model_->getNumPoints() - 1)).y == start_y) {
      point.x = 1.0f;
      model_->setPoint(model_->getNumPoints() - 1, pointToPair(point));
    }
    else if (entering_index_ == model_->getNumPoints() - 1 && pairToPoint(model_->getPoint(0)).y == start_y) {
      point.x = 0.0f;
      model_->setPoint(0, pointToPair(point));
    }
  }

  model_->render();
  hideTextEntry();
  resetPositions();
}

vital::poly_float LineEditor::adjustBoostPhase(vital::poly_float phase) {
  vital::poly_float result;
  result.set(0, adjustBoostPhase(phase[0]));
  result.set(1, adjustBoostPhase(phase[1]));
  return result;
}

float LineEditor::adjustBoostPhase(float phase) {
  int num_points = model_->getNumPoints();
  int points_to_left = 0;

  for (; points_to_left < num_points; ++points_to_left) {
    if (model_->getPoint(points_to_left).first >= phase)
      break;
  }

  return (phase * kDrawPoints + points_to_left) / (kDrawPoints + num_points);
}

void LineEditor::enableTemporaryPaintToggle(bool toggle) {
  if (temporary_paint_toggle_ == toggle)
    return;

  temporary_paint_toggle_ = toggle;

  for (Listener* listener : listeners_)
    listener->togglePaintMode(isPaintEnabled(), toggle);

  resetPositions();
}
