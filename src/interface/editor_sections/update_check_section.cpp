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

#include "update_check_section.h"

#include "fonts.h"
#include "load_save.h"

UpdateMemory::UpdateMemory() {
  checkers_ = 0;
  if (!LoadSave::shouldCheckForUpdates())
    checkers_ = 1;
}

UpdateMemory::~UpdateMemory() {
  clearSingletonInstance();
}

JUCE_IMPLEMENT_SINGLETON(UpdateMemory)

UpdateCheckSection::UpdateCheckSection(String name) : Overlay(name), version_request_(this), 
                                                      body_(Shaders::kRoundedRectangleFragment) {
  addOpenGlComponent(&body_);

  notify_text_ = std::make_unique<PlainTextComponent>("notify", "There is a new version of Vital!");
  notify_text_->setTextSize(20.0f);
  notify_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(notify_text_.get());

  version_text_ = std::make_unique<PlainTextComponent>("version", "");
  version_text_->setTextSize(14.0f);
  version_text_->setFontType(PlainTextComponent::kLight);
  addOpenGlComponent(version_text_.get());

  download_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Download"));
  download_button_->addListener(this);
  download_button_->setUiButton(true);
  addAndMakeVisible(download_button_.get());
  addOpenGlComponent(download_button_->getGlComponent());

  nope_button_ = std::make_unique<OpenGlToggleButton>(TRANS("Ignore"));
  nope_button_->addListener(this);
  nope_button_->setUiButton(false);
  addAndMakeVisible(nope_button_.get());
  addOpenGlComponent(nope_button_->getGlComponent());

  content_update_ = false;
}

void UpdateCheckSection::resized() {
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));

  Colour text_color = findColour(Skin::kBodyText, true);
  notify_text_->setColor(text_color);
  version_text_->setColor(text_color);

  Rectangle<int> update_rect = getUpdateCheckRect();
  body_.setBounds(update_rect);

  int text_width = update_rect.getWidth() - 2 * kPaddingX;
  int height = 32;
  notify_text_->setBounds(update_rect.getX() + kPaddingX, update_rect.getY() + kPaddingY, text_width, height);
  version_text_->setBounds(update_rect.getX() + kPaddingX, update_rect.getY() + kPaddingY + height, text_width, height);

  float button_width = (update_rect.getWidth() - 3 * kPaddingX) / 2.0f;
  download_button_->setBounds(update_rect.getX() + kPaddingX,
                              update_rect.getBottom() - kPaddingY - kButtonHeight,
                              button_width, kButtonHeight);
  nope_button_->setBounds(update_rect.getX() + button_width + 2 * kPaddingX,
                          update_rect.getBottom() - kPaddingY - kButtonHeight,
                          button_width, kButtonHeight);
 
  Overlay::resized();
}

void UpdateCheckSection::setVisible(bool should_be_visible) {
  Overlay::setVisible(should_be_visible);

  if (should_be_visible) {
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }
}

void UpdateCheckSection::updateContent(String version) {
}

void UpdateCheckSection::needsUpdate() {
  for (Listener* listener : listeners_)
    listener->needsUpdate();
}

void UpdateCheckSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == download_button_.get()) {
    if (content_update_)
      updateContent(content_version_);
    else
      URL("").launchInDefaultBrowser();
  }
  setVisible(false);
}

void UpdateCheckSection::mouseUp(const MouseEvent &e) {
  if (!getUpdateCheckRect().contains(e.getPosition()))
    setVisible(false);
}

void UpdateCheckSection::finished(URL::DownloadTask* task, bool success) {
  if (!success)
    return;

  StringArray versions;
  version_file_.readLines(versions);
  if (versions.size() < 2)
    return;

  MessageManagerLock lock(&version_request_);
  if (!lock.lockWasGained())
    return;

  app_version_ = versions[0];
  content_version_ = versions[1];

  if (!app_version_.isEmpty() && LoadSave::compareVersionStrings(ProjectInfo::versionString, app_version_) < 0) {
    version_text_->setText(String("Version: ") + app_version_);
    needsUpdate();
  }
}

void UpdateCheckSection::checkUpdate() {
  URL version_url("");
  version_file_ = File::getSpecialLocation(File::tempDirectory).getChildFile("vital_versions.txt");
  download_task_ = version_url.downloadToFile(version_file_, "", this);
}

void UpdateCheckSection::checkContentUpdate() {
  String content_version = LoadSave::loadContentVersion();
  if (!content_version_.isEmpty() && LoadSave::compareVersionStrings(content_version, content_version_) < 0) {
    notify_text_->setText("There is new preset content available");
    version_text_->setText(String("Version: ") + content_version_);
    content_update_ = true;
    needsUpdate();
  }
}

Rectangle<int> UpdateCheckSection::getUpdateCheckRect() {
  int x = (getWidth() - kUpdateCheckWidth) / 2;
  int y = (getHeight() - kUpdateCheckHeight) / 2;
  return Rectangle<int>(x, y, kUpdateCheckWidth, kUpdateCheckHeight);
}
