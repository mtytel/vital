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

#include "transpose_quantize.h"
#include "skin.h"
#include "fonts.h"
#include "common.h"
#include "text_look_and_feel.h"

namespace {
  void setKeyBounds(Rectangle<float>* bounds, float y, float width, float height) {
    static constexpr int kWhiteKeys = 7;
    static constexpr int kMissingBlackKey = 2;
    static constexpr float kHeightDifferenceMult = 0.86602540378f;
    static constexpr float kPaddingRatio = 0.03f;
    static constexpr float kOuterPaddingRatio = 0.11f;

    int inner_padding = width * kPaddingRatio;
    int outer_padding = width * kOuterPaddingRatio;
    float key_width = (width - (kWhiteKeys - 1) * inner_padding - 2 * outer_padding) * 1.0f / kWhiteKeys;
    float y_mid = y + (height - key_width) / 2.0f;
    float height_offset = kHeightDifferenceMult * (key_width + inner_padding);
    float y_black = y_mid - height_offset / 2.0f;
    float y_white = y_black + height_offset;

    for (int i = 0; i < kWhiteKeys; ++i) {
      float x = outer_padding + (key_width + inner_padding) * i;
      int index = 2 * i;
      if (i > kMissingBlackKey)
        index--;

      bounds[index] = Rectangle<float>(x, y_white, key_width, key_width);
    }

    float black_offset = (key_width + inner_padding) / 2.0f;
    for (int i = 0; i < kWhiteKeys - 1; ++i) {
      if (i == kMissingBlackKey)
        continue;

      float x = outer_padding + black_offset + (key_width + inner_padding) * i;
      int index = 2 * i;
      if (i < kMissingBlackKey)
        index++;
      bounds[index] = Rectangle<float>(x, y_black, key_width, key_width);
    }
  }
}

TransposeQuantizeCallOut::TransposeQuantizeCallOut(bool* selected, bool* global_snap) :
    SynthSection("Transpose Quantize Call Out"), selected_(selected), global_snap_(global_snap),
    hover_index_(-1), enabling_(false), disabling_(false) {
  global_snap_button_ = std::make_unique<ToggleButton>("Global Snap");
  global_snap_button_->addListener(this);
  addAndMakeVisible(global_snap_button_.get());
  global_snap_button_->setLookAndFeel(TextLookAndFeel::instance());
  global_snap_button_->setToggleState(*global_snap, dontSendNotification);

  setSkinOverride(Skin::kOscillator);
}

void TransposeQuantizeCallOut::paint(Graphics& g) {
  int title_height = getHeight() * kTitleHeightRatio;
  int text_height = title_height * kTitleTextRatio;

  g.setColour(parent_->findColour(Skin::kBodyText, true));
  g.setFont(Fonts::instance()->proportional_light().withPointHeight(text_height));
  g.drawText("TRANSPOSE SNAP", 0, 0, getWidth(), title_height, Justification::centred);

  g.setColour(parent_->findColour(Skin::kLabelBackground, true));
  float rounding = findValue(Skin::kLabelBackgroundRounding);
  g.fillRoundedRectangle(global_snap_button_->getBounds().toFloat(), rounding);

  for (int i = 0; i < vital::kNotesPerOctave; ++i) {
    if (selected_[i]) {
      if (*global_snap_)
        g.setColour(parent_->findColour(Skin::kUiActionButton, true));
      else
        g.setColour(parent_->findColour(Skin::kWidgetPrimary1, true));
    }
    else
      g.setColour(parent_->findColour(Skin::kLightenScreen, true));

    g.fillEllipse(key_bounds_[i]);
  }

  if (hover_index_ >= 0) {
    g.setColour(parent_->findColour(Skin::kLightenScreen, true));
    g.fillEllipse(key_bounds_[hover_index_]);
  }
}

void TransposeQuantizeCallOut::resized() {
  int title_height = getHeight() * kTitleHeightRatio;
  int global_height = getHeight() * kGlobalHeightRatio;
  setKeyBounds(key_bounds_, title_height, getWidth(), getHeight() - title_height - global_height);
  global_snap_button_->setBounds(0, getHeight() - global_height, getWidth(), global_height);
  repaint();
}

void TransposeQuantizeCallOut::mouseDown(const MouseEvent& e) {
  int hover_index = getHoverIndex(e);
  enabling_ = false;
  disabling_ = false;
  if (hover_index < 0)
    return;

  if (selected_[hover_index])
    disabling_ = true;
  else
    enabling_ = true;

  selected_[hover_index] = !selected_[hover_index];
  notify();
  repaint();
}

void TransposeQuantizeCallOut::mouseDrag(const MouseEvent& e) {
  int index = getHoverIndex(e);

  hover_index_ = index;
  if (hover_index_ < 0)
    return;

  if (!disabling_ && !enabling_) {
    if (selected_[hover_index_])
      disabling_ = true;
    else
      enabling_ = true;
  }

  if (disabling_ && selected_[hover_index_]) {
    selected_[hover_index_] = false;
    notify();
    repaint();
  }
  else if (enabling_ && !selected_[hover_index_]) {
    selected_[hover_index_] = true;
    notify();
    repaint();
  }
}

void TransposeQuantizeCallOut::mouseMove(const MouseEvent& e) {
  int hover_index = getHoverIndex(e);
  if (hover_index != hover_index_) {
    hover_index_ = hover_index;
    repaint();
  }
}

void TransposeQuantizeCallOut::mouseExit(const MouseEvent& e) {
  hover_index_ = -1;
  repaint();
}

void TransposeQuantizeCallOut::buttonClicked(Button* clicked_button) {
  *global_snap_ = global_snap_button_->getToggleState();
  notify();
  repaint();
}

int TransposeQuantizeCallOut::getHoverIndex(const MouseEvent& e) {
  float key_radius = key_bounds_[0].getWidth() / 2.0f;
  float key_radius2 = key_radius * key_radius;
  Point<float> position = e.position;

  for (int i = 0; i < vital::kNotesPerOctave; ++i) {
    if (position.getDistanceSquaredFrom(key_bounds_[i].getCentre()) <= key_radius2)
      return i;
  }

  return -1;
}

TransposeQuantizeButton::TransposeQuantizeButton() : OpenGlImageComponent("Transpose Quantize"),
                                                     global_snap_(false), hover_(false) {
  for (int i = 0; i < vital::kNotesPerOctave; ++i)
    selected_[i] = false;
}

void TransposeQuantizeButton::paint(Graphics& g) {
  for (int i = 0; i < vital::kNotesPerOctave; ++i) {
    if (selected_[i]) {
      if (global_snap_)
        g.setColour(findColour(Skin::kUiActionButton, true));
      else
        g.setColour(findColour(Skin::kWidgetPrimary1, true));
    }
    else
      g.setColour(findColour(Skin::kLightenScreen, true));

    g.fillEllipse(key_bounds_[i]);
  }

  if (hover_) {
    g.setColour(findColour(Skin::kLightenScreen, true));
    for (int i = 0; i < vital::kNotesPerOctave; ++i)
      g.fillEllipse(key_bounds_[i]);
  }
}

void TransposeQuantizeButton::resized() {
  setKeyBounds(key_bounds_, 0, getWidth(), getHeight());
  OpenGlImageComponent::resized();
  redrawImage(false);
}

void TransposeQuantizeButton::mouseDown(const MouseEvent& e) {
  static constexpr float kWidthMult = 4.0f;
  static constexpr float kHeightRatio = 0.6f;

  int width = getWidth() * kWidthMult;
  int height = width * kHeightRatio;

  TransposeQuantizeCallOut* quantize = new TransposeQuantizeCallOut(selected_, &global_snap_);
  quantize->addQuantizeListener(this);
  quantize->setSize(width, height);
  quantize->setParent(findParentComponentOfClass<SynthSection>());
  quantize->setLookAndFeel(DefaultLookAndFeel::instance());

  CallOutBox& callout = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(quantize), getScreenBounds(), nullptr);
  callout.setLookAndFeel(DefaultLookAndFeel::instance());
  callout.setColour(Skin::kBody, findColour(Skin::kBody, true));
  callout.setColour(Skin::kPopupBorder, findColour(Skin::kPopupBorder, true));

  hover_ = false;
  redrawImage(true);
}

void TransposeQuantizeButton::mouseEnter(const MouseEvent& e) {
  hover_ = true;
  redrawImage(true);
}

void TransposeQuantizeButton::mouseExit(const MouseEvent& e) {
  hover_ = false;
  redrawImage(true);
}

int TransposeQuantizeButton::getValue() {
  int value = 0;
  if (global_snap_)
    value = 1 << vital::kNotesPerOctave;

  for (int i = 0; i < vital::kNotesPerOctave; ++i) {
    if (selected_[i])
      value += 1 << i;
  }
  return value;
}

void TransposeQuantizeButton::setValue(int value) {
  for (int i = 0; i < vital::kNotesPerOctave; ++i)
    selected_[i] = (value >> i) & 1;

  global_snap_ = value >> vital::kNotesPerOctave;
  redrawImage(true);
}

void TransposeQuantizeButton::quantizeUpdated() {
  for (Listener* listener : listeners_)
    listener->quantizeUpdated();

  redrawImage(true);
}
