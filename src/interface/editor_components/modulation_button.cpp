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

#include "modulation_button.h"

#include "default_look_and_feel.h"
#include "modulation_matrix.h"
#include "synth_gui_interface.h"
#include "fonts.h"
#include "skin.h"

ModulationButton::ModulationButton(String name) : PlainShapeComponent(std::move(name)), parent_(nullptr),
                                                  mouse_state_(kNone), selected_(false), connect_right_(false),
                                                  draw_border_(false), active_modulation_(false), font_size_(12.0f),
                                                  show_drag_drop_(false), drag_drop_alpha_(0.0f) {
  setWantsKeyboardFocus(true);
  Path shape = Paths::dragDropArrows();
  shape.addLineSegment(Line<float>(-50.0f, -50.0f, -50.0f, -50.0f), 0.2f);
  setShape(Paths::dragDropArrows());
  setComponent(&drag_drop_area_);
  setActive(false);
  setUseAlpha(true);
  setInterceptsMouseClicks(true, false);
  addAndMakeVisible(drag_drop_area_);
  drag_drop_area_.setInterceptsMouseClicks(false, false);
  setColor(Colours::transparentWhite);
}

ModulationButton::~ModulationButton() {
  if (parent_)
    parent_->getSynth()->forceShowModulation(getName().toStdString(), false);
}

bool ModulationButton::hasAnyModulation() {
  if (parent_)
    return parent_->getSynth()->isSourceConnected(getName().toStdString());
  return false;
}

Rectangle<int> ModulationButton::getModulationAmountBounds(int index, int total) {
  int columns = kModulationKnobColumns;

  int row = index / columns;
  int column = index % columns;

  Rectangle<int> all_bounds = getModulationAreaBounds();
  int x = all_bounds.getX() + (all_bounds.getWidth() * column) / columns;
  int right = all_bounds.getX() + (all_bounds.getWidth() * (column + 1)) / columns;
  int width = right - x;
  int y = all_bounds.getY() + all_bounds.getHeight() - width * (row + 1);
  return Rectangle<int>(x, y, width, width);
}

Rectangle<int> ModulationButton::getMeterBounds() {
  static constexpr int kMinMeterWidth = 4;

  int width = getWidth();
  int meter_width = std::max<int>(kMinMeterWidth, std::round(width * kMeterAreaRatio / 2.0f) * 2);
  int meter_height = getHeight() - 2;
  return Rectangle<int>(1, 1, meter_width, meter_height);
}

Rectangle<int> ModulationButton::getModulationAreaBounds() {
  static constexpr int kMaxWidthHeightRatio = 3;

  SynthSection* parent = findParentComponentOfClass<SynthSection>();
  int widget_margin = 0;
  if (parent)
    widget_margin = parent->findValue(Skin::kWidgetMargin);

  int width = getWidth() - getMeterBounds().getRight();
  int height = getHeight();

  int widget_width = width - 2 * widget_margin;
  int knob_width = widget_width / kModulationKnobColumns;
  widget_width = knob_width * kModulationKnobColumns;
  int widget_x = getMeterBounds().getRight() + (width - widget_width) / 2;
  int min_y = kFontAreaHeightRatio * width;
  int max_widget_height = ceilf(widget_width * 2.0f / 3.0f);
  int widget_y = std::max(min_y, height - widget_margin - max_widget_height);
  int widget_height = height - widget_y - widget_margin;
  int center_y = widget_y + widget_height / 2;
  widget_height = std::max(widget_height, (widget_width + kMaxWidthHeightRatio - 1) / kMaxWidthHeightRatio);
  widget_y = center_y - widget_height / 2;
  return Rectangle<int>(widget_x, widget_y, widget_width, widget_height);
}

void ModulationButton::paintBackground(Graphics& g) {
  static constexpr float kShadowArea = 0.04f;

  if (getWidth() == 0 || getHeight() == 0)
    return;

  if (selected_)
    g.setColour(findColour(Skin::kModulationButtonSelected, true));
  else
    g.setColour(findColour(Skin::kModulationButtonUnselected, true));

  SynthSection* parent = findParentComponentOfClass<SynthSection>();
  int rounding_amount = 0;
  if (parent)
    rounding_amount = parent->findValue(Skin::kBodyRounding);

  Rectangle<float> meter_bounds = getMeterBounds().toFloat();
  int width = getWidth();
  int adjusted_width = connect_right_ ? width * 2 : width;
  Rectangle<float> bounds(0, 0, adjusted_width, getHeight());
  g.fillRoundedRectangle(bounds, rounding_amount);

  g.setColour(findColour(Skin::kWidgetBackground, true));
  g.fillRoundedRectangle(meter_bounds, meter_bounds.getWidth() / 2.0f);
  float meter_width = meter_bounds.getWidth();
  g.fillRect(meter_bounds.getX() + meter_width / 2.0f, meter_bounds.getY(), meter_width / 2, meter_bounds.getHeight());

  if (draw_border_) {
    g.setColour(findColour(Skin::kBorder, true));
    g.drawRoundedRectangle(bounds.reduced(0.5f), rounding_amount, 1.0f);
  }

  int height = getHeight();
  g.setColour(findColour(Skin::kBodyText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(font_size_));
  String text = text_override_;
  if (text.isEmpty())
    text = ModulationMatrix::getUiSourceDisplayName(getName());

  int font_area_height = kFontAreaHeightRatio * width;
  g.drawText(text, meter_bounds.getRight(), 0, width - meter_bounds.getRight(),
             font_area_height, Justification::centred);

  if (connect_right_ && !selected_) {
    int shadow_width = width * kShadowArea;
    Colour shadow_color = findColour(Skin::kShadow, true);
    ColourGradient gradient(shadow_color, width, 0, shadow_color.withAlpha(0.0f), width - shadow_width, 0, false);
    g.setGradientFill(gradient);
    g.fillRect(width - shadow_width, 0, shadow_width, height);
  }
}

void ModulationButton::parentHierarchyChanged() {
  if (parent_ == nullptr) {
    parent_ = findParentComponentOfClass<SynthGuiInterface>();
    setForceEnableModulationSource();
  }
}

void ModulationButton::resized() {
  static constexpr float kBorder = 0.2f;

  PlainShapeComponent::resized();
  Rectangle<float> meter_bounds = getMeterBounds().toFloat();
  int left = meter_bounds.getRight();
  int width = getWidth() - left;
  int font_area_height = kFontAreaHeightRatio * width;
  int top = font_area_height - (font_area_height - font_size_) * 0.5f;
  int height = getHeight() - top;

  float size_mult = 1.0f - 2.0f * kBorder;
  drag_drop_area_.setBounds(left + width * kBorder, top + height * kBorder,
                            width * size_mult, height * size_mult);
}

void ModulationButton::render(OpenGlWrapper& open_gl, bool animate) {
  static constexpr float kDeltaAlpha = 0.15f;

  float target = 0.0f;
  if (show_drag_drop_) {
    target = 1.0f;
    if (mouse_state_ == kMouseDown || mouse_state_ == kMouseDragging)
      target = 2.0f;
  }

  bool increase = drag_drop_alpha_ < target;
  if (increase)
    drag_drop_alpha_ = std::min(drag_drop_alpha_ + kDeltaAlpha, target);
  else
    drag_drop_alpha_ = std::max(drag_drop_alpha_ - kDeltaAlpha, target);

  if (drag_drop_alpha_ <= 0.0f) {
    drag_drop_alpha_ = 0.0f;
    setActive(false);
  }

  setColor(drag_drop_color_.withMultipliedAlpha(drag_drop_alpha_));
  PlainShapeComponent::render(open_gl, animate);
}

void ModulationButton::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu()) {
    if (parent_ == nullptr)
      return;

    std::vector<vital::ModulationConnection*> connections =
        parent_->getSynth()->getSourceConnections(getName().toStdString());

    if (connections.empty())
      return;

    mouse_state_ = kNone;

    PopupItems options;
    std::string disconnect = "Disconnect from ";
    for (int i = 0; i < connections.size(); ++i) {
      std::string destination = vital::Parameters::getDisplayName(connections[i]->destination_name);
      options.addItem(kModulationList + i, disconnect + destination);
    }

    if (connections.size() > 1)
      options.addItem(kDisconnect, "Disconnect all");

    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    parent->showPopupSelector(this, e.getPosition(), options, [=](int selection) { disconnectIndex(selection); });
  }
  else {
    setActiveModulation(true);
    mouse_state_ = kMouseDown;

    for (Listener* listener : listeners_)
      listener->modulationSelected(this);
  }
}

void ModulationButton::mouseDrag(const MouseEvent& e) {
  if (e.mods.isRightButtonDown())
    return;

  if (!getLocalBounds().contains(e.getPosition()) && mouse_state_ != kDraggingOut) {
    for (Listener* listener : listeners_)
      listener->startModulationMap(this, e);
    mouse_state_ = kDraggingOut;
    setMouseCursor(MouseCursor::DraggingHandCursor);
  }

  if (mouse_state_ == kDraggingOut) {
    for (Listener* listener : listeners_)
      listener->modulationDragged(e);
  }
  else if (mouse_state_ != kMouseDragging)
    mouse_state_ = kMouseDragging;
}

void ModulationButton::mouseUp(const MouseEvent& e) {
  if (!e.mods.isRightButtonDown() && mouse_state_ == kDraggingOut) {
    for (Listener* listener : listeners_)
      listener->endModulationMap();
  }
  else if (!e.mods.isRightButtonDown()) {
    for (Listener* listener : listeners_)
      listener->modulationClicked(this);
  }
  setMouseCursor(MouseCursor::ParentCursor);

  mouse_state_ = kHover;
}

void ModulationButton::mouseEnter(const MouseEvent& e) {
  mouse_state_ = kHover;
  drag_drop_color_ = findColour(Skin::kLightenScreen, true);
  show_drag_drop_ = parent_->getSynth()->getSourceConnections(getName().toStdString()).empty();
  setActive(show_drag_drop_);
  redrawImage(true);
}

void ModulationButton::mouseExit(const MouseEvent& e) {
  mouse_state_ = kNone;
  show_drag_drop_ = false;
}

void ModulationButton::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  for (Listener* listener : listeners_)
    listener->modulationWheelMoved(e, wheel);
}

void ModulationButton::focusLost(FocusChangeType cause) {
  for (Listener* listener : listeners_)
    listener->modulationLostFocus(this);
}

void ModulationButton::addListener(Listener* listener) {
  listeners_.push_back(listener);
}

void ModulationButton::disconnectIndex(int index) {
  if (parent_ == nullptr)
    return;

  std::vector<vital::ModulationConnection*> connections =
      parent_->getSynth()->getSourceConnections(getName().toStdString());

  if (index == kDisconnect) {
    for (vital::ModulationConnection* connection : connections)
      disconnectModulation(connection);
  }
  else if (index >= kModulationList) {
    int connection_index = index - kModulationList;
    disconnectModulation(connections[connection_index]);
  }
}

void ModulationButton::select(bool select) {
  selected_ = select;
  setForceEnableModulationSource();
}

void ModulationButton::setActiveModulation(bool active) {
  active_modulation_ = active;
  setForceEnableModulationSource();
}

void ModulationButton::setForceEnableModulationSource() {
  if (parent_)
    parent_->getSynth()->forceShowModulation(getName().toStdString(), active_modulation_);
}

void ModulationButton::disconnectModulation(vital::ModulationConnection* connection) {
  int modulations_left = parent_->getSynth()->getNumModulations(connection->destination_name);

  for (Listener* listener : listeners_) {
    listener->modulationDisconnected(connection, modulations_left <= 1);
    listener->modulationConnectionChanged();
  }

  parent_->disconnectModulation(connection);

  if (modulations_left <= 1) {
    for (Listener* listener : listeners_)
      listener->modulationCleared();
  }
}

