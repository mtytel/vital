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

#include "download_section.h"
#include "load_save.h"
#include "update_check_section.h"

#include "skin.h"
#include "fonts.h"

namespace {
  const std::string kDownloadUrlPrefix = "";
  const std::string kPacksUrlPrefix = "";
  const std::string kTokenUrlQuery = "?idToken=";
}

DownloadSection::DownloadSection(String name, Authentication* auth) :
    Overlay(std::move(name)), auth_(auth),
    body_(Shaders::kRoundedRectangleFragment),
    download_progress_(Shaders::kColorFragment),
    download_background_(Shaders::kColorFragment),
    install_text_background_(Shaders::kRoundedRectangleFragment),
    install_thread_(this) {

  cancel_ = false;
  initial_download_ = !LoadSave::hasDataDirectory();
  progress_value_ = 0.0f;

  addOpenGlComponent(&body_);
  addOpenGlComponent(&download_background_);
  addOpenGlComponent(&install_text_background_);
  download_progress_.addRoundedCorners();
  addOpenGlComponent(&download_progress_);

  logo_ = std::make_unique<AppLogo>("logo");
  addOpenGlComponent(logo_.get());

  loading_wheel_ = std::make_unique<LoadingWheel>();
  addOpenGlComponent(loading_wheel_.get());

  install_button_ = std::make_unique<OpenGlToggleButton>("Install");
  install_button_->setText("Install");
  install_button_->setUiButton(true);
  install_button_->addListener(this);
  install_button_->setEnabled(false);
  addAndMakeVisible(install_button_.get());
  addOpenGlComponent(install_button_->getGlComponent());

  cancel_button_ = std::make_unique<OpenGlToggleButton>("Cancel");
  cancel_button_->setText("Cancel");
  cancel_button_->setUiButton(false);
  cancel_button_->addListener(this);
  addAndMakeVisible(cancel_button_.get());
  addOpenGlComponent(cancel_button_->getGlComponent());

  download_text_ = std::make_unique<PlainTextComponent>("Download", "Downloading factory content...");
  addOpenGlComponent(download_text_.get());
  download_text_->setFontType(PlainTextComponent::kLight);
  download_text_->setTextSize(kTextHeight);
  download_text_->setJustification(Justification::centred);

  install_location_ = LoadSave::getDataDirectory();
  install_location_text_ = std::make_unique<PlainTextComponent>("Location", install_location_.getFullPathName());
  addOpenGlComponent(install_location_text_.get());
  install_location_text_->setFontType(PlainTextComponent::kLight);
  install_location_text_->setTextSize(kTextHeight);
  install_location_text_->setJustification(Justification::centredLeft);

  folder_button_ = std::make_unique<OpenGlShapeButton>("Folder");
  addAndMakeVisible(folder_button_.get());
  addOpenGlComponent(folder_button_->getGlComponent());
#if !defined(NO_TEXT_ENTRY)
  folder_button_->addListener(this);
#endif
  folder_button_->setTriggeredOnMouseDown(true);
  folder_button_->setShape(Paths::folder());

  available_packs_location_ = File::getSpecialLocation(File::tempDirectory).getChildFile("available_packs.json");
}

void DownloadSection::resized() {
  static constexpr int kLogoWidth = 128;
  static constexpr int kDownloadHeight = 8;
  static constexpr float kRingThicknessRatio = 0.03f;
  static constexpr float kRingMarginRatio = 0.03f;
  Overlay::resized();

  body_.setRounding(findValue(Skin::kBodyRounding));
  Colour background = findColour(Skin::kBackground, true);
  download_background_.setColor(background);
  install_text_background_.setColor(background);
  install_text_background_.setRounding(findValue(Skin::kWidgetRoundedCorner));
  download_progress_.setColor(findColour(Skin::kWidgetPrimary1, true));

  body_.setColor(findColour(Skin::kBody, true));

  Rectangle<int> download_rect = getDownloadRect();
  body_.setBounds(download_rect);

  int logo_width = size_ratio_ * kLogoWidth;
  int padding_x = size_ratio_ * kPaddingX;
  int padding_y = size_ratio_ * kPaddingY;
  int button_height = size_ratio_ * kButtonHeight;

  int logo_x = (getWidth() - logo_width) / 2;
  int logo_y = download_rect.getY() + padding_y;
  logo_->setBounds(logo_x, logo_y, logo_width, logo_width);

  int wheel_margin = logo_width * kRingMarginRatio;
  loading_wheel_->setBounds(logo_->getX() - wheel_margin, logo_->getY() - wheel_margin,
                            logo_->getWidth() + 2 * wheel_margin, logo_->getHeight() + 2 * wheel_margin);
  loading_wheel_->setThickness(logo_width * kRingThicknessRatio);

  float button_width = (download_rect.getWidth() - 3 * padding_x) / 2;
  install_button_->setBounds(download_rect.getX() + padding_x,
                             download_rect.getBottom() - padding_y - button_height,
                             button_width, button_height);
  cancel_button_->setBounds(install_button_->getRight() + padding_x, install_button_->getY(),
                            download_rect.getRight() - 2 * padding_x - install_button_->getRight(), button_height);

  float text_size = kTextHeight * size_ratio_;
  download_text_->setTextSize(text_size);
  float text_height = 22.0f * size_ratio_;
  download_text_->setBounds(download_rect.getX() + padding_x, logo_y + logo_width + padding_y,
                            download_rect.getWidth() - 2 * padding_x, text_height);
  int download_height = size_ratio_ * kDownloadHeight;
  int download_y = (download_text_->getBottom() + install_button_->getY() -
                    download_height + text_size - text_height) / 2;
  if (initial_download_) {
    folder_button_->setBounds(install_button_->getX(), install_button_->getY() - padding_y - button_height,
                              button_height, button_height);
    download_y = (download_text_->getBottom() + folder_button_->getY() - download_height + text_size - text_height) / 2;

    install_location_text_->setTextSize(text_size);
    int install_background_x = folder_button_->getRight() + text_height / 2;
    int install_text_x = folder_button_->getRight() + text_height;
    install_location_text_->setBounds(install_text_x, folder_button_->getY(),
                                      download_rect.getRight() - padding_x - install_text_x, button_height);
    install_text_background_.setBounds(install_background_x, folder_button_->getY(),
                                       download_rect.getRight() - padding_x - install_background_x, button_height);
  }

  download_progress_.setBounds(install_button_->getX(), download_y,
                               download_rect.getWidth() - 2 * padding_x, download_height);
  download_background_.setBounds(download_progress_.getBounds());
}

void DownloadSection::setVisible(bool should_be_visible) {
  Overlay::setVisible(should_be_visible);

  if (should_be_visible) {
    Image image(Image::ARGB, 1, 1, false);
    Graphics g(image);
    paintOpenGlChildrenBackgrounds(g);
  }
}

void DownloadSection::mouseUp(const MouseEvent &e) {
  if (install_text_background_.getBounds().contains(e.getPosition()))
    chooseInstallFolder();
}

void DownloadSection::buttonClicked(Button* clicked_button) {
  if (clicked_button == cancel_button_.get())
    cancelDownload();
  else if (clicked_button == install_button_.get())
    triggerInstall();
  else if (clicked_button == folder_button_.get())
    chooseInstallFolder();
}

void DownloadSection::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  download_progress_.setQuad(0, -1.0f, -1.0f, 2.0f * progress_value_, 2.0f);

  SynthSection::renderOpenGlComponents(open_gl, animate);
  download_progress_.renderCorners(open_gl, animate, download_progress_.getBodyColor(),
                                   download_progress_.getHeight() / 2);
}

void DownloadSection::finished(URL::DownloadTask* task, bool success) {
  if (cancel_)
    return;

  if (awaiting_install_.empty()) {
    try {
      json packs_data;
      if (success)
        packs_data = json::parse(available_packs_location_.loadFileAsString().toStdString(), nullptr, false);

      if (available_packs_location_.exists() && LoadSave::getAvailablePacksFile() != File())
        available_packs_location_.moveFileTo(LoadSave::getAvailablePacksFile());

      json available_packs = packs_data["packs"];
      json installed_packs = LoadSave::getInstalledPacks();

      MessageManagerLock lock(Thread::getCurrentThread());
      if (!lock.lockWasGained())
        return;

      for (auto& pack : available_packs) {
        bool purchased = false;
        if (pack.count("Purchased"))
          purchased = pack["Purchased"];
        int id = pack["Id"];
        std::string pack_name = pack["Name"];
        pack_name = String(pack_name).removeCharacters(" ._").toLowerCase().toStdString();

        if (purchased && installed_packs.count(std::to_string(id)) == 0 && installed_packs.count(pack_name) == 0) {
          std::string name = pack["Name"];
          std::string author = pack["Author"];
          std::string url = pack["DownloadLink"];
          LoadSave::writeErrorLog(url);

          File download_location = File::getSpecialLocation(File::tempDirectory).getChildFile(name + ".zip");
          if (!url.empty() && url[0] == '/')
            url = kDownloadUrlPrefix + url;
          awaiting_download_.emplace_back(name, author, id, URL(url), download_location);
        }
      }

      if (awaiting_download_.empty()) {
        for (Listener* listener : listeners_)
          listener->noDownloadNeeded();
      }
      else
        setVisible(true);
    }
    catch (const json::exception& e) {
      LoadSave::writeErrorLog(e.what());
    }
  }
  else {
    progress_value_ = 1.0f;
    awaiting_install_[awaiting_install_.size() - 1].finished = success;
  }

  if (!awaiting_download_.empty()) {
    DownloadPack pack = awaiting_download_[awaiting_download_.size() - 1];
    awaiting_download_.pop_back();
    awaiting_install_.push_back(pack);
    int number = static_cast<int>(awaiting_install_.size());
    int total = awaiting_download_.size() + awaiting_install_.size();

    MessageManagerLock lock(Thread::getCurrentThread());
    if (!lock.lockWasGained())
      return;

    std::string order_string = "(" + std::to_string(number) + " / " + std::to_string(total) + ")";
    download_text_->setText(pack.author + ": " + pack.name + " " + order_string);
    download_threads_.emplace_back(std::make_unique<DownloadThread>(this, pack.url, pack.download_location));
    download_threads_[download_threads_.size() - 1]->startThread();
    return;
  }

  MessageManagerLock lock(Thread::getCurrentThread());
  if (!lock.lockWasGained())
    return;

  if (success)
    install_button_->setEnabled(true);

  loading_wheel_->setActive(false);
  download_text_->setText("Downloads completed");
}

void DownloadSection::progress(URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) {
  MessageManagerLock lock(Thread::getCurrentThread());

  if (lock.lockWasGained() && !awaiting_install_.empty()) {
    double completed = bytesDownloaded;
    progress_value_ = completed / totalLength;
  }
}

Rectangle<int> DownloadSection::getDownloadRect() {
  int width = kDownloadWidth * size_ratio_;
  int height = kDownloadInitialHeight * size_ratio_;
  if (!initial_download_)
    height = kDownloadAdditionalHeight * size_ratio_;

  int x = (getWidth() - width) / 2;
  int y = kY * size_ratio_;
  return Rectangle<int>(x, y, width, height);
}

void DownloadSection::triggerDownload() {
  cancel_ = false;
  progress_value_ = 0.0f;
  awaiting_install_.clear();
  awaiting_download_.clear();
  packs_url_ = URL(kPacksUrlPrefix + kTokenUrlQuery + auth_->token());
  download_text_->setText("Getting available packs...");
  loading_wheel_->setActive(true);
  download_threads_.emplace_back(std::make_unique<DownloadThread>(this, packs_url_, available_packs_location_));
  download_threads_[download_threads_.size() - 1]->startThread();
}

void DownloadSection::triggerInstall() {
  install_location_.createDirectory();
  File errors_file = install_location_.getChildFile("errors.txt");
  errors_file.create();

  if (!install_location_.exists() || !errors_file.exists() || !errors_file.hasWriteAccess()) {
    MessageManagerLock lock(Thread::getCurrentThread());
    String warning = "Can't create install directory. Select another destination";
    AlertWindow::showNativeDialogBox("Can't Create Directory", warning, false);
    install_button_->setEnabled(true);
    cancel_button_->setEnabled(true);
    return;
  }

  loading_wheel_->setActive(true);
  download_text_->setText("Installing...");
  install_button_->setEnabled(false);
  cancel_button_->setEnabled(false);
  install_thread_.startThread();
}

void DownloadSection::startDownload(Thread* thread, URL& url, const File& dest) {
  download_tasks_.emplace_back(url.downloadToFile(dest, "", this));
}

void DownloadSection::startInstall(Thread* thread) {
  std::vector<int> installed;
  LoadSave::saveDataDirectory(install_location_);

  for (const DownloadPack& pack : awaiting_install_) {
    if (!pack.download_location.exists())
      LoadSave::writeErrorLog("Install Error: Pack file moved or is missing.");

    if (!pack.finished)
      LoadSave::writeErrorLog("Install Error: Pack didn't download correctly");

    if (!pack.finished || !pack.download_location.exists())
      continue;

    ZipFile zip(pack.download_location);
    if (zip.getNumEntries() <= 0)
      LoadSave::writeErrorLog("Unzipping Error: no entries");
    else {
      Result unzip_result = zip.uncompressTo(install_location_);
      if (!unzip_result.wasOk())
        LoadSave::writeErrorLog("Unzipping Error: " + unzip_result.getErrorMessage());
      else
        installed.push_back(pack.id);
    }

    pack.download_location.deleteFile();
  }

  MessageManagerLock lock(Thread::getCurrentThread());
  if (!lock.lockWasGained())
    return;
  for (int installed_pack_id : installed)
    LoadSave::markPackInstalled(installed_pack_id);

  for (Listener* listener : listeners_)
    listener->dataDirectoryChanged();

  loading_wheel_->completeRing();
  download_text_->setText("All done!");

  startTimer(kCompletionWaitMs);
}

void DownloadSection::cancelDownload() {
  cancel_ = true;
  download_tasks_.clear();
  setVisible(false);
}

void DownloadSection::chooseInstallFolder() {
  FileChooser open_box("Choose Install Directory", install_location_);
  if (open_box.browseForDirectory()) {
    File result = open_box.getResult();
    if (result.getFileName() != "Vial")
      result = result.getChildFile("Vial");

    result.createDirectory();
    File errors_file = result.getChildFile("errors.txt");
    errors_file.create();

    if (result.exists() && errors_file.exists() && errors_file.hasWriteAccess()) {
      install_location_ = result;
      install_location_text_->setText(install_location_.getFullPathName());
    }
    else {
      String warning = "Can't create install directory. Select another destination";
      AlertWindow::showNativeDialogBox("Invalid Directory", warning, false);
    }
  }
}
