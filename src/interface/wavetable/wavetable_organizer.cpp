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

#include "wavetable_organizer.h"

#include "skin.h"
#include "default_look_and_feel.h"

namespace {
  void organizerCallback(int result, WavetableOrganizer* organizer) {
    if (result == WavetableOrganizer::kCreate)
      organizer->createKeyframeAtMenu();
    else if (result == WavetableOrganizer::kRemove)
      organizer->deleteSelectedKeyframes();
  }
}

bool DraggableFrame::isInside(int x, int y) {
  int x_distance = std::min(x, getWidth() - x);
  int y_distance = std::min(y, getWidth() - y);
  return x_distance + y_distance >= getWidth() / 2;
}

WavetableOrganizer::WavetableOrganizer(WavetableCreator* wavetable_creator, int max_frames) :
    SynthSection("Wavetable Organizer"),
    unselected_frame_quads_(kMaxKeyframes, Shaders::kDiamondFragment),
    selected_frame_quads_(kMaxKeyframes, Shaders::kDiamondFragment),
    active_rows_(WavetableComponentList::kMaxRows, Shaders::kColorFragment),
    selection_quad_(Shaders::kColorFragment), playhead_quad_(Shaders::kColorFragment) {
  unselected_frame_quads_.setTargetComponent(this);
  selected_frame_quads_.setTargetComponent(this);
  active_rows_.setTargetComponent(this);

  unselected_frame_quads_.setThickness(2.0f);
  selected_frame_quads_.setThickness(2.0f);
  addOpenGlComponent(&active_rows_);
  addOpenGlComponent(&unselected_frame_quads_);
  addOpenGlComponent(&selected_frame_quads_);
  addOpenGlComponent(&selection_quad_);
  addOpenGlComponent(&playhead_quad_);

  wavetable_creator_ = wavetable_creator;
  max_frames_ = max_frames;
  frame_width_ = 0.0f;
  currently_dragged_ = nullptr;
  dragged_start_x_ = 0;
  mouse_mode_ = kWaiting;
  playhead_position_ = 0;
  draw_vertical_offset_ = 0;

  recreateVisibleFrames();
}

WavetableOrganizer::~WavetableOrganizer() {
  listeners_.clear();
  clearVisibleFrames();
}

void WavetableOrganizer::paintBackground(Graphics& g) {
  g.fillAll(findColour(Skin::kBackground, true));

  Colour lighten = findColour(Skin::kLightenScreen, true);
  g.setColour(lighten.withMultipliedAlpha(0.5f));
  int half_handle = 0.5f * handleWidth();
  g.fillRect(0, 0, half_handle, getHeight());
  g.fillRect(getWidth() - half_handle, 0, half_handle, getHeight());
  for (int i = kDrawSkip; i < max_frames_ - 1; i += kDrawSkip) {
    if (i % kDrawSkipLarge == 0)
      g.setColour(lighten);
    else
      g.setColour(lighten.withMultipliedAlpha(0.5f));

    int x = i * frame_width_ + half_handle;
    g.fillRect(x, 0, 1, getHeight());
  }

  g.setColour(lighten);

  Colour edge = findColour(Skin::kBackground, true);
  Colour primary = findColour(Skin::kWidgetPrimary1, true);
  unselected_frame_quads_.setColor(edge);
  unselected_frame_quads_.setAltColor(findColour(Skin::kWidgetPrimaryDisabled, true));
  selected_frame_quads_.setColor(edge);
  selected_frame_quads_.setAltColor(primary);
  selection_quad_.setColor(lighten);
  playhead_quad_.setColor(primary);
  active_rows_.setColor(lighten);
}

void WavetableOrganizer::resized() {
  repositionVisibleFrames();
  playheadMoved(playhead_position_);

  setFrameQuads();
  setRowQuads();
}

void WavetableOrganizer::deselect() {
  for (WavetableKeyframe* keyframe : currently_selected_) {
    if (frame_lookup_.count(keyframe))
      frame_lookup_[keyframe]->select(false);
  }
  currently_selected_.clear();

  for (Listener* listener : listeners_)
    listener->frameSelected(nullptr);

  setFrameQuads();
}

void WavetableOrganizer::deleteKeyframe(WavetableKeyframe* keyframe) {
  keyframe->owner()->remove(keyframe);
  frame_lookup_.erase(keyframe);

  for (Listener* listener : listeners_)
    listener->positionsUpdated();

  setRowQuads();
}

void WavetableOrganizer::createKeyframeAtPosition(Point<int> position) {
  int row_index = getRowFromY(position.y);
  WavetableComponent* component = getComponentAtRow(row_index);
  if (component == nullptr)
    return;

  int x_position = getPositionFromX(position.x - handleWidth() / 2);

  WavetableKeyframe* new_keyframe = component->insertNewKeyframe(x_position);
  std::unique_ptr<DraggableFrame> new_frame = std::make_unique<DraggableFrame>(!component->hasKeyframes());
  int x = frame_width_ * new_keyframe->position();
  new_frame->setBounds(x, row_index * handleWidth() + draw_vertical_offset_, handleWidth(), handleWidth());
  addAndMakeVisible(new_frame.get());
  frame_lookup_[new_keyframe] = std::move(new_frame);

  selectFrame(new_keyframe);
  for (Listener* listener : listeners_)
    listener->positionsUpdated();

  setFrameQuads();
  setRowQuads();
}

void WavetableOrganizer::selectDefaultFrame() {
  if (wavetable_creator_->numGroups()) {
    WavetableGroup* group = wavetable_creator_->getGroup(0);
    if (group->numComponents()) {
      WavetableComponent* component = group->getComponent(0);
      if (component->numFrames())
        selectFrame(component->getFrameAt(0));
    }
  }
}

void WavetableOrganizer::selectFrame(WavetableKeyframe* keyframe) {
  selectFrames({keyframe});

  for (Listener* listener : listeners_)
    listener->frameSelected(keyframe);
}

void WavetableOrganizer::selectFrames(std::vector<WavetableKeyframe*> keyframes) {
  deselect();

  for (WavetableKeyframe* keyframe : keyframes) {
    if (frame_lookup_.count(keyframe)) {
      DraggableFrame* frame = frame_lookup_[keyframe].get();
      frame->select(true);
      frame->toFront(false);
    }

    currently_selected_.push_back(keyframe);
  }
  setFrameQuads();
}

void WavetableOrganizer::positionSelectionBox(const MouseEvent& e) {
  int position_end = getPositionFromX(e.x - handleWidth() / 2);
  int position_start = getPositionFromX(mouse_down_position_.x - handleWidth() / 2);
  int row_end = getRowFromY(e.y);
  int row_start = getRowFromY(mouse_down_position_.y);

  int position_left = std::min(position_start, position_end);
  int position_right = std::max(position_start, position_end);
  int row_top = std::min(row_start, row_end);
  int row_bottom = std::max(row_start, row_end) + 1;

  int x = std::roundf(position_left * frame_width_);
  int y = std::roundf(row_top * handleWidth()) + draw_vertical_offset_ + 1;
  int width = std::roundf(position_right * frame_width_) - x;
  int height = std::roundf(row_bottom * handleWidth()) + draw_vertical_offset_ - y;

  selection_quad_.setBounds(x + handleWidth() / 2 - 1, y, width + 2, height);
}

void WavetableOrganizer::setRowQuads() {
  float height = getHeight();
  int handle_width = handleWidth();
  float row_height = handle_width * 2.0f / height;
  float buffer = 2.0f / height;
  float y = 1.0f - draw_vertical_offset_ * 2.0f / height - buffer;

  int index = 0;
  for (int g = 0; g < wavetable_creator_->numGroups(); ++g) {
    int num_components = wavetable_creator_->getGroup(g)->numComponents();
    for (int n = 0; n < num_components; ++n) {
      active_rows_.setQuad(index, -1.0f, y - row_height + buffer, 2.0f, row_height - 2.0f * buffer);
      y -= row_height;
      index++;
    }
    y -= row_height;
  }

  active_rows_.setNumQuads(index);
}

void WavetableOrganizer::setFrameQuads() {
  int num_selected = 0;
  int num_unselected = 0;
  float gl_width_scale = 2.0f / getWidth();
  float gl_height_scale = 2.0f / getHeight();
  for (auto& frame : frame_lookup_) {
    if (frame.second == nullptr)
      continue;

    Rectangle<int> bounds = frame.second->getBounds();
    float x = bounds.getX() * gl_width_scale - 1.0f;
    float y = 1.0f - bounds.getBottom() * gl_height_scale;
    float width = bounds.getWidth() * gl_width_scale;
    float height = bounds.getHeight() * gl_height_scale;
    if (frame.second->selected()) {
      selected_frame_quads_.setQuad(num_selected, x, y, width, height);
      num_selected++;
    }
    else {
      unselected_frame_quads_.setQuad(num_unselected, x, y, width, height);
      num_unselected++;
    }
  }

  selected_frame_quads_.setNumQuads(num_selected);
  unselected_frame_quads_.setNumQuads(num_unselected);
}

int WavetableOrganizer::handleWidth() const {
  return 1 + 2 * (int)(getHeight() * kHandleHeightPercent * 0.5f);
}

void WavetableOrganizer::deleteSelectedKeyframes() {
  std::vector<WavetableKeyframe*> selected = currently_selected_;
  deselect();
  for (WavetableKeyframe* keyframe : selected)
    deleteKeyframe(keyframe);

  setFrameQuads();
}

void WavetableOrganizer::createKeyframeAtMenu() {
  createKeyframeAtPosition(menu_created_position_);
}

int WavetableOrganizer::getRowFromY(int y) {
  return std::max(0.0f, (y - draw_vertical_offset_) * 1.0f / handleWidth());
}

int WavetableOrganizer::getPositionFromX(int x) {
  return std::min(std::max(0, getUnclampedPositionFromX(x)), max_frames_ - 1);
}

int WavetableOrganizer::getUnclampedPositionFromX(int x) {
  return x / frame_width_;
}

bool WavetableOrganizer::isSelected(WavetableKeyframe* keyframe) {
  auto found = std::find(currently_selected_.begin(), currently_selected_.end(), keyframe);
  return found != currently_selected_.end();
}

void WavetableOrganizer::clearVisibleFrames() {
  frame_lookup_.clear();
}

void WavetableOrganizer::recreateVisibleFrames() {
  clearVisibleFrames();

  for (int g = 0; g < wavetable_creator_->numGroups(); ++g) {
    WavetableGroup* group = wavetable_creator_->getGroup(g);

    for (int i = 0; i < group->numComponents(); ++i) {
      WavetableComponent* component = group->getComponent(i);

      for (int f = 0; f < component->numFrames(); ++f) {
        WavetableKeyframe* keyframe = component->getFrameAt(f);
        std::unique_ptr<DraggableFrame> frame = std::make_unique<DraggableFrame>(!component->hasKeyframes());
        addAndMakeVisible(frame.get());
        frame_lookup_[keyframe] = std::move(frame);
      }
    }
  }
  repositionVisibleFrames();
  if (currently_selected_.size() == 1)
    selectFrame(currently_selected_[0]);
  else if (currently_selected_.size() > 1)
    selectFrames(currently_selected_);
}

void WavetableOrganizer::repositionVisibleFrames() {
  frame_width_ = (getWidth() - handleWidth() + 1) / (max_frames_ - 1.0f);

  int y = draw_vertical_offset_;
  for (int g = 0; g < wavetable_creator_->numGroups(); ++g) {
    WavetableGroup* group = wavetable_creator_->getGroup(g);

    for (int i = 0; i < group->numComponents(); ++i) {
      WavetableComponent* component = group->getComponent(i);

      for (int f = 0; f < component->numFrames(); ++f) {
        WavetableKeyframe* keyframe = component->getFrameAt(f);
        int x = frame_width_ * keyframe->position();
        DraggableFrame* frame = frame_lookup_[keyframe].get();
        if (frame) {
          if (frame->fullFrame())
            frame->setBounds(0, y, getWidth(), handleWidth());
          else
            frame->setBounds(x, y, handleWidth(), handleWidth());
        }
      }

      y += handleWidth();
    }

    y += handleWidth();
  }
  setFrameQuads();
}

WavetableComponent* WavetableOrganizer::getComponentAtRow(int row) {
  int internal_row = row;
  for (int g = 0; g < wavetable_creator_->numGroups() && internal_row >= 0; ++g) {
    WavetableGroup* group = wavetable_creator_->getGroup(g);

    if (internal_row < group->numComponents())
      return group->getComponent(internal_row);

    internal_row -= group->numComponents() + 1;
  }
  return nullptr;
}

WavetableKeyframe* WavetableOrganizer::getFrameAtMouseEvent(const MouseEvent& e) {
  WavetableComponent* component = getComponentAtRow(getRowFromY(e.y));
  if (component == nullptr)
    return nullptr;

  int position = getUnclampedPositionFromX(e.x - handleWidth());
  if (!component->hasKeyframes())
    return component->getFrameAtPosition(-1);

  WavetableKeyframe* keyframe = component->getFrameAtPosition(position);
  DraggableFrame* frame = frame_lookup_[keyframe].get();

  if (frame && frame->isInside(e.x - frame->getX(), e.y - frame->getY()))
    return keyframe;

  return nullptr;
}

void WavetableOrganizer::mouseDown(const MouseEvent& e) {
  Component::mouseDown(e);
  mouse_down_position_ = e.position.toInt();

  WavetableKeyframe* keyframe = getFrameAtMouseEvent(e);
  if (keyframe) {
    DraggableFrame* frame = frame_lookup_[keyframe].get();

    mouse_mode_ = kDragging;
    currently_dragged_ = keyframe;
    dragged_start_x_ = frame->getX();

    if (!isSelected(keyframe))
      selectFrame(keyframe);
  }
}

void WavetableOrganizer::mouseDrag(const MouseEvent& e) {
  Component::mouseDrag(e);

  int x_movement = e.x - mouse_down_position_.x;
  if (mouse_mode_ == kWaiting) {
    if (x_movement) {
      selection_quad_.setVisible(true);
      mouse_mode_ = kSelecting;
      positionSelectionBox(e);
    }
  }
  else if (mouse_mode_ == kDragging) {
    int new_frame_position = getUnclampedPositionFromX(dragged_start_x_ + x_movement);
    int delta_frame_position = new_frame_position - currently_dragged_->position();

    for (WavetableKeyframe* keyframe : currently_selected_) {
      if (!keyframe->owner()->hasKeyframes())
        continue;

      DraggableFrame* frame = frame_lookup_[keyframe].get();
      int frame_position = keyframe->position() + delta_frame_position;
      int show_frame_position = vital::utils::iclamp(frame_position, 0, max_frames_ - 1);
      int x = show_frame_position * frame_width_;

      keyframe->setPosition(show_frame_position);
      frame->setTopLeftPosition(x, frame->getY());
    }

    int clamped_position = getPositionFromX(dragged_start_x_ + x_movement);

    for (Listener* listener : listeners_)
      listener->frameDragged(currently_dragged_, clamped_position);

    setFrameQuads();
  }
  else if (mouse_mode_ == kSelecting)
    positionSelectionBox(e);
}

void WavetableOrganizer::mouseUp(const MouseEvent& e) {
  Component::mouseUp(e);

  if (mouse_mode_ == kWaiting)
    deselect();
  else if (mouse_mode_ == kSelecting) {
    std::vector<WavetableKeyframe*> selection;
    int row_start_index = getRowFromY(selection_quad_.getY());
    int row_end_index = getRowFromY(selection_quad_.getBottom());

    for (int row_index = row_start_index; row_index < row_end_index; ++row_index) {
      WavetableComponent* component = getComponentAtRow(row_index);

      if (component) {
        int start_position = getPositionFromX(selection_quad_.getX() - handleWidth() / 2);
        int end_position = getPositionFromX(selection_quad_.getRight() - handleWidth() / 2);
        int start = component->getIndexFromPosition(start_position - 1);
        int end = component->getIndexFromPosition(end_position);
        for (int i = start; i < end; ++i)
          selection.push_back(component->getFrameAt(i));
      }
    }
    selectFrames(selection);
    selection_quad_.setVisible(false);
  }
  else if (mouse_mode_ == kDragging) {
    currently_dragged_ = nullptr;


    int start_position = vital::WaveFrame::kWaveformSize - 1;
    int end_position = 0;
    for (WavetableKeyframe* keyframe : currently_selected_) {
      if (!keyframe->owner()->hasKeyframes())
        continue;

      keyframe->setPosition(vital::utils::iclamp(keyframe->position(), 0, max_frames_ - 1));
      keyframe->owner()->reposition(keyframe);
      start_position = std::min(start_position, keyframe->position());
      end_position = std::max(end_position, keyframe->position());
    }

    for (Listener* listener : listeners_)
      listener->positionsUpdated();
  }

  if (e.mods.isPopupMenu()) {
    int row_index = getRowFromY(e.y);
    WavetableComponent* component = getComponentAtRow(row_index);
    if (component == nullptr || !component->hasKeyframes()) {
      mouse_mode_ = kWaiting;
      return;
    }

    menu_created_position_ = e.getPosition();
    PopupItems options;

    if (!currently_selected_.empty()) {
      if (currently_selected_.size() > 1)
        options.addItem(kRemove, "Remove Keyframes");
      else
        options.addItem(kRemove, "Remove Keyframe");
    }
    else {
      if (getComponentAtRow(row_index))
        options.addItem(kCreate, "Create Keyframe");
    }

    showPopupSelector(this, e.getPosition(), options, [=](int selection) { organizerCallback(selection, this); });
  }

  mouse_mode_ = kWaiting;
}

void WavetableOrganizer::mouseDoubleClick(const MouseEvent& e) {
  int row_index = getRowFromY(e.y);
  WavetableComponent* component = getComponentAtRow(row_index);
  if (component == nullptr || !component->hasKeyframes())
    return;

  WavetableKeyframe* keyframe = getFrameAtMouseEvent(e);
  if (keyframe) {
    deselect();
    deleteKeyframe(keyframe);
    setFrameQuads();
  }
  else
    createKeyframeAtPosition(e.getPosition());
}

void WavetableOrganizer::playheadMoved(int position) {
  playhead_position_ = position;
  int x = handleWidth() / 2 + position * frame_width_;
  playhead_quad_.setBounds(x, 0, 1, getHeight());
}

void WavetableOrganizer::componentRemoved(WavetableComponent* component) {
  std::vector<WavetableKeyframe*> new_selected;
  for (WavetableKeyframe* selected : currently_selected_) {
    if (selected->owner() != component)
      new_selected.push_back(selected);
  }
  if (!new_selected.empty())
    selectFrames(new_selected);
  else
    deselect();
}

void WavetableOrganizer::componentAdded(WavetableComponent* component) {
  recreateVisibleFrames();

  if (component->numFrames())
    selectFrame(component->getFrameAt(0));
}

void WavetableOrganizer::componentsScrolled(int offset) {
  draw_vertical_offset_ = offset;
  repositionVisibleFrames();
  setRowQuads();
}
