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

#include "synth_preset_selector.h"

#include "paths.h"
#include "skin.h"
#include "default_look_and_feel.h"
#include "fonts.h"
#include "load_save.h"
#include "full_interface.h"
#include "save_section.h"
#include "synth_gui_interface.h"
#include "tuning.h"

namespace {
  void menuCallback(int result, SynthPresetSelector* preset_selector) {
    if (preset_selector == nullptr)
      return;
    if (result == SynthPresetSelector::kInitPreset)
      preset_selector->initPreset();
    else if (result == SynthPresetSelector::kImportPreset)
      preset_selector->importPreset();
    else if (result == SynthPresetSelector::kExportPreset)
      preset_selector->exportPreset();
    else if (result == SynthPresetSelector::kImportBank)
      preset_selector->importBank();
    else if (result == SynthPresetSelector::kExportBank)
      preset_selector->exportBank();
    else if (result == SynthPresetSelector::kSavePreset)
      preset_selector->savePreset();
    else if (result == SynthPresetSelector::kBrowsePresets)
      preset_selector->browsePresets();
    else if (result == SynthPresetSelector::kLoadTuning)
      preset_selector->loadTuningFile();
    else if (result == SynthPresetSelector::kClearTuning)
      preset_selector->clearTuning();
    else if (result == SynthPresetSelector::kOpenSkinDesigner)
      preset_selector->openSkinDesigner();
    else if (result == SynthPresetSelector::kLoadSkin)
      preset_selector->loadSkin();
    else if (result == SynthPresetSelector::kClearSkin)
      preset_selector->clearSkin();
    else if (result == SynthPresetSelector::kLogOut)
      preset_selector->signOut();
    else if (result == SynthPresetSelector::kLogIn)
      preset_selector->signIn();
  }

  String redactEmail(const String& email) {
    static constexpr int kLeaveCharacters = 2;

    StringArray tokens;
    tokens.addTokens(email, "@", "");
    String beginning = tokens[0];
    String result = beginning.substring(0, 2);
    for (int i = kLeaveCharacters; i < beginning.length(); ++i)
      result += "*";

    return result + "@" + tokens[1];
  }
}

SynthPresetSelector::SynthPresetSelector() : SynthSection("preset_selector"), bank_exporter_(nullptr),
                                             browser_(nullptr), save_section_(nullptr), modified_(false) {
  static const PathStrokeType arrow_stroke(0.05f, PathStrokeType::JointStyle::curved,
                                           PathStrokeType::EndCapStyle::rounded);
  full_skin_ = std::make_unique<Skin>();
  selector_ = std::make_unique<PresetSelector>();
  addSubSection(selector_.get());
  selector_->addListener(this);

  menu_button_ = std::make_unique<OpenGlShapeButton>("Menu");
  addAndMakeVisible(menu_button_.get());
  addOpenGlComponent(menu_button_->getGlComponent());
  menu_button_->addListener(this);
  menu_button_->setTriggeredOnMouseDown(true);
  menu_button_->setShape(Paths::menu());

  save_button_ = std::make_unique<OpenGlShapeButton>("Save");
  addAndMakeVisible(save_button_.get());
  addOpenGlComponent(save_button_->getGlComponent());
  save_button_->addListener(this);

  save_button_->setShape(Paths::save());
}

SynthPresetSelector::~SynthPresetSelector() {
  skin_designer_.deleteAndZero();
}

void SynthPresetSelector::resized() {
  static constexpr float kSelectorButtonPaddingHeightPercent = 0.2f;

  int height = getHeight();
  selector_->setRoundAmount(height / 2.0f);
  int padding = kSelectorButtonPaddingHeightPercent * height;
  selector_->setBounds(0, 0, getWidth() - 2 * height - padding, height);
  save_button_->setBounds(getWidth() - 2 * height, 0, height, height);
  save_button_->setShape(Paths::save(height));
  menu_button_->setBounds(getWidth() - height, 0, height, height);
  menu_button_->setShape(Paths::menu(height));

  resetText();

  Component::resized();
}

void SynthPresetSelector::buttonClicked(Button* clicked_button) {
  if (clicked_button == menu_button_.get()) {
    if (ModifierKeys::getCurrentModifiersRealtime().isAltDown())
      showAlternatePopupMenu(menu_button_.get());
    else
      showPopupMenu(menu_button_.get());
  }
  else if (clicked_button == save_button_.get())
    savePreset();
}

void SynthPresetSelector::newPresetSelected(File preset) {
  browser_->clearExternalPreset();
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::string error;
  bool result = parent->getSynth()->loadFromFile(preset, error);
  if (result)
    resetText();
  else {
    error = "There was an error open the preset. " + error;
    AlertWindow::showNativeDialogBox("Error opening preset", error, false);
  }
}

void SynthPresetSelector::deleteRequested(File preset) {
  for (Listener* listener : listeners_)
    listener->deleteRequested(preset);
}

void SynthPresetSelector::hidePresetBrowser() {
  for (Listener* listener : listeners_)
    listener->setPresetBrowserVisibility(false);
}

void SynthPresetSelector::hideBankExporter() {
  for (Listener* listener : listeners_)
    listener->setBankExporterVisibility(false);
}

void SynthPresetSelector::resetText() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  String preset_text = parent->getSynth()->getPresetName();
  if (preset_text == "")
    preset_text = TRANS("Init Preset");

  if (modified_)
    preset_text = "*" + preset_text;

  selector_->setText(preset_text);
  repaint();
}

void SynthPresetSelector::prevClicked() {
  if (browser_)
    browser_->loadPrevPreset();
}

void SynthPresetSelector::nextClicked() {
  if (browser_)
    browser_->loadNextPreset();
}

void SynthPresetSelector::textMouseUp(const MouseEvent& e) {
  if (e.mods.isPopupMenu())
    showPopupMenu(selector_.get());
  else if (browser_)
    setPresetBrowserVisibile(!browser_->isVisible());
}

void SynthPresetSelector::showPopupMenu(Component* anchor) {
  PopupItems options;
  options.addItem(kBrowsePresets, "Browse Presets");
  options.addItem(kSavePreset, "Save Preset");
  options.addItem(kImportPreset, "Open External Preset");
  options.addItem(kExportPreset, "Export Preset");
  options.addItem(kImportBank, "Import Bank");
  options.addItem(kExportBank, "Export Bank");
  options.addItem(-1, "");
  options.addItem(kInitPreset, "Initialize Preset");
  options.addItem(-1, "");
  options.addItem(kLoadTuning, "Load Tuning File");
  if (!hasDefaultTuning())
    options.addItem(kClearTuning, "Clear Tuning: " + getTuningName());
  
  options.addItem(-1, "");
  std::string logged_in_as = loggedInName();
  if (logged_in_as.empty())
    options.addItem(kLogIn, "Log in");
  else
    options.addItem(kLogOut, "Log out - " + redactEmail(logged_in_as).toStdString());

  if (LoadSave::getDefaultSkin().exists()) {
    options.addItem(-1, "");
    options.addItem(kClearSkin, "Load Default Skin");
  }

  showPopupSelector(this, Point<int>(anchor->getX(), anchor->getBottom()), options,
                    [=](int selection) { menuCallback(selection, this); });
}

void SynthPresetSelector::showAlternatePopupMenu(Component* anchor) {
  PopupItems options;
  options.addItem(kOpenSkinDesigner, "Open Skin Designer");
  options.addItem(kLoadSkin, "Load Skin");

  if (LoadSave::getDefaultSkin().exists())
    options.addItem(kClearSkin, "Load Default Skin");

  showPopupSelector(this, Point<int>(anchor->getX(), anchor->getBottom()), options,
                    [=](int selection) { menuCallback(selection, this); });
}

void SynthPresetSelector::setModified(bool modified) {
  if (modified_ == modified)
    return;

  modified_ = modified;
  String text = selector_->getText();
  if (modified_ && text.length() && text[0] != '*')
    selector_->setText("*" + text);
  else if (!modified_ && text.length() && text[0] == '*')
    selector_->setText(text.substring(1));
}

void SynthPresetSelector::loadFromFile(File& preset) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  std::string error;
  parent->getSynth()->loadFromFile(preset, error);
}

void SynthPresetSelector::setPresetBrowserVisibile(bool visible) {
  for (Listener* listener : listeners_)
    listener->setPresetBrowserVisibility(visible);
}

void SynthPresetSelector::makeBankExporterVisibile() {
  for (Listener* listener : listeners_)
    listener->setBankExporterVisibility(true);
}

void SynthPresetSelector::initPreset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  parent->getSynth()->loadInitPreset();
  if (browser_)
    browser_->externalPresetLoaded(File());
  parent->updateFullGui();
  parent->notifyFresh();
  resetText();
}

void SynthPresetSelector::savePreset() {
  if (save_section_) {
    save_section_->setIsPreset(true);
    save_section_->setVisible(true);
  }
}

void SynthPresetSelector::importPreset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  File active_file = parent->getSynth()->getActiveFile();
  FileChooser open_box("Open Preset", active_file, String("*.") + vital::kPresetExtension);
  if (!open_box.browseForFileToOpen())
    return;
  
  File choice = open_box.getResult();
  if (!choice.exists())
    return;
  
  std::string error;
  if (!parent->getSynth()->loadFromFile(choice, error)) {
    std::string name = ProjectInfo::projectName;
    error = "There was an error open the preset. " + error;
    AlertWindow::showNativeDialogBox("Error opening preset", error, false);
  }
  else
    parent->externalPresetLoaded(choice);
}

void SynthPresetSelector::exportPreset() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  SynthBase* synth = parent->getSynth();
  File active_file = synth->getActiveFile();
  FileChooser save_box("Export Preset", File(), String("*.") + vital::kPresetExtension);
  if (!save_box.browseForFileToSave(true))
    return;
  
  synth->saveToFile(save_box.getResult().withFileExtension(vital::kPresetExtension));
  parent->externalPresetLoaded(synth->getActiveFile());
}

void SynthPresetSelector::importBank() {
  FileChooser import_box("Import Bank", File(), String("*.") + vital::kBankExtension);
  if (import_box.browseForFileToOpen()) {
    File result = import_box.getResult();
    FileInputStream input_stream(result);
    if (input_stream.openedOk()) {
      File data_directory = LoadSave::getDataDirectory();
      data_directory.createDirectory();
      if (!LoadSave::hasDataDirectory())
        LoadSave::saveDataDirectory(data_directory);

      ZipFile import_zip(input_stream);
      Result unzip_result = import_zip.uncompressTo(data_directory);
      if (unzip_result == Result::ok())
        LoadSave::markPackInstalled(result.getFileNameWithoutExtension().toStdString());
      else
        LoadSave::writeErrorLog("Unzipping bank failed!");

      for (Listener* listener : listeners_)
        listener->bankImported();
    }
    else
      LoadSave::writeErrorLog("Opening file stream to bank failed!");
  }
}

void SynthPresetSelector::exportBank() {
  makeBankExporterVisibile();
}

void SynthPresetSelector::loadTuningFile() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  FileChooser load_box("Load Tuning", File(), Tuning::allFileExtensions());
  if (load_box.browseForFileToOpen())
    parent->getSynth()->loadTuningFile(load_box.getResult());
}

void SynthPresetSelector::clearTuning() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  parent->getSynth()->getTuning()->setDefaultTuning();
}

std::string SynthPresetSelector::getTuningName() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  return parent->getSynth()->getTuning()->getName();
}

bool SynthPresetSelector::hasDefaultTuning() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  return parent->getSynth()->getTuning()->isDefault();
}

std::string SynthPresetSelector::loggedInName() {
  FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
  if (full_interface)
    return full_interface->getSignedInName();
  return "";
}

void SynthPresetSelector::signOut() {
  FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
  return full_interface->signOut();
}

void SynthPresetSelector::signIn() {
  FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
  return full_interface->signIn();
}

void SynthPresetSelector::openSkinDesigner() {
  skin_designer_.deleteAndZero();
  SkinDesigner* skin_designer = new SkinDesigner(full_skin_.get(), findParentComponentOfClass<FullInterface>());
  RectanglePlacement placement(RectanglePlacement::xLeft |
                               RectanglePlacement::yTop |
                               RectanglePlacement::doNotResize);

  Rectangle<int> area(0, 0, 700, 800);
  Rectangle<int> bounds = Desktop::getInstance().getDisplays().getMainDisplay().userArea.reduced(20);
  Rectangle<int> bounds_result(placement.appliedTo(area, bounds));

  skin_designer->setBounds(bounds_result);
  skin_designer->setResizable(true, false);
  skin_designer->setUsingNativeTitleBar(true);
  skin_designer->setVisible(true);
  skin_designer_ = skin_designer;
}

void SynthPresetSelector::loadSkin() {
  FileChooser open_box("Open Skin", File(), String("*.") + vital::kSkinExtension);
  if (open_box.browseForFileToOpen()) {
    File loaded = open_box.getResult();
    loaded.copyFileTo(LoadSave::getDefaultSkin());
    full_skin_->loadFromFile(loaded);
    repaintWithSkin();
  }
}

void SynthPresetSelector::clearSkin() {
  File default_skin = LoadSave::getDefaultSkin();
  if (default_skin.exists() && default_skin.hasWriteAccess())
    default_skin.deleteFile();

  full_skin_->loadDefaultSkin();
  repaintWithSkin();
}

void SynthPresetSelector::repaintWithSkin() {
  FullInterface* full_interface = findParentComponentOfClass<FullInterface>();
  full_interface->reloadSkin(*full_skin_);
}

void SynthPresetSelector::browsePresets() {
  if (browser_)
    setPresetBrowserVisibile(true);
}
