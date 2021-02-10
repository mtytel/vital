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

#include "wavetable_component_list.h"

#include "paths.h"
#include "skin.h"
#include "text_look_and_feel.h"
#include "wavetable_component_factory.h"
#include "wavetable_creator.h"
#include "wavetable_group.h"

namespace {
  void componentRowCallback(int result, WavetableComponentList* component_list) {
    if (component_list == nullptr)
      return;
    if (result == WavetableComponentList::kReset)
      component_list->resetComponent();
    else if (result == WavetableComponentList::kRemove)
      component_list->removeComponent();
    else if (result == WavetableComponentList::kMoveUp)
      component_list->moveModifierUp();
    else if (result == WavetableComponentList::kMoveDown)
      component_list->moveModifierDown();
  }

  void componentRowGroupCallback(int result, WavetableComponentList* component_list) {
    if (component_list == nullptr)
      return;
    if (result == WavetableComponentList::kReset)
      component_list->resetComponent();
    else if (result == WavetableComponentList::kRemove)
      component_list->removeGroup();
    else if (result == WavetableComponentList::kMoveUp)
      component_list->moveGroupUp();
    else if (result == WavetableComponentList::kMoveDown)
      component_list->moveGroupDown();
  }
} // namespace

WavetableComponentList::WavetableComponentList(WavetableCreator* wavetable_creator) :
    SynthSection("component list"), wavetable_creator_(wavetable_creator),
    component_backgrounds_(kMaxRows, Shaders::kRoundedRectangleFragment), row_height_(0) {
  current_group_index_ = -1;
  current_component_index_ = -1;

  addAndMakeVisible(component_container_);

  addOpenGlComponent(&component_backgrounds_);
  component_backgrounds_.setTargetComponent(&component_container_);

  create_component_button_ = std::make_unique<OpenGlToggleButton>("Add Source");
  component_container_.addAndMakeVisible(create_component_button_.get());
  addOpenGlComponent(create_component_button_->getGlComponent());
  create_component_button_->setUiButton(false);
  create_component_button_->addListener(this);
  create_component_button_->setJustification(Justification::centredLeft);

  for (int i = 0; i < kMaxSources; ++i) {
    add_modifier_buttons_[i] = std::make_unique<OpenGlToggleButton>("Add Modifier");
    component_container_.addAndMakeVisible(add_modifier_buttons_[i].get());
    addOpenGlComponent(add_modifier_buttons_[i]->getGlComponent());
    add_modifier_buttons_[i]->setUiButton(false);
    add_modifier_buttons_[i]->addListener(this);
    add_modifier_buttons_[i]->setJustification(Justification::centredLeft);
  }

  for (int i = 0; i <= kMaxSources; ++i) {
    plus_icons_[i] = std::make_unique<PlainShapeComponent>("plus");
    plus_icons_[i]->setJustification(Justification::centredLeft);
    component_container_.addAndMakeVisible(plus_icons_[i].get());
    plus_icons_[i]->setVisible(false);
    open_gl_components_.push_back(plus_icons_[i].get());
  }
  plus_icons_[kMaxSources]->setVisible(true);

  addAndMakeVisible(viewport_);
  viewport_.setViewedComponent(&component_container_);
  viewport_.setScrollBarsShown(false, false, true, false);
  viewport_.addListener(this);

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);
  scroll_bar_->setAlwaysOnTop(true);
  scroll_bar_->setShrinkLeft(true);

  for (int i = 0; i < kMaxRows; ++i) {
    names_[i] = std::make_unique<PlainTextComponent>(String(i), "");
    names_[i]->setFontType(PlainTextComponent::kLight);
    names_[i]->setInterceptsMouseClicks(false, false);
    names_[i]->setJustification(Justification::centredLeft); 
    names_[i]->setScissor(true);
    component_container_.addChildComponent(names_[i].get());
    open_gl_components_.push_back(names_[i].get());

    menu_buttons_[i] = std::make_unique<OpenGlShapeButton>(String(i));
    menu_buttons_[i]->setShape(Paths::menu());
    component_container_.addChildComponent(menu_buttons_[i].get());
    addOpenGlComponent(menu_buttons_[i]->getGlComponent());
    menu_buttons_[i]->addListener(this);
  }
}

void WavetableComponentList::clear() {
  resetGroups();
}

void WavetableComponentList::init() {
  resetGroups();
}

void WavetableComponentList::paintBackground(Graphics& g) {
  Colour lighten = findColour(Skin::kLightenScreen, true);
  component_backgrounds_.setColor(lighten);
  scroll_bar_->setColor(lighten);

  Colour ui_button_text = findColour(Skin::kUiButtonText, true);
  for (int i = 0; i <= kMaxSources; ++i)
    plus_icons_[i]->setColor(ui_button_text);

  paintOpenGlChildrenBackgrounds(g);
}

void WavetableComponentList::resized() {
  static constexpr float kScrollBarWidth = 13.0f;
  SynthSection::resized();

  scroll_bar_->setBounds(0, 0, size_ratio_ * kScrollBarWidth, getHeight());
  resetGroups();
}

void WavetableComponentList::resetGroups() {
  int num_groups = numGroups();
  int index = 0;
  for (int i = 0; i < num_groups; ++i) {
    WavetableGroup* group = wavetable_creator_->getGroup(i);
    int num_components = group->numComponents();

    for (int n = 0; n < num_components; ++n) {
      WavetableComponent* component = group->getComponent(n);
      String name = WavetableComponentFactory::getComponentName(component->getType());
      names_[index]->setText(name);
      names_[index]->setTextSize(row_height_ * 0.5f);
      index++;
    }
  }

  positionGroups();
}

void WavetableComponentList::positionGroups() {
  viewport_.setScrollBarThickness(0.0f);
  viewport_.setBounds(0, 0, getWidth(), getHeight());

  float rounding = findValue(Skin::kLabelBackgroundRounding);
  component_backgrounds_.setRounding(rounding);

  int total_rows = 0;
  int num_groups = numGroups();
  for (int i = 0; i < num_groups; ++i)
    total_rows += wavetable_creator_->getGroup(i)->numComponents() + 1;

  int cell_height = row_height_ - 2;
  int button_y = total_rows * row_height_ + 2;
  create_component_button_->getGlComponent()->text().setBuffer(rounding * 0.5f + cell_height);
  create_component_button_->setBounds(row_height_ - rounding, button_y, getWidth(), cell_height);
  plus_icons_[kMaxSources]->setBounds(row_height_ - rounding * 0.5f, button_y, cell_height, cell_height);
  component_container_.setBounds(0, 0, getWidth(), button_y + cell_height + row_height_ * 0.5f);

  int index = 0;
  int row = 0;
  int width = getWidth();
  float source_gl_x = (row_height_ - rounding) * 2.0f / width - 1.0f;
  float modifier_gl_x = (2.0f * row_height_ - rounding) * 2.0f / width - 1.0f;
  float gl_width = 2.0f + row_height_ * 2.0f / width;
  int container_height = component_container_.getHeight();
  float gl_height = 2.0f * cell_height / container_height;
  Path menu = Paths::menu(cell_height);
  Path plus = Paths::plus(cell_height);
  plus_icons_[kMaxSources]->setShape(plus);

  for (int i = 0; i < num_groups; ++i) {
    WavetableGroup* group = wavetable_creator_->getGroup(i);
    int num_components = group->numComponents();

    for (int n = 0; n < num_components; ++n) {
      int y = row * row_height_ + 2;
      float gl_y = 1.0f - y * 2.0f / container_height - gl_height;

      menu_buttons_[index]->setBounds(width - cell_height, y, cell_height, cell_height);

      if (n == 0) {
        names_[index]->setBounds(row_height_, y, width - row_height_, cell_height);
        component_backgrounds_.setQuad(index, source_gl_x, gl_y, gl_width, gl_height);
      }
      else {
        names_[index]->setBounds(2 * row_height_, y, width - 2 * row_height_, cell_height);
        component_backgrounds_.setQuad(index, modifier_gl_x, gl_y, gl_width, gl_height);
      }
      names_[index]->setVisible(true);
      names_[index]->redrawImage(false);
      menu_buttons_[index]->setShape(menu);
      menu_buttons_[index]->setVisible(true);

      index++;
      row++;
    }

    int add_modifier_y = row * row_height_ + 2;
    add_modifier_buttons_[i]->getGlComponent()->text().setBuffer(rounding * 0.5f + cell_height);
    add_modifier_buttons_[i]->setBounds(2 * row_height_ - rounding, add_modifier_y, width, cell_height);
    add_modifier_buttons_[i]->setVisible(true);

    plus_icons_[i]->setBounds(2 * row_height_ - rounding * 0.5f, add_modifier_y, cell_height, cell_height);
    plus_icons_[i]->setShape(plus);
    plus_icons_[i]->setVisible(true);

    row++;
  }

  for (int i = num_groups; i < kMaxSources; ++i) {
    add_modifier_buttons_[i]->setVisible(false);
    plus_icons_[i]->setVisible(false);
  }

  component_backgrounds_.setNumQuads(index);
  for (int i = index; i < kMaxRows; ++i) {
    names_[i]->setVisible(false);
    menu_buttons_[i]->setVisible(false);
  }

  setScrollBarRange();
}

void WavetableComponentList::setScrollBarRange() {
  scroll_bar_->setRangeLimits(0.0, component_container_.getHeight());
  scroll_bar_->setCurrentRange(scroll_bar_->getCurrentRangeStart(), viewport_.getHeight(), dontSendNotification);
}

std::pair<int, int> WavetableComponentList::getIndicesForRow(int row_index) {
  int num_groups = numGroups();
  int index = row_index;
  for (int i = 0; i < num_groups; ++i) {
    WavetableGroup* group = wavetable_creator_->getGroup(i);
    int num_components = group->numComponents();
    if (index < num_components)
      return { i, index };

    index -= num_components;
  }
  return { -1, -1 };
}

void WavetableComponentList::groupMenuClicked(int row_index) {
  PopupItems options;

  current_group_index_ = getIndicesForRow(row_index).first;
  current_component_index_ = 0;
  if (current_group_index_ < 0)
    return;

  if (current_group_index_ > 0)
    options.addItem(kMoveUp, "Move Group Up");
  if (current_group_index_ < numGroups() - 1)
    options.addItem(kMoveDown, "Move Group Down");

  options.addItem(kReset, "Reset Source");
  options.addItem(kRemove, "Remove Group");

  OpenGlShapeButton* button = menu_buttons_[row_index].get();
  showPopupSelector(this, Point<int>(button->getX(), button->getBottom()), options,
                    [=](int selection) { componentRowGroupCallback(selection, this); });
  button->setState(Button::buttonNormal);
}

void WavetableComponentList::modifierMenuClicked(int row_index) {
  PopupItems options;

  std::pair<int, int> indices = getIndicesForRow(row_index);
  current_group_index_ = indices.first;
  current_component_index_ = indices.second;
  if (current_group_index_ < 0)
    return;

  if (current_component_index_ > 1)
    options.addItem(kMoveUp, "Move Up");
  if (current_component_index_ < wavetable_creator_->getGroup(current_group_index_)->numComponents() - 1)
    options.addItem(kMoveDown, "Move Down");

  options.addItem(kReset, "Reset");
  options.addItem(kRemove, "Remove");
  OpenGlShapeButton* button = menu_buttons_[row_index].get();
  showPopupSelector(this, Point<int>(button->getX(), button->getBottom()), options,
                    [=](int selection) { componentRowCallback(selection, this); });
  button->setState(Button::buttonNormal);
}

void WavetableComponentList::menuClicked(int row_index) {
  if (getIndicesForRow(row_index).second == 0)
    groupMenuClicked(row_index);
  else
    modifierMenuClicked(row_index);
}

void WavetableComponentList::addModifierClicked(int group_index) {
  current_group_index_ = group_index;
  PopupItems options;

  for (int i = 0; i < WavetableComponentFactory::numModifierTypes(); ++i) {
    WavetableComponentFactory::ComponentType type = WavetableComponentFactory::getModifierType(i);
    options.addItem(i, WavetableComponentFactory::getComponentName(type));
  }

  Component* button = add_modifier_buttons_[current_group_index_].get();
  showPopupSelector(this, Point<int>(button->getX(), button->getBottom()), options,
                    [=](int selection) { addComponent(selection); });
}

void WavetableComponentList::addSourceClicked() {
  PopupItems options;

  for (int i = 0; i < WavetableComponentFactory::numSourceTypes(); ++i) {
    WavetableComponentFactory::ComponentType type = WavetableComponentFactory::getSourceType(i);
    options.addItem(i, WavetableComponentFactory::getComponentName(type));
  }

  showPopupSelector(this, Point<int>(create_component_button_->getX(), create_component_button_->getBottom()), options,
                    [=](int selection) { addSource(selection); });
}

void WavetableComponentList::buttonClicked(Button* button) {
  if (button == create_component_button_.get()) {
    addSourceClicked();
    return;
  }
  for (int i = 0; i < kMaxSources; ++i) {
    if (button == add_modifier_buttons_[i].get()) {
      addModifierClicked(i);
      return;
    }
  }
  for (int i = 0; i < kMaxRows; ++i) {
    if (button == menu_buttons_[i].get()) {
      menuClicked(i);
      return;
    }
  }
}

void WavetableComponentList::componentsScrolled() {
  int offset = viewport_.getVerticalScrollBar().getCurrentRangeStart();
  for (Listener* listener : listeners_)
    listener->componentsScrolled(-offset);

  scroll_bar_->setCurrentRange(offset, viewport_.getHeight(), dontSendNotification);
}

void WavetableComponentList::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
  viewport_.setViewPosition({ 0, (int)range_start });
}

void WavetableComponentList::addSource(int index) {
  WavetableComponentFactory::ComponentType type = WavetableComponentFactory::getSourceType(index);
  WavetableComponent* component = WavetableComponentFactory::createComponent(type);
  component->insertNewKeyframe(0);
  WavetableGroup* group = new WavetableGroup();
  group->addComponent(component);
  wavetable_creator_->addGroup(group);

  notifyComponentAdded(component);
  notifyComponentsChanged();
  resetGroups();
}

void WavetableComponentList::removeGroup(int index) {
  WavetableGroup* to_remove = wavetable_creator_->getGroup(index);

  for (int i = 0; i < to_remove->numComponents(); ++i)
    notifyComponentRemoved(to_remove->getComponent(i));
  wavetable_creator_->removeGroup(index);
  notifyComponentsChanged();
}

void WavetableComponentList::addComponent(int type) {
  if (current_group_index_ >= 0) {
    WavetableGroup* group = wavetable_creator_->getGroup(current_group_index_);
    WavetableComponentFactory::ComponentType comp_type =
        static_cast<WavetableComponentFactory::ComponentType>(type + WavetableComponentFactory::kBeginModifierTypes);
    WavetableComponent* component = WavetableComponentFactory::createComponent(comp_type);
    component->insertNewKeyframe(0);
    group->addComponent(component);
    resetGroups();
  }

  notifyComponentsReordered();
}

void WavetableComponentList::removeComponent() {
  if (current_group_index_ >= 0 && current_component_index_ >= 0) {
    WavetableGroup* group = wavetable_creator_->getGroup(current_group_index_);
    notifyComponentRemoved(group->getComponent(current_component_index_));
    group->removeComponent(current_component_index_);
    notifyComponentsChanged();
    resetGroups();
  }
}

void WavetableComponentList::resetComponent() {
  if (current_group_index_ >= 0) {
    WavetableGroup* group = wavetable_creator_->getGroup(current_group_index_);
    WavetableComponent* to_reset = group->getComponent(current_component_index_);
    notifyComponentRemoved(to_reset);
    to_reset->reset();
    notifyComponentAdded(to_reset);
    notifyComponentsChanged();
  }
}

void WavetableComponentList::removeGroup() {
  if (current_group_index_ >= 0) {
    WavetableGroup* group = wavetable_creator_->getGroup(current_group_index_);
    int num_components = group->numComponents();
    for (int i = 0; i < num_components; ++i)
      notifyComponentRemoved(group->getComponent(i));

    wavetable_creator_->removeGroup(current_group_index_);
    resetGroups();
  }

  notifyComponentsChanged();
}

void WavetableComponentList::moveGroupUp() {
  if (current_group_index_ > 0) {
    wavetable_creator_->moveUp(current_group_index_);
    resetGroups();
  }

  notifyComponentsReordered();
}

void WavetableComponentList::moveGroupDown() {
  if (current_group_index_ < numGroups() - 1) {
    wavetable_creator_->moveDown(current_group_index_);
    resetGroups();
  }

  notifyComponentsReordered();
}

void WavetableComponentList::moveModifierUp() {
  if (current_group_index_ >= 0 && current_component_index_ > 0) {
    WavetableGroup* group = wavetable_creator_->getGroup(current_group_index_);
    group->moveUp(current_component_index_);
    notifyComponentsReordered();
    resetGroups();
  }
}

void WavetableComponentList::moveModifierDown() {
  if (current_group_index_ >= 0){
    WavetableGroup* group = wavetable_creator_->getGroup(current_group_index_);
    if (current_component_index_ < group->numComponents() - 1)
      group->moveDown(current_component_index_);
    notifyComponentsReordered();
    resetGroups();
  }
}

void WavetableComponentList::notifyComponentAdded(WavetableComponent* component) {
  resetGroups();
  for (Listener* listener : listeners_)
    listener->componentAdded(component);
}

void WavetableComponentList::notifyComponentRemoved(WavetableComponent* component) {
  resetGroups();
  for (Listener* listener : listeners_)
    listener->componentRemoved(component);
}

void WavetableComponentList::notifyComponentsReordered() {
  for (Listener* listener : listeners_)
    listener->componentsReordered();
  notifyComponentsChanged();
}

void WavetableComponentList::notifyComponentsChanged() {
  for (Listener* listener : listeners_)
    listener->componentsChanged();
}
