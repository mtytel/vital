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

#include "about_section.h"
#include "skin.h"
#include "fonts.h"
#include "load_save.h"
#include "paths.h"
#include "synth_gui_interface.h"
#include "synth_section.h"
#include "text_look_and_feel.h"

namespace {
  void setColorRecursively(Component *component, int color_id, const Colour& color) {
    component->setColour(color_id, color);
    for (Component *child : component->getChildren())
      setColorRecursively(child, color_id, color);
  }
}

AboutSection::AboutSection(const String& name) : Overlay(name), body_(Shaders::kRoundedRectangleFragment) {
  addOpenGlComponent(&body_);
  logo_ = std::make_unique<AppLogo>("logo");
  addOpenGlComponent(logo_.get());

  name_text_ = std::make_unique<PlainTextComponent>("plugin name", "VIAL");
  addOpenGlComponent(name_text_.get());
  name_text_->setFontType(PlainTextComponent::kRegular);
  name_text_->setTextSize(40.0f);

  version_text_ = std::make_unique<PlainTextComponent>("version", String("version  ") + ProjectInfo::versionString);
  addOpenGlComponent(version_text_.get());
  version_text_->setFontType(PlainTextComponent::kLight);
  version_text_->setTextSize(12.0f);

  size_button_extra_small_ = std::make_unique<OpenGlToggleButton>(String(100 * kMultExtraSmall) + "%");
  size_button_extra_small_->setUiButton(false);
  addAndMakeVisible(size_button_extra_small_.get());
  addOpenGlComponent(size_button_extra_small_->getGlComponent());
  size_button_extra_small_->addListener(this);

  size_button_small_ = std::make_unique<OpenGlToggleButton>(String(100 * kMultSmall) + "%");
  size_button_small_->setUiButton(false);
  addAndMakeVisible(size_button_small_.get());
  addOpenGlComponent(size_button_small_->getGlComponent());
  size_button_small_->addListener(this);

  size_button_normal_ = std::make_unique<OpenGlToggleButton>(String("100") + "%");
  size_button_normal_->setUiButton(false);
  addAndMakeVisible(size_button_normal_.get());
  addOpenGlComponent(size_button_normal_->getGlComponent());
  size_button_normal_->addListener(this);

  size_button_large_ = std::make_unique<OpenGlToggleButton>(String(100 * kMultLarge) + "%");
  size_button_large_->setUiButton(false);
  addAndMakeVisible(size_button_large_.get());
  addOpenGlComponent(size_button_large_->getGlComponent());
  size_button_large_->addListener(this);

  size_button_double_ = std::make_unique<OpenGlToggleButton>(String(100 * kMultDouble) + "%");
  size_button_double_->setUiButton(false);
  addAndMakeVisible(size_button_double_.get());
  addOpenGlComponent(size_button_double_->getGlComponent());
  size_button_double_->addListener(this);

  size_button_triple_ = std::make_unique<OpenGlToggleButton>(String(100 * kMultTriple) + "%");
  size_button_triple_->setUiButton(false);
  addAndMakeVisible(size_button_triple_.get());
  addOpenGlComponent(size_button_triple_->getGlComponent());
  size_button_triple_->addListener(this);

  size_button_quadruple_ = std::make_unique<OpenGlToggleButton>(String(100 * kMultQuadruple) + "%");
  size_button_quadruple_->setUiButton(false);
  addAndMakeVisible(size_button_quadruple_.get());
  addOpenGlComponent(size_button_quadruple_->getGlComponent());
  size_button_quadruple_->addListener(this);
}

AboutSection::~AboutSection() = default;

void AboutSection::setLogoBounds() {
  Rectangle<int> info_rect = getInfoRect();
  int left_buffer = kLeftLogoBuffer * size_ratio_;
  logo_->setBounds(info_rect.getX() + left_buffer, info_rect.getY() + (kPaddingY + 12) * size_ratio_,
                   kLogoWidth * size_ratio_, kLogoWidth * size_ratio_);
}

void AboutSection::resized() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent && device_selector_ == nullptr) {
    AudioDeviceManager* device_manager = parent->getAudioDeviceManager();
    if (device_manager) {
      device_selector_ = std::make_unique<OpenGlDeviceSelector>(
          *device_manager, 0, 0, vital::kNumChannels, vital::kNumChannels, true, false, false, false);
      addAndMakeVisible(device_selector_.get());
      addOpenGlComponent(device_selector_->getImageComponent());
    }
  }

  Rectangle<int> info_rect = getInfoRect();
  body_.setBounds(info_rect);
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));
  Colour body_text = findColour(Skin::kBodyText, true);
  name_text_->setColor(body_text);
  version_text_->setColor(body_text);
  int padding_x = size_ratio_ * kPaddingX;
  int padding_y = size_ratio_ * kPaddingY;
  int button_height = size_ratio_ * kButtonHeight;

  if (isVisible())
    setLogoBounds();

  int name_x = (kLogoWidth + kLeftLogoBuffer) * size_ratio_;
  name_text_->setBounds(info_rect.getX() + name_x, info_rect.getY() + padding_y + 40 * size_ratio_,
                        info_rect.getWidth() - name_x - kNameRightBuffer * size_ratio_, 40 * size_ratio_);

  version_text_->setBounds(info_rect.getX() + name_x, info_rect.getY() + padding_y + 76 * size_ratio_,
                           info_rect.getWidth() - name_x - kNameRightBuffer * size_ratio_, 32 * size_ratio_);

  int size_padding = 5 * size_ratio_;
  int size_start_x = info_rect.getX() + padding_x;
  int size_end_x = info_rect.getRight() - padding_x + size_padding;
  std::vector<OpenGlToggleButton*> size_buttons = {
    size_button_extra_small_.get(),
    size_button_small_.get(),
    size_button_normal_.get(),
    size_button_large_.get(),
    size_button_double_.get(),
    size_button_triple_.get(),
    size_button_quadruple_.get(),
  };

  float size_width = (size_end_x - size_start_x) * 1.0f / size_buttons.size() - size_padding;

  int size_y = version_text_->getBottom() + padding_y;

  int index = 0;
  for (OpenGlToggleButton* size_button : size_buttons) {
    int start_x = std::round(size_start_x + index * (size_width + size_padding));
    size_button->setBounds(start_x, size_y, size_width, button_height);
    index++;
  }

  if (device_selector_) {
    int y = size_button_quadruple_->getBottom() + padding_y;
    device_selector_->setBounds(info_rect.getX(), y,
                                info_rect.getWidth(), info_rect.getBottom() - y);
  }

  if (device_selector_) {
    Colour background = findColour(Skin::kPopupBackground, true); 
    setColorRecursively(device_selector_.get(), ListBox::backgroundColourId, background);
    setColorRecursively(device_selector_.get(), ComboBox::backgroundColourId, background);
    setColorRecursively(device_selector_.get(), PopupMenu::backgroundColourId, background);
    setColorRecursively(device_selector_.get(), BubbleComponent::backgroundColourId, background);

    Colour text = findColour(Skin::kBodyText, true);
    setColorRecursively(device_selector_.get(), ListBox::textColourId, text);
    setColorRecursively(device_selector_.get(), ComboBox::textColourId, text);

    setColorRecursively(device_selector_.get(), TextEditor::highlightColourId, Colours::transparentBlack);
    setColorRecursively(device_selector_.get(), ListBox::outlineColourId, Colours::transparentBlack);
    setColorRecursively(device_selector_.get(), ComboBox::outlineColourId, Colours::transparentBlack);
  }

  name_text_->setTextSize(40.0f * size_ratio_);
  version_text_->setTextSize(12.0f * size_ratio_);

  Overlay::resized();
}

void AboutSection::mouseUp(const MouseEvent &e) {
  if (!getInfoRect().contains(e.getPosition()))
    setVisible(false);
}

void AboutSection::setVisible(bool should_be_visible) {
  if (should_be_visible) {
    setLogoBounds();
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }

  Overlay::setVisible(should_be_visible);
}

void AboutSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == size_button_extra_small_.get())
    setGuiSize(kMultExtraSmall); 
  else if (clicked_button == size_button_small_.get())
    setGuiSize(kMultSmall);
  else if (clicked_button == size_button_normal_.get())
    setGuiSize(1.0);
  else if (clicked_button == size_button_large_.get())
    setGuiSize(kMultLarge);
  else if (clicked_button == size_button_double_.get())
    setGuiSize(kMultDouble);
  else if (clicked_button == size_button_triple_.get())
    setGuiSize(kMultTriple);
  else if (clicked_button == size_button_quadruple_.get())
    setGuiSize(kMultQuadruple);
}

Rectangle<int> AboutSection::getInfoRect() {
  int info_height = kBasicInfoHeight * size_ratio_;
  int info_width = kInfoWidth * size_ratio_;
  if (device_selector_)
    info_height += device_selector_->getBounds().getHeight();

  int x = (getWidth() - info_width) / 2;
  int y = (getHeight() - info_width) / 2;
  return Rectangle<int>(x, y, info_width, info_height);
}

void AboutSection::setGuiSize(float multiplier) {
  if (Desktop::getInstance().getKioskModeComponent()) {
    Desktop::getInstance().setKioskModeComponent(nullptr);
    return;
  }

  float percent = sqrtf(multiplier);
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent)
    parent->setGuiSize(percent);
}

void AboutSection::fullScreen() {
  if (Desktop::getInstance().getKioskModeComponent())
    Desktop::getInstance().setKioskModeComponent(nullptr);
  else
    Desktop::getInstance().setKioskModeComponent(getTopLevelComponent());
}
