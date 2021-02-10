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

#include "preset_selector.h"

#include "skin.h"
#include "default_look_and_feel.h"
#include "fonts.h"

PresetSelector::PresetSelector() : SynthSection("preset_selector"),
                                   font_height_ratio_(kDefaultFontHeightRatio),
                                   round_amount_(0.0f), hover_(false), text_component_(false) {
  static const PathStrokeType arrow_stroke(0.05f, PathStrokeType::JointStyle::curved,
                                           PathStrokeType::EndCapStyle::rounded);

  text_ = std::make_unique<PlainTextComponent>("Text", "Init");
  text_->setFontType(PlainTextComponent::kTitle);
  text_->setInterceptsMouseClicks(false, false);
  addOpenGlComponent(text_.get());
  text_->setScissor(true);
  Path prev_line, prev_shape, next_line, next_shape;

  prev_preset_ = std::make_unique<OpenGlShapeButton>("Prev");
  addAndMakeVisible(prev_preset_.get());
  addOpenGlComponent(prev_preset_->getGlComponent());
  prev_preset_->addListener(this);
  prev_line.startNewSubPath(0.65f, 0.3f);
  prev_line.lineTo(0.35f, 0.5f);
  prev_line.lineTo(0.65f, 0.7f);

  arrow_stroke.createStrokedPath(prev_shape, prev_line);
  prev_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
  prev_shape.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
  prev_preset_->setShape(prev_shape);

  next_preset_ = std::make_unique<OpenGlShapeButton>("Next");
  addAndMakeVisible(next_preset_.get());
  addOpenGlComponent(next_preset_->getGlComponent());
  next_preset_->addListener(this);
  next_line.startNewSubPath(0.35f, 0.3f);
  next_line.lineTo(0.65f, 0.5f);
  next_line.lineTo(0.35f, 0.7f);

  arrow_stroke.createStrokedPath(next_shape, next_line);
  next_shape.addLineSegment(Line<float>(0.0f, 0.0f, 0.0f, 0.0f), 0.2f);
  next_shape.addLineSegment(Line<float>(1.0f, 1.0f, 1.0f, 1.0f), 0.2f);
  next_preset_->setShape(next_shape);
}

PresetSelector::~PresetSelector() { }

void PresetSelector::paintBackground(Graphics& g) {
  float round_amount = findValue(Skin::kWidgetRoundedCorner);
  g.setColour(findColour(Skin::kPopupSelectorBackground, true));
  g.fillRoundedRectangle(0, 0, getWidth(), getHeight(), round_amount);
}

void PresetSelector::resized() {
  SynthSection::resized();

  if (text_component_) {
    SynthSection* parent = findParentComponentOfClass<SynthSection>();
    int button_height = parent->findValue(Skin::kTextComponentFontSize);
    int offset = parent->findValue(Skin::kTextComponentOffset);
    int button_y = (getHeight() - button_height) / 2 + offset;
    prev_preset_->setBounds(0, button_y, button_height, button_height);
    next_preset_->setBounds(getWidth() - button_height, button_y, button_height, button_height);
    text_->setBounds(getLocalBounds().translated(0, offset));
    text_->setTextSize(button_height);
  }
  else {
    int height = getHeight();
    text_->setBounds(Rectangle<int>(height, 0, getWidth() - 2 * height, height));
    text_->setTextSize(height * font_height_ratio_);
    prev_preset_->setBounds(0, 0, height, height);
    next_preset_->setBounds(getWidth() - height, 0, height, height);
    text_->setColor(findColour(Skin::kPresetText, true));
  }
}

void PresetSelector::mouseDown(const MouseEvent& e) {
  textMouseDown(e);
}

void PresetSelector::mouseUp(const MouseEvent& e) {
  textMouseUp(e);
}

void PresetSelector::buttonClicked(Button* clicked_button) {
  if (clicked_button == prev_preset_.get())
    clickPrev();
  else if (clicked_button == next_preset_.get())
    clickNext();
}

void PresetSelector::setText(String text) {
  text_->setText(text);
}

void PresetSelector::setText(String left, String center, String right) {
  text_->setText(left + "  " + center + "  " + right);
}

void PresetSelector::clickPrev() {
  for (Listener* listener : listeners_)
    listener->prevClicked();
}

void PresetSelector::clickNext() {
  for (Listener* listener : listeners_)
    listener->nextClicked();
}

void PresetSelector::textMouseDown(const MouseEvent& e) {
  for (Listener* listener : listeners_)
    listener->textMouseDown(e);
}

void PresetSelector::textMouseUp(const MouseEvent& e) {
  for (Listener* listener : listeners_)
    listener->textMouseUp(e);
}
