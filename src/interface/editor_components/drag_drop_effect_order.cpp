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

#include "drag_drop_effect_order.h"

#include "skin.h"
#include "fonts.h"
#include "paths.h"
#include "synth_button.h"
#include "synth_gui_interface.h"
#include "synth_strings.h"
#include "text_look_and_feel.h"
#include "utils.h"

namespace {
  Path getPathForEffect(String effect) {
    if (effect == "compressor")
      return Paths::compressor();
    if (effect == "chorus")
      return Paths::chorus();
    if (effect == "delay")
      return Paths::delay();
    if (effect == "distortion")
      return Paths::distortion();
    if (effect == "eq")
      return Paths::equalizer();
    if (effect == "filter")
      return Paths::effectsFilter();
    if (effect == "flanger")
      return Paths::flanger();
    if (effect == "phaser")
      return Paths::phaser();
    if (effect == "reverb")
      return Paths::reverb();

    return Path();
  }
}

DraggableEffect::DraggableEffect(const String& name, int order) : SynthSection(name), order_(order), hover_(false) {
  setInterceptsMouseClicks(false, true);

  StringArray tokens;
  tokens.addTokens(getName(), "_", "");
  icon_ = getPathForEffect(tokens[0].toLowerCase());

  background_ = std::make_unique<OpenGlImageComponent>("background");
  addOpenGlComponent(background_.get());
  background_->setComponent(this);
  background_->paintEntireComponent(false);

  enable_ = std::make_unique<SynthButton>(name + "_on");
  enable_->setPowerButton();
  addButton(enable_.get());
  enable_->setButtonText("");
  enable_->getGlComponent()->setAlwaysOnTop(true);
}

void DraggableEffect::paint(Graphics& g) {
  static constexpr float kLeftPadding = 0.07f;
  static constexpr float kIconSize = 0.6f;
  static constexpr int kTextureRows = 2;
  static constexpr int kTextureColumns = 3;
  static constexpr float kTextureYStart = 0.13f;
  static constexpr float kTexturePadding = 0.45f;
  static constexpr float kTextureCircleRadiusPercent = 0.25f;

  g.setColour(getParentComponent()->findColour(Skin::kBody, true));
  int round_amount = findValue(Skin::kBodyRounding);
  g.fillRoundedRectangle(0, 0, getWidth(), getHeight(), round_amount);

  if (enable_->getToggleState())
    g.setColour(findColour(Skin::kPowerButtonOn, true));
  else
    g.setColour(findColour(Skin::kPowerButtonOff, true));
  g.drawRoundedRectangle(0.5f, 0.5f, getWidth() - 1.0f, getHeight() - 1.0f, round_amount, 1.0f);

  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(size_ratio_ * 12.0f));
  float text_x = getWidth() * kLeftPadding;
  StringArray tokens;
  tokens.addTokens(getName(), "_", "");
  String text = tokens[0];
  text = text.substring(0, 1).toUpperCase() + text.substring(1);
  g.drawText(text, text_x, 0, getWidth() - text_x, getHeight(), Justification::centredLeft, true);

  float icon_width = kIconSize * getHeight();
  float icon_x = getWidth() / 2.0f + (getWidth() / 2.0f - icon_width) / 2.0f;
  float icon_y = (getHeight() - icon_width) / 2.0f;
  Rectangle<float> icon_bounds(icon_x, icon_y, icon_width, icon_width);
  g.fillPath(icon_, icon_.getTransformToScaleToFit(icon_bounds, true));

  if (hover_) {
    g.setColour(findColour(Skin::kLightenScreen, true).withMultipliedAlpha(1.5f));

    int width = getWidth();
    int height = getHeight();
    float spacing = width * (1.0f - 2.0f * kTexturePadding) / (kTextureColumns - 1.0f);
    float radius = spacing * kTextureCircleRadiusPercent;
    float x = width * kTexturePadding;
    float y = height * kTextureYStart;
    for (int c = 0; c < kTextureColumns; ++c) {
      for (int r = 0; r < kTextureRows; ++r) {
        g.fillEllipse(x + spacing * c - radius, y + spacing * r - radius, 2.0f * radius, 2.0f * radius);
        g.fillEllipse(x + spacing * c - radius, height - y - spacing * r - radius, 2.0f * radius, 2.0f * radius);
      }
    }
  }
}

void DraggableEffect::resized() {
  int width = getTitleWidth();
  enable_->setBounds(0, 0, width, width);
  background_->redrawImage(true);
}

void DraggableEffect::buttonClicked(Button* clicked_button) {
  for (Listener* listener : listeners_)
    listener->effectEnabledChanged(this, clicked_button->getToggleState());

  background_->redrawImage(true);
  SynthSection::buttonClicked(clicked_button);
}

void DraggableEffect::hover(bool hover) {
  if (hover_ != hover) {
    hover_ = hover;
    background_->redrawImage(true);
  }
}

DragDropEffectOrder::DragDropEffectOrder(String name) : SynthSection(name) {
  currently_dragged_ = nullptr;
  currently_hovered_ = nullptr;
  last_dragged_index_ = 0;
  mouse_down_y_ = 0;
  dragged_starting_y_ = 0;
  for (int i = 0; i < vital::constants::kNumEffects; ++i) {
    effect_order_[i] = i;
    effect_list_.push_back(std::make_unique<DraggableEffect>(strings::kEffectOrder[i], i));
    addSubSection(effect_list_[i].get());
    effect_list_[i]->addListener(this);
    effect_list_[i]->setSkinOverride(static_cast<Skin::SectionOverride>(Skin::kAllEffects + 1 + i));
  }
}

DragDropEffectOrder::~DragDropEffectOrder() { }

void DragDropEffectOrder::resized() {
  float padding = size_ratio_ * kEffectPadding;

  for (int i = 0; i < vital::constants::kNumEffects; ++i) {
    Component* effect = getEffect(i);
    int from_y = getEffectY(i);
    int to_y = getEffectY(i + 1);
    effect->setBounds(0, from_y, getWidth(), to_y - from_y - padding);
  }
}

void DragDropEffectOrder::paintBackground(Graphics& g) {
  static constexpr float kConnectionWidth = 0.1f;
  g.setColour(findColour(Skin::kLightenScreen, true));
  int width = getWidth() * kConnectionWidth;
  int center = getWidth() / 2.0f + findValue(Skin::kWidgetRoundedCorner);
  g.fillRect(center - width / 2, 0, width, getHeight());
}

void DragDropEffectOrder::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  SynthSection::renderOpenGlComponents(open_gl, animate);

  DraggableEffect* current = currently_dragged_;
  if (current)
    current->renderOpenGlComponents(open_gl, animate);
}

void DragDropEffectOrder::mouseMove(const MouseEvent& e) {
  int current_index = getEffectIndexFromY(e.y);
  DraggableEffect* hover = effect_list_[effect_order_[current_index]].get();
  if (hover != currently_hovered_) {
    if (currently_hovered_)
      currently_hovered_->hover(false);
    if (hover)
      hover->hover(true);
    currently_hovered_ = hover;
  }
}

void DragDropEffectOrder::mouseDown(const MouseEvent& e) {
  mouse_down_y_ = e.y;
  last_dragged_index_ = getEffectIndexFromY(e.y);
  currently_dragged_ = effect_list_[effect_order_[last_dragged_index_]].get();
  dragged_starting_y_ = currently_dragged_->getY();

  currently_dragged_->setAlwaysOnTop(true);
}

void DragDropEffectOrder::mouseDrag(const MouseEvent& e) {
  if (currently_dragged_ == nullptr)
    return;

  int delta_y = e.y - mouse_down_y_;
  int clamped_y = vital::utils::iclamp(dragged_starting_y_ + delta_y, 0,
                                       getHeight() - currently_dragged_->getHeight());
  currently_dragged_->setTopLeftPosition(currently_dragged_->getX(), clamped_y);

  int next_index = getEffectIndexFromY(e.y);
  if (next_index != last_dragged_index_) {
    moveEffect(last_dragged_index_, next_index);
    last_dragged_index_ = next_index;
  }
}

void DragDropEffectOrder::mouseUp(const MouseEvent& e) {
  if (currently_dragged_)
    currently_dragged_->setAlwaysOnTop(false);

  currently_dragged_ = nullptr;
  setStationaryEffectPosition(last_dragged_index_);
}

void DragDropEffectOrder::mouseExit(const MouseEvent& e) {
  if (currently_hovered_) {
    currently_hovered_->hover(false);
    currently_hovered_ = nullptr;
  }
}

void DragDropEffectOrder::effectEnabledChanged(DraggableEffect* effect, bool enabled) {
  for (Listener* listener : listeners_)
    listener->effectEnabledChanged(effect->order(), enabled);
}

void DragDropEffectOrder::setAllValues(vital::control_map& controls) {
  SynthSection::setAllValues(controls);
  float order = controls[getName().toStdString()]->value();
  vital::utils::decodeFloatToOrder(effect_order_, order, vital::constants::kNumEffects);

  for (int i = 0; i < vital::constants::kNumEffects; ++i)
    setStationaryEffectPosition(i);

  for (Listener* listener : listeners_)
    listener->orderChanged(this);
}

void DragDropEffectOrder::moveEffect(int start_index, int end_index) {
  int delta_index = end_index - start_index;
  if (delta_index == 0)
    return;

  int moving = effect_order_[start_index];

  int direction = delta_index / abs(delta_index);
  for (int i = start_index; i != end_index; i += direction) {
    effect_order_[i] = effect_order_[i + direction];
    setStationaryEffectPosition(i);
  }

  effect_order_[end_index] = moving;

  float order = vital::utils::encodeOrderToFloat(effect_order_, vital::constants::kNumEffects);
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->getSynth()->valueChangedInternal(getName().toStdString(), order);

  for (Listener* listener : listeners_)
    listener->orderChanged(this);
}

void DragDropEffectOrder::setStationaryEffectPosition(int index) {
  float padding = size_ratio_ * kEffectPadding;
  Component* effect = getEffect(index);
  int from_y = getEffectY(index);
  int to_y = getEffectY(index + 1);
  effect->setBounds(0, from_y, getWidth(), to_y - from_y - padding);
}

int DragDropEffectOrder::getEffectIndex(int index) const {
  int i = vital::utils::iclamp(index, 0, vital::constants::kNumEffects - 1);
  return effect_order_[i];
}

Component* DragDropEffectOrder::getEffect(int index) const {
  return effect_list_[getEffectIndex(index)].get();
}

bool DragDropEffectOrder::effectEnabled(int index) const {
  return effect_list_[getEffectIndex(index)]->enabled();
}

int DragDropEffectOrder::getEffectIndexFromY(float y) const {
  float padding = size_ratio_ * kEffectPadding;
  int index = vital::constants::kNumEffects * (y + padding / 2.0f) / (getHeight() + padding);
  return vital::utils::iclamp(index, 0, vital::constants::kNumEffects - 1);
}

int DragDropEffectOrder::getEffectY(int index) const {
  int padding = size_ratio_ * kEffectPadding;
  return std::round((1.0f * index * (getHeight() + padding)) / vital::constants::kNumEffects);
}
