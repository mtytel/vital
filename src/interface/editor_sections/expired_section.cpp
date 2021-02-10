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

#include "expired_section.h"

#include "skin.h"
#include "fonts.h"
#include "text_look_and_feel.h"

class OpenGlHyperlink : public OpenGlAutoImageComponent<HyperlinkButton> {
  public:
    OpenGlHyperlink(String text, URL url) : OpenGlAutoImageComponent(text, url) {
      image_component_.setComponent(this);
    }
};

ExpiredSection::ExpiredSection(String name) : Overlay(name), body_(Shaders::kRoundedRectangleFragment) {
  addOpenGlComponent(&body_);

  text_ = std::make_unique<PlainTextComponent>("text", "This Beta has expired");
  text_->setTextSize(14.0f);
  text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(text_.get());

  link_ = std::make_unique<OpenGlHyperlink>("Download a new version", URL(""));
  link_->setFont(Fonts::instance()->proportional_light().withPointHeight(14.0f),
                 false, Justification::centred);
  addAndMakeVisible(link_.get());
  addOpenGlComponent(link_->getImageComponent());
}

void ExpiredSection::resized() {
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));
  text_->setColor(findColour(Skin::kBodyText, true));

  Rectangle<int> expired_rect = getExpiredRect();
  body_.setBounds(expired_rect);
  text_->setBounds(expired_rect.getX() + kPaddingX, expired_rect.getY() + kPaddingY,
                   expired_rect.getWidth() - 2 * kPaddingX, 22);
  link_->setBounds(expired_rect.getX(), expired_rect.getY() + kPaddingY + 22, expired_rect.getWidth(), 22);

  text_->redrawImage(false);
  link_->redoImage();
  Overlay::resized();
}

void ExpiredSection::setVisible(bool should_be_visible) {
  if (should_be_visible) {
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }

  Overlay::setVisible(should_be_visible);
}

Rectangle<int> ExpiredSection::getExpiredRect() {
  int x = (getWidth() - kExpiredWidth) / 2;
  int y = getHeight() / 2 - kExpiredHeight;
  return Rectangle<int>(x, y, kExpiredWidth, kExpiredHeight);
}
