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

#include "popup_browser.h"

#include "skin.h"
#include "fonts.h"
#include "load_save.h"
#include "paths.h"
#include "open_gl_component.h"
#include "synth_section.h"

namespace {
  template<class Comparator>
  void sortSelectionArray(Array<File>& selection_array) {
    Comparator comparator;
    selection_array.sort(comparator, true);
  }

  const std::string kAddFolderName = "Add Folder";
  const std::string kStoreUrl = "";
  constexpr int kMaxRootFiles = 8000;

  bool isAcceptableRoot(const File& file) {
    std::list<File> folders;
    folders.push_back(file);
    int num_files = 0;

    while (!folders.empty()) {
      File current_file = folders.back();
      folders.pop_back();

      num_files += current_file.getNumberOfChildFiles(File::findFiles);
      if (num_files > kMaxRootFiles)
        return false;

      Array<File> sub_folders = current_file.findChildFiles(File::findDirectories, false);

      for (const File& folder : sub_folders)
        folders.push_back(folder);

    }
    return true;
  }

  void showAddRootWarning() {
    String error = String("Folder has too many files to add to browser. Max: ") + String(kMaxRootFiles);
    NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Error Adding Folder", error);
  }
}

PopupDisplay::PopupDisplay() : SynthSection("Popup Display"), text_("Popup Text", ""),
                               body_(Shaders::kRoundedRectangleFragment),
                               border_(Shaders::kRoundedRectangleBorderFragment) {
  addOpenGlComponent(&body_);
  addOpenGlComponent(&border_);
  addOpenGlComponent(&text_);

  text_.setJustification(Justification::centred);
  text_.setFontType(PlainTextComponent::kLight);

  setSkinOverride(Skin::kPopupBrowser);
}

void PopupDisplay::resized() {
  Rectangle<int> bounds = getLocalBounds();
  int rounding = findValue(Skin::kBodyRounding);

  body_.setBounds(bounds);
  body_.setRounding(rounding);
  body_.setColor(findColour(Skin::kBody, true));

  border_.setBounds(bounds);
  border_.setRounding(rounding);
  border_.setThickness(1.0f, true);
  border_.setColor(findColour(Skin::kBorder, true));

  text_.setBounds(bounds);
  text_.setColor(findColour(Skin::kBodyText, true));
}

void PopupDisplay::setContent(const std::string& text, Rectangle<int> bounds,
                              BubbleComponent::BubblePlacement placement) {
  static constexpr int kHeight = 24;

  int height = kHeight * size_ratio_;
  int mult = getPixelMultiple();
  Font font = Fonts::instance()->proportional_light().withPointHeight(height * 0.5f * mult);
  int padding = height / 4;
  int buffer = padding * 2 + 2;
  int width = (font.getStringWidth(text) / getPixelMultiple()) + buffer;

  int middle_x = bounds.getX() + bounds.getWidth() / 2;
  int middle_y = bounds.getY() + bounds.getHeight() / 2;

  if (placement == BubbleComponent::above)
    setBounds(middle_x - width / 2, bounds.getY() - height, width, height);
  else if (placement == BubbleComponent::below)
    setBounds(middle_x - width / 2, bounds.getBottom(), width, height);
  else if (placement == BubbleComponent::left)
    setBounds(bounds.getX() - width, middle_y - height / 2, width, height);
  else if (placement == BubbleComponent::right)
    setBounds(bounds.getRight(), middle_y - height / 2, width, height);

  text_.setText(text);
  text_.setTextSize(height * 0.5f);
}

PopupList::PopupList() : SynthSection("Popup List"),
                         selected_(-1), hovered_(-1), show_selected_(false), view_position_(0.0f),
                         highlight_(Shaders::kColorFragment), hover_(Shaders::kColorFragment) {
  highlight_.setTargetComponent(this);
  highlight_.setAdditive(true);

  hover_.setTargetComponent(this);
  hover_.setAdditive(true);

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);
}

void PopupList::resized() {
  Colour lighten = findColour(Skin::kLightenScreen, true);
  scroll_bar_->setColor(lighten);

  if (getScrollableRange() > getHeight()) {
    int scroll_bar_width = kScrollBarWidth * getSizeRatio();
    int scroll_bar_height = getHeight();
    scroll_bar_->setVisible(true);
    scroll_bar_->setBounds(getWidth() - scroll_bar_width, 0, scroll_bar_width, scroll_bar_height);
    setScrollBarRange();
  }
  else
    scroll_bar_->setVisible(false);

  redoImage();
}

void PopupList::setSelections(PopupItems selections) {
  selections_ = std::move(selections);
  selected_ = std::min(selected_, selections_.size() - 1);
  hovered_ = std::min(hovered_, selections_.size() - 1);
  for (int i = 0; i < selections_.size(); ++i) {
    if (selections_.items[i].selected)
      selected_ = i;
  }
  resized();
}

int PopupList::getRowFromPosition(float mouse_position) {
  int index = floorf((mouse_position + getViewPosition()) / getRowHeight());
  if (index < selections_.size() && index >= 0 && selections_.items[index].id < 0)
    return -1;
  return index;
}

int PopupList::getBrowseWidth() {
  static constexpr int kMinWidth = 150;

  Font font = getFont();
  int max_width = kMinWidth * size_ratio_;
  int buffer = getTextPadding() * 2 + 2;
  for (int i = 0; i < selections_.size(); ++i)
    max_width = std::max(max_width, font.getStringWidth(selections_.items[i].name) / getPixelMultiple() + buffer);

  return max_width;
}

void PopupList::mouseMove(const MouseEvent& e) {
  int row = getRowFromPosition(e.position.y);
  if (row >= selections_.size() || row < 0)
    row = -1;
  hovered_ = row;
}

void PopupList::mouseDrag(const MouseEvent& e) {
  int row = getRowFromPosition(e.position.y);
  if (e.position.x < 0 || e.position.x > getWidth() || row >= selections_.size() || row < 0)
    row = -1;
  hovered_ = row;
}

void PopupList::mouseExit(const MouseEvent& e) {
  hovered_ = -1;
}

int PopupList::getSelection(const MouseEvent& e) {
  float click_y_position = e.position.y;
  int row = getRowFromPosition(click_y_position);
  if (row < selections_.size() && row >= 0)
    return row;

  return -1;
}

void PopupList::mouseUp(const MouseEvent& e) {
  if (e.position.x < 0 || e.position.x > getWidth())
    return;

  select(getSelection(e));
}

void PopupList::mouseDoubleClick(const MouseEvent& e) {
  int selection = getSelection(e);
  if (selection != selected_ || selection < 0)
    return;

  for (Listener* listener : listeners_)
    listener->doubleClickedSelected(this, selections_.items[selection].id, selection);
}

void PopupList::select(int selection) {
  if (selection < 0 || selection >= selections_.size())
    return;

  selected_ = selection;
  for (int i = 0; i < selections_.size(); ++i)
    selections_.items[i].selected = false;
  selections_.items[selected_].selected = true;

  for (Listener* listener : listeners_)
    listener->newSelection(this, selections_.items[selection].id, selection);
}

void PopupList::initOpenGlComponents(OpenGlWrapper& open_gl) {
  rows_.init(open_gl);
  rows_.setColor(Colours::white);

  highlight_.init(open_gl);
  hover_.init(open_gl);
  SynthSection::initOpenGlComponents(open_gl);
}

void PopupList::redoImage() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int mult = getPixelMultiple();
  int row_height = getRowHeight() * mult;
  int image_width = getWidth() * mult;

  Colour text_color = findColour(Skin::kTextComponentText, true);
  Colour lighten = findColour(Skin::kLightenScreen, true);
  int image_height = std::max(row_height * selections_.size(), getHeight());
  Image rows_image(Image::ARGB, image_width, image_height, true);
  Graphics g(rows_image);
  g.setColour(text_color);
  g.setFont(getFont());

  int padding = getTextPadding();
  int width = (getWidth() - 2 * padding) * mult;
  for (int i = 0; i < selections_.size(); ++i) {
    if (selections_.items[i].id < 0) {
      g.setColour(lighten);
      int y = row_height * (i + 0.5f);
      g.drawRect(padding, y, width, 1);
    }
    else {
      g.setColour(text_color);
      String name = selections_.items[i].name;
      g.drawText(name, padding, row_height * i, width, row_height, Justification::centredLeft, true);
    }
  }
  rows_.setOwnImage(rows_image);
}

void PopupList::moveQuadToRow(OpenGlQuad& quad, int row) {
  int row_height = getRowHeight();
  float view_height = getHeight();
  float open_gl_row_height = 2.0f * row_height / view_height;
  float offset = row * open_gl_row_height - 2.0f * getViewPosition() / view_height;

  float y = 1.0f - offset;
  quad.setQuad(0, -1.0f, y - open_gl_row_height, 2.0f, open_gl_row_height);
}

void PopupList::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  Rectangle<int> view_bounds(getLocalBounds());
  OpenGlComponent::setViewPort(this, view_bounds, open_gl);

  float image_width = vital::utils::nextPowerOfTwo(getWidth());
  float image_height = vital::utils::nextPowerOfTwo(rows_.getImageHeight());
  float width_ratio = image_width / getWidth();
  float height_ratio = image_height / (getPixelMultiple() * getHeight());
  float y_offset = 2.0f * getViewPosition() / getHeight();

  rows_.setTopLeft(-1.0f, 1.0f + y_offset);
  rows_.setTopRight(2.0f * width_ratio - 1.0f, 1.0f + y_offset);
  rows_.setBottomLeft(-1.0f, 1.0f + y_offset - 2.0f * height_ratio);
  rows_.setBottomRight(2.0f * width_ratio - 1.0f, 1.0f + y_offset - 2.0f * height_ratio);
  rows_.drawImage(open_gl);

  if (hovered_ >= 0) {
    moveQuadToRow(hover_, hovered_);
    if (show_selected_)
      hover_.setColor(findColour(Skin::kLightenScreen, true));
    else
      hover_.setColor(findColour(Skin::kWidgetPrimary1, true).darker(0.8f));
    hover_.render(open_gl, animate);
  }
  if (selected_ >= 0 && show_selected_) {
    moveQuadToRow(highlight_, selected_);
    highlight_.setColor(findColour(Skin::kWidgetPrimary1, true).darker(0.8f));
    highlight_.render(open_gl, animate);
  }

  SynthSection::renderOpenGlComponents(open_gl, animate);
}

void PopupList::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  rows_.destroy(open_gl);

  highlight_.destroy(open_gl);
  hover_.destroy(open_gl);
  SynthSection::destroyOpenGlComponents(open_gl);
}

void PopupList::resetScrollPosition() {
  view_position_ = 0;
  setScrollBarRange();
}

void PopupList::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  view_position_ -= wheel.deltaY * kScrollSensitivity;
  view_position_ = std::max(0.0f, view_position_);
  float scaled_height = getHeight();
  int scrollable_range = getScrollableRange();
  view_position_ = std::min(view_position_, 1.0f * scrollable_range - scaled_height);
  setScrollBarRange();
}

void PopupList::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
  view_position_ = range_start;
}

void PopupList::setScrollBarRange() {
  static constexpr float kScrollStepRatio = 0.05f;

  float scaled_height = getHeight();
  scroll_bar_->setRangeLimits(0.0f, getScrollableRange());
  scroll_bar_->setCurrentRange(getViewPosition(), scaled_height, dontSendNotification);
  scroll_bar_->setSingleStepSize(scroll_bar_->getHeight() * kScrollStepRatio);
  scroll_bar_->cancelPendingUpdate();
}

int PopupList::getScrollableRange() {
  int row_height = getRowHeight();
  int selections_height = row_height * static_cast<int>(selections_.size());
  return std::max(selections_height, getHeight());
}

SelectionList::SelectionList() : SynthSection("Selection List"), favorites_option_(false),
    num_view_selections_(0), hovered_(-1), x_area_(false), cache_position_(0), is_additional_(),
    highlight_(Shaders::kColorFragment), hover_(Shaders::kColorFragment),
    remove_additional_x_("remove_additional"), view_position_(0.0f) {
  addAndMakeVisible(browse_area_);
  addChildComponent(remove_additional_x_);

  remove_additional_x_.setShape(Paths::thickX());
  browse_area_.setInterceptsMouseClicks(false, false);
  highlight_.setTargetComponent(&browse_area_);
  hover_.setTargetComponent(&browse_area_);

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);

  highlight_.setAdditive(true);
  hover_.setAdditive(true);

  favorites_ = LoadSave::getFavorites();
}

void SelectionList::resized() {
  int scroll_bar_width = kScrollBarWidth * getSizeRatio();
  int scroll_bar_height = getHeight();
  scroll_bar_->setBounds(getWidth() - scroll_bar_width, 0, scroll_bar_width, scroll_bar_height);
  setScrollBarRange();

  browse_area_.setBounds(2, 0, scroll_bar_->getX() - 3, getHeight());
  int row_height = getRowHeight();
  remove_additional_x_.setBounds(0, 0, row_height, row_height);
  remove_additional_x_.redrawImage(false);

  loadBrowserCache(cache_position_, cache_position_ + kNumCachedRows);

  Colour lighten = findColour(Skin::kLightenScreen, true);
  scroll_bar_->setColor(lighten);
}

void SelectionList::sort() {
  sortFileArray<FileNameAscendingComparator>(selections_);
  filter(filter_string_);
  setScrollBarRange();
}

void SelectionList::setSelections(Array<File> selections) {
  selections_ = std::move(selections);
  sort();
  redoCache();
}

void SelectionList::resetScrollPosition() {
  view_position_ = 0;
  viewPositionChanged();
  setScrollBarRange();
}

void SelectionList::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  view_position_ -= wheel.deltaY * kScrollSensitivity;
  view_position_ = std::max(0.0f, view_position_);
  float scaled_height = getHeight();
  int scrollable_range = getScrollableRange();
  view_position_ = std::min(view_position_, 1.0f * scrollable_range - scaled_height);
  viewPositionChanged();
  setScrollBarRange();
}

int SelectionList::getRowFromPosition(float mouse_position) {  
  return floorf((mouse_position + getViewPosition()) / getRowHeight());
}

void SelectionList::mouseMove(const MouseEvent& e) {
  hovered_ = getRowFromPosition(e.position.y);
  if (hovered_ >= filtered_selections_.size())
    hovered_ = -1;

  int row_height = getRowHeight();
  int x = e.position.x - getWidth() + kScrollBarWidth * size_ratio_ + row_height;
  x_area_ = x >= 0 && x < row_height;
}

void SelectionList::mouseExit(const MouseEvent& e) {
  hovered_ = -1;
}

void SelectionList::respondToMenuCallback(int result) {
  if (result < 0 || result >= filtered_selections_.size())
    return;

  filtered_selections_[result].revealToUser();
}

void SelectionList::menuClick(const MouseEvent& e) {
  float click_y_position = e.position.y;
  int row = getRowFromPosition(click_y_position);

  if (row >= 0 && hovered_ >= 0) {
    PopupItems options;
    options.addItem(hovered_, "Open File Location");
    showPopupSelector(this, e.getPosition(), options, [=](int selection) { respondToMenuCallback(selection); });
  }
}

File SelectionList::getSelection(const MouseEvent& e) {
  float click_y_position = e.position.y;
  int row = getRowFromPosition(click_y_position);
  if (row < filtered_selections_.size() && row >= 0)
    return filtered_selections_[row];

  return File();
}

void SelectionList::leftClick(const MouseEvent& e) {
  float click_x_position = e.position.x;
  int star_right = getRowHeight() + getIconPadding();
  File selection = getSelection(e);
  if (!selection.exists() && selection != getFavoritesFile() && selection != getAllFile()) {
    if (selection.getFileName() == String(kAddFolderName))
      addAdditionalFolder();
    return;
  }

  if (click_x_position < star_right)
    selectIcon(selection);
  else if (x_area_)
    removeAdditionalFolder(selection);
  else
    select(selection);
}

void SelectionList::mouseDown(const MouseEvent& e) {
  if (e.mods.isPopupMenu())
    menuClick(e);
  else
    leftClick(e);
}

void SelectionList::mouseDoubleClick(const MouseEvent& e) {
  float click_x_position = e.position.x;
  int star_right = getRowHeight() + getIconPadding();
  File selection = getSelection(e);
  if (!selection.exists())
    return;

  if (click_x_position < star_right || selection != selected_)
    return;

  for (Listener* listener : listeners_)
    listener->doubleClickedSelected(selection);
}

void SelectionList::addAdditionalFolder() {
  FileChooser open_box("Add Folder", File());
  if (open_box.browseForDirectory()) {
    File result = open_box.getResult();
    if (result.exists()) {
      if (isAcceptableRoot(result)) {
        std::vector<std::string> roots = LoadSave::getAdditionalFolders(additional_roots_name_);
        for (const std::string& root : roots) {
          if (result == File(root)) {
            NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Error Adding Folder",
                                                  String("Folder already added"));
            return;
          }
        }
        if (selections_.contains(File(result))) {
          NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Error Adding Folder",
                                                String("Folder already added"));
          return;
        }
        additional_roots_.add(result);
        roots.push_back(result.getFullPathName().toStdString());
        LoadSave::saveAdditionalFolders(additional_roots_name_, roots);
        sort();
        redoCache();
      }
      else
        showAddRootWarning();
    }
  }
}

void SelectionList::removeAdditionalFolder(const File& folder) {
  additional_roots_.removeFirstMatchingValue(folder);
  std::vector<std::string> roots = LoadSave::getAdditionalFolders(additional_roots_name_);
  std::string path = folder.getFullPathName().toStdString();
  const auto& pos = std::find(roots.begin(), roots.end(), path);
  if (pos != roots.end())
    roots.erase(pos);
  LoadSave::saveAdditionalFolders(additional_roots_name_, roots);

  sort();
  redoCache();
}

void SelectionList::select(const File& selection) {
  if (selection.exists() && selection.isDirectory() && selection == selected_) {
    toggleOpenFolder(selection);
    return;
  }
  selected_ = selection;

  if (selected_ == getFavoritesFile()) {
    for (Listener* listener : listeners_)
      listener->favoritesSelected();
  }
  else if (selected_ == getAllFile()) {
    for (Listener* listener : listeners_)
      listener->allSelected();
  }
  else if (selected_.exists()) {
    for (Listener* listener : listeners_)
      listener->newSelection(selection);
  }
}

void SelectionList::selectIcon(const File& selection) {
  if (selection.isDirectory())
    select(selection);
  else
    toggleFavorite(selection);
}

void SelectionList::toggleFavorite(const File& selection) {
  if (selection == getFavoritesFile() || selection == getAllFile()) {
    select(selection);
    return;
  }

  std::string path = selection.getFullPathName().toStdString();
  if (favorites_.count(path)) {
    favorites_.erase(path);
    LoadSave::removeFavorite(selection);
  }
  else {
    favorites_.insert(path);
    LoadSave::addFavorite(selection);
  }
  redoCache();
}

void SelectionList::toggleOpenFolder(const File& selection) {
  std::string path = selection.getFullPathName().toStdString();
  Array<File> children;
  selection.findChildFiles(children, File::findDirectories, false);
  if (open_folders_.count(path))
    open_folders_.erase(path);
  else if (!children.isEmpty())
    open_folders_[path] = getFolderDepth(selection);
  sort();
  redoCache();
}

void SelectionList::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
  view_position_ = range_start;
  viewPositionChanged();
}

void SelectionList::setScrollBarRange() {
  static constexpr float kScrollStepRatio = 0.05f;

  float scaled_height = getHeight();
  scroll_bar_->setRangeLimits(0.0f, getScrollableRange());
  scroll_bar_->setCurrentRange(getViewPosition(), scaled_height, dontSendNotification);
  scroll_bar_->setSingleStepSize(scroll_bar_->getHeight() * kScrollStepRatio);
  scroll_bar_->cancelPendingUpdate();
}

void SelectionList::redoCache() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int max = static_cast<int>(filtered_selections_.size()) - kNumCachedRows;
  int position = std::max(0, std::min<int>(cache_position_, max));
  loadBrowserCache(position, position + kNumCachedRows);
}

int SelectionList::getFolderDepth(const File& file) {
  std::string parent_string = file.getParentDirectory().getFullPathName().toStdString();
  if (open_folders_.count(parent_string))
    return open_folders_[parent_string] + 1;
  return 0;
}

void SelectionList::addSubfolderSelections(const File& selection, std::vector<File>& selections) {
  Array<File> children;
  selection.findChildFiles(children, File::findDirectories, false);
  for (const File& child : children) {
    selections.push_back(child);
    if (open_folders_.count(child.getFullPathName().toStdString()))
      addSubfolderSelections(child, selections);
  }
}

void SelectionList::setAdditionalRootsName(const std::string& name) {
  additional_roots_name_ = name;
  additional_roots_.clear();
  if (!name.empty()) {
    std::vector<std::string> roots = LoadSave::getAdditionalFolders(additional_roots_name_);
    for (const std::string& root : roots) {
      File file(root);
      if (file.exists())
        additional_roots_.add(file);
    }
  }
}

void SelectionList::filter(const String& filter_string) {
  filter_string_ = filter_string.toLowerCase();
  StringArray tokens;
  tokens.addTokens(filter_string_, " ", "");
  filtered_selections_.clear();
  if (favorites_option_) {
    filtered_selections_.push_back(getAllFile());
    filtered_selections_.push_back(getFavoritesFile());
  }

  Array<File> all_selections = selections_;
  all_selections.addArray(additional_roots_);

  for (const File& selection : all_selections) {
    bool match = true;
    if (tokens.size()) {
      String name = selection.getFileNameWithoutExtension().toLowerCase();

      for (const String& token : tokens) {
        if (!name.contains(token))
          match = false;
      }
    }
    if (match) {
      filtered_selections_.push_back(selection);
      if (open_folders_.count(selection.getFullPathName().toStdString()))
        addSubfolderSelections(selection, filtered_selections_);
    }
  }

  if (!additional_roots_name_.empty())
    filtered_selections_.push_back(File::getCurrentWorkingDirectory().getChildFile("_").getChildFile(kAddFolderName));
  num_view_selections_ = static_cast<int>(filtered_selections_.size());

  auto found = std::find(filtered_selections_.begin(), filtered_selections_.end(), selected_);
  if (found == filtered_selections_.end())
    selected_ = File();
}

int SelectionList::getSelectedIndex() {
  for (int i = 0; i < filtered_selections_.size(); ++i) {
    if (selected_ == filtered_selections_[i])
      return i;
  }
  return -1;
}

int SelectionList::getScrollableRange() {
  int row_height = getRowHeight();
  int presets_height = row_height * static_cast<int>(filtered_selections_.size());
  return std::max(presets_height, getHeight());
}

void SelectionList::selectNext() {
  if (filtered_selections_.empty())
    return;
  
  int index = getSelectedIndex();
  index = (index + 1) % filtered_selections_.size();
  select(filtered_selections_[index]);
}

void SelectionList::selectPrev() {
  if (filtered_selections_.empty())
    return;
  
  int index = std::max(0, getSelectedIndex());
  index = (index - 1 + filtered_selections_.size()) % filtered_selections_.size();
  select(filtered_selections_[index]);
}

void SelectionList::initOpenGlComponents(OpenGlWrapper& open_gl) {
  for (OpenGlImage& row : rows_) {
    row.setScissor(true);
    row.init(open_gl);
    row.setColor(Colours::white);
  }

  highlight_.init(open_gl);
  hover_.init(open_gl);
  remove_additional_x_.init(open_gl);
  SynthSection::initOpenGlComponents(open_gl);
}

void SelectionList::viewPositionChanged() {
  int row_height = getRowHeight();

  int last_cache_position = cache_position_;
  cache_position_ = getViewPosition() / row_height;
  int max = static_cast<int>(filtered_selections_.size() - kNumCachedRows);
  cache_position_ = std::max(0, std::min<int>(cache_position_, max));

  if (std::abs(cache_position_ - last_cache_position) >= kNumCachedRows)
    redoCache();
  else if (last_cache_position < cache_position_)
    loadBrowserCache(last_cache_position + kNumCachedRows, cache_position_ + kNumCachedRows);
  else if (last_cache_position > cache_position_)
    loadBrowserCache(cache_position_, last_cache_position);
}

void SelectionList::loadBrowserCache(int start_index, int end_index) {
  int mult = getPixelMultiple();
  int row_height = getRowHeight() * mult;
  int image_width = getWidth() * mult;

  int padding = getIconPadding();
  int icon_x = padding;
  int icon_width = row_height;
  int name_x = icon_x + icon_width + padding;
  int name_width = image_width - name_x;

  end_index = std::min(static_cast<int>(filtered_selections_.size()), end_index);
  Font font = Fonts::instance()->proportional_light().withPointHeight(row_height * 0.55f);

  Path star = Paths::star();
  Path folder = Paths::folder();
  float star_draw_width = row_height * 0.8f;
  float star_y = (row_height - star_draw_width) / 2.0f;
  Rectangle<float> star_bounds(icon_x + (icon_width - star_draw_width) / 2.0f, star_y,
                               star_draw_width, star_draw_width);
  star.applyTransform(star.getTransformToScaleToFit(star_bounds, true));

  float folder_draw_width = row_height * 0.6f;
  float folder_y = (row_height - folder_draw_width) / 2.0f;
  Rectangle<float> folder_bounds(icon_x + (icon_width - folder_draw_width) / 2.0f, folder_y,
                                 folder_draw_width, folder_draw_width);
  folder.applyTransform(folder.getTransformToScaleToFit(folder_bounds, true));
  PathStrokeType icon_stroke(1.0f, PathStrokeType::curved);

  Colour text_color = findColour(Skin::kTextComponentText, true);
  Colour icon_unselected = text_color.withMultipliedAlpha(0.5f);
  Colour icon_selected = findColour(Skin::kWidgetPrimary1, true);

  for (int i = start_index; i < end_index; ++i) {
    Image row_image(Image::ARGB, image_width, row_height, true);
    Graphics g(row_image);

    File selection = filtered_selections_[i];
    String name = selection.getFileNameWithoutExtension();
    if (selection.isDirectory()) {
      int parents = getFolderDepth(selection);
      g.addTransform(AffineTransform::translation(Point<int>(parents * folder_draw_width, 0)));

      if (name == String(passthrough_name_))
        name = selection.getParentDirectory().getFileNameWithoutExtension();

      g.setColour(icon_unselected);
      if (open_folders_.count(selection.getFullPathName().toStdString()))
        g.fillPath(folder);

      g.strokePath(folder, icon_stroke);
    }
    else if (selection.getFileName() == String(kAddFolderName)) {
      g.setColour(icon_unselected);
      Path add_folder_path;
      float dashes[2] = { 4.0f * size_ratio_, 2.0f * size_ratio_ };
      icon_stroke.createDashedStroke(add_folder_path, folder, dashes, 2);
      g.fillPath(add_folder_path);
    }
    else if (selection.exists() || selection.getFileName() == "Favorites") {
      if (favorites_.count(selection.getFullPathName().toStdString())) {
        g.setColour(icon_selected);
        g.fillPath(star);
      }
      else
        g.setColour(icon_unselected);

      g.strokePath(star, icon_stroke);
    }

    g.setColour(text_color);
    g.setFont(font);
    g.drawText(name, name_x, 0, name_width - 2 * padding, row_height, Justification::centredLeft, true);
    rows_[i % kNumCachedRows].setOwnImage(row_image);
    is_additional_[i % kNumCachedRows] = additional_roots_.contains(selection);
  }
}

void SelectionList::moveQuadToRow(OpenGlQuad& quad, int row, float y_offset) {
  int row_height = getRowHeight();
  float view_height = getHeight();
  float open_gl_row_height = 2.0f * row_height / view_height;
  float offset = row * open_gl_row_height;

  float y = 1.0f + y_offset - offset;
  quad.setQuad(0, -1.0f, y - open_gl_row_height, 2.0f, open_gl_row_height);
}

void SelectionList::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  float view_height = getHeight();
  int row_height = getRowHeight();
  int num_presets = num_view_selections_;

  int view_position = getViewPosition();
  float y_offset = 2.0f * view_position / view_height;

  Rectangle<int> view_bounds(getLocalBounds());
  OpenGlComponent::setViewPort(this, view_bounds, open_gl);

  float image_width = vital::utils::nextPowerOfTwo(getWidth());
  float image_height = vital::utils::nextPowerOfTwo(row_height);
  float width_ratio = image_width / getWidth();
  float height_ratio = image_height / row_height;

  float open_gl_row_height = height_ratio * 2.0f * row_height / view_height;
  int cache_position = std::max(0, std::min(cache_position_, num_presets - kNumCachedRows));
  for (int i = 0; i < kNumCachedRows && i < num_presets; ++i) {
    int row = cache_position + i;
    int cache_index = row % kNumCachedRows;
    float offset = (2.0f * row_height * row) / view_height;
    float y = 1.0f + y_offset - offset;

    Rectangle<int> row_bounds(0, row_height * row - view_position, getWidth(), row_height);
    OpenGlComponent::setScissorBounds(this, row_bounds, open_gl);

    rows_[cache_index].setTopLeft(-1.0f, y);
    rows_[cache_index].setTopRight(-1.0f + 2.0f * width_ratio, y);
    rows_[cache_index].setBottomLeft(-1.0f, y - open_gl_row_height);
    rows_[cache_index].setBottomRight(-1.0f + 2.0f * width_ratio, y - open_gl_row_height);
    rows_[cache_index].drawImage(open_gl);
  }

  int selected_index = getSelectedIndex();
  if (selected_index >= 0) {
    moveQuadToRow(highlight_, selected_index, y_offset);
    highlight_.setColor(findColour(Skin::kWidgetPrimary1, true).darker(0.8f));
    highlight_.render(open_gl, animate);
  }

  if (hovered_ >= 0) {
    moveQuadToRow(hover_, hovered_, y_offset);
    hover_.setColor(findColour(Skin::kLightenScreen, true));
    hover_.render(open_gl, animate);

    int cache_index = hovered_ % kNumCachedRows;

    int scroll_bar_width = kScrollBarWidth * size_ratio_;
    Rectangle<int> bounds(getWidth() - row_height - scroll_bar_width, row_height * hovered_ - view_position_,
                          row_height, row_height);
    if (OpenGlComponent::setViewPort(&browse_area_, bounds, open_gl) && is_additional_[cache_index]) {
      if (x_area_)
        remove_additional_x_.setColor(findColour(Skin::kIconButtonOffHover, true));
      else
        remove_additional_x_.setColor(findColour(Skin::kIconButtonOff, true));
      remove_additional_x_.image().drawImage(open_gl);
    }
  }

  SynthSection::renderOpenGlComponents(open_gl, animate);
}

void SelectionList::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  for (OpenGlImage& row : rows_)
    row.destroy(open_gl);

  highlight_.destroy(open_gl);
  hover_.destroy(open_gl);
  remove_additional_x_.destroy(open_gl);
  SynthSection::destroyOpenGlComponents(open_gl);
}

SinglePopupSelector::SinglePopupSelector() : SynthSection("Popup Selector"),
                                             body_(Shaders::kRoundedRectangleFragment),
                                             border_(Shaders::kRoundedRectangleBorderFragment) {
  callback_ = nullptr;
  cancel_ = nullptr;
  addOpenGlComponent(&body_);
  addOpenGlComponent(&border_);

  popup_list_ = std::make_unique<PopupList>();
  popup_list_->addListener(this);
  addSubSection(popup_list_.get());
  popup_list_->setAlwaysOnTop(true);
  popup_list_->setWantsKeyboardFocus(false);

  setSkinOverride(Skin::kPopupBrowser);
}

void SinglePopupSelector::resized() {
  SynthSection::resized();

  Rectangle<int> bounds = getLocalBounds();
  int rounding = findValue(Skin::kBodyRounding);
  popup_list_->setBounds(1, rounding, getWidth() - 2, getHeight() - 2 * rounding);

  body_.setBounds(bounds);
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));

  border_.setBounds(bounds);
  border_.setRounding(findValue(Skin::kBodyRounding));
  border_.setThickness(1.0f, true);
  border_.setColor(findColour(Skin::kBorder, true));
}

void SinglePopupSelector::setPosition(Point<int> position, Rectangle<int> bounds) {
  int rounding = findValue(Skin::kBodyRounding);
  int width = popup_list_->getBrowseWidth();
  int height = popup_list_->getBrowseHeight() + 2 * rounding;
  int x = position.x;
  int y = position.y;
  if (x + width > bounds.getRight())
    x -= width;
  if (y + height > bounds.getBottom())
    y = bounds.getBottom() - height;
  setBounds(x, y, width, height);
}

DualPopupSelector::DualPopupSelector() : SynthSection("Dual Popup Selector"),
                                         body_(Shaders::kRoundedRectangleFragment),
                                         border_(Shaders::kRoundedRectangleBorderFragment),
                                         divider_(Shaders::kColorFragment) {
  callback_ = nullptr;

  addOpenGlComponent(&body_);
  addOpenGlComponent(&border_);
  addOpenGlComponent(&divider_);

  left_list_ = std::make_unique<PopupList>();
  left_list_->addListener(this);
  addSubSection(left_list_.get());
  left_list_->setAlwaysOnTop(true);
  left_list_->setWantsKeyboardFocus(false);
  left_list_->showSelected(true);

  right_list_ = std::make_unique<PopupList>();
  right_list_->addListener(this);
  addSubSection(right_list_.get());
  right_list_->setAlwaysOnTop(true);
  right_list_->setWantsKeyboardFocus(false);
  right_list_->showSelected(true);

  setSkinOverride(Skin::kPopupBrowser);
}

void DualPopupSelector::resized() {
  SynthSection::resized();

  Rectangle<int> bounds = getLocalBounds();
  int rounding = findValue(Skin::kBodyRounding);
  int height = getHeight() - 2 * rounding;
  left_list_->setBounds(1, rounding, getWidth() / 2 - 2, height);
  int right_x = left_list_->getRight() + 1;
  right_list_->setBounds(right_x, rounding, getWidth() - right_x - 1, height);

  body_.setBounds(bounds);
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));

  border_.setBounds(bounds);
  border_.setRounding(findValue(Skin::kBodyRounding));
  border_.setThickness(1.0f, true);

  divider_.setBounds(getWidth() / 2 - 1, 1, 1, getHeight() - 2);

  Colour border = findColour(Skin::kBorder, true);
  border_.setColor(border);
  divider_.setColor(border);
}

void DualPopupSelector::setPosition(Point<int> position, int width, Rectangle<int> bounds) {
  int rounding = findValue(Skin::kBodyRounding);
  int height = left_list_->getBrowseHeight() + 2 * rounding;
  int x = position.x;
  int y = position.y;
  if (x + width > bounds.getRight())
    x -= width;
  if (y + height > bounds.getBottom())
    y = bounds.getBottom() - height;
  setBounds(x, y, width, height);
}

void DualPopupSelector::newSelection(PopupList* list, int id, int index) {
  if (list == left_list_.get()) {
    PopupItems right_items = left_list_->getSelectionItems(index);
    if (right_items.size() == 0) {
      callback_(id);
      right_list_->setSelections(right_items);
      return;
    }

    int right_selection = right_list_->getSelected();
    if (right_selection < 0 || right_selection >= right_items.size() ||
      right_list_->getSelectionItems(right_selection).name != right_items.items[right_selection].name) {
      right_selection = 0;
    }

    right_list_->setSelections(right_items);
    right_list_->select(right_selection);
  }
  else
    callback_(id);
}

PopupBrowser::PopupBrowser() : SynthSection("Popup Browser"),
                               body_(Shaders::kRoundedRectangleFragment),
                               border_(Shaders::kRoundedRectangleBorderFragment),
                               horizontal_divider_(Shaders::kColorFragment),
                               vertical_divider_(Shaders::kColorFragment),
                               owner_(nullptr) {
  addKeyListener(this);
  setInterceptsMouseClicks(false, true);

  addOpenGlComponent(&body_);
  addOpenGlComponent(&border_);
  addOpenGlComponent(&horizontal_divider_);
  addOpenGlComponent(&vertical_divider_);

  folder_list_ = std::make_unique<SelectionList>();
  folder_list_->addFavoritesOption();
  folder_list_->addListener(this);
  addSubSection(folder_list_.get());
  folder_list_->setAlwaysOnTop(true);

  selection_list_ = std::make_unique<SelectionList>();
  selection_list_->addListener(this);
  addSubSection(selection_list_.get());
  selection_list_->setAlwaysOnTop(true);
                                 
  for (int i = 0; i < 4; ++i) {
    addAndMakeVisible(closing_areas_[i]);
    closing_areas_[i].addListener(this);
  }

  exit_button_ = std::make_unique<OpenGlShapeButton>("Exit");
  addAndMakeVisible(exit_button_.get());
  addOpenGlComponent(exit_button_->getGlComponent());
  exit_button_->addListener(this);
  exit_button_->setShape(Paths::exitX());

  store_button_ = std::make_unique<OpenGlToggleButton>("Store");
  store_button_->setUiButton(true);
  addButton(store_button_.get());
  store_button_->setVisible(false);

  download_button_ = std::make_unique<OpenGlToggleButton>("Login");
  download_button_->setUiButton(true);
  download_button_->setText("Download content");
  addButton(download_button_.get());
  download_button_->setVisible(false);

#if !defined(NO_TEXT_ENTRY)
  search_box_ = std::make_unique<OpenGlTextEditor>("Search");
  search_box_->addListener(this);
  search_box_->setSelectAllWhenFocused(true);
  search_box_->setMultiLine(false, false);
  search_box_->setJustification(Justification::centredLeft);
  addAndMakeVisible(search_box_.get());
  addOpenGlComponent(search_box_->getImageComponent());
#endif

  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  setSkinOverride(Skin::kPopupBrowser);
}

PopupBrowser::~PopupBrowser() = default;

void PopupBrowser::resized() {
  static constexpr float kBrowseWidthRatio = 0.5f;
  static constexpr float kTopHeight = 38.0f;

  SynthSection::resized();
  
  closing_areas_[0].setBounds(0, 0, passthrough_bounds_.getX(), getHeight());
  closing_areas_[1].setBounds(passthrough_bounds_.getRight(), 0,
                              getWidth() - passthrough_bounds_.getRight(), getHeight());
  closing_areas_[2].setBounds(0, 0, getWidth(), passthrough_bounds_.getY());
  closing_areas_[3].setBounds(0, passthrough_bounds_.getBottom(),
                              getWidth(), getHeight() - passthrough_bounds_.getBottom());

  body_.setBounds(browser_bounds_);
  body_.setRounding(findValue(Skin::kBodyRounding));
  body_.setColor(findColour(Skin::kBody, true));

  border_.setBounds(browser_bounds_);
  border_.setRounding(findValue(Skin::kBodyRounding));
  border_.setThickness(1.0f, true);

  Colour border = findColour(Skin::kBorder, true);
  border_.setColor(border);
  horizontal_divider_.setColor(border);
  vertical_divider_.setColor(border);

  Colour empty_color = findColour(Skin::kBodyText, true);
  empty_color = empty_color.withAlpha(0.5f * empty_color.getFloatAlpha());

  if (search_box_) {
    search_box_->setTextToShowWhenEmpty(TRANS("Search"), empty_color);
    search_box_->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
    search_box_->setColour(TextEditor::textColourId, findColour(Skin::kBodyText, true));
    search_box_->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
    search_box_->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
  }

  int selection_list_width = browser_bounds_.getWidth() * kBrowseWidthRatio;
  int top_height = kTopHeight * size_ratio_;
  int folder_list_width = browser_bounds_.getWidth() - selection_list_width;
  int list_height = browser_bounds_.getHeight() - top_height - 2;
  int x = browser_bounds_.getX();
  int y = browser_bounds_.getY();

  folder_list_->setBounds(x, y + top_height + 1, folder_list_width - 1, list_height);
  selection_list_->setBounds(x + folder_list_width, y + top_height + 1, selection_list_width - 3, list_height);
  horizontal_divider_.setBounds(x + 1, y + top_height - 1, browser_bounds_.getWidth() - 2, 1);
  vertical_divider_.setBounds(x + folder_list_width, y + top_height, 1, list_height);

  int padding = getPadding();
  int text_height = top_height - 2 * padding;
  download_button_->setBounds(x + padding, y + padding, selection_list_width - 2 * padding, text_height);
  if (search_box_) {
    search_box_->setBounds(download_button_->getBounds());
    search_box_->resized();
  }

  int store_x = x + padding + selection_list_width;
  store_button_->setBounds(store_x, y + padding, browser_bounds_.getRight() - store_x - top_height, text_height);
  exit_button_->setBounds(x + browser_bounds_.getWidth() - top_height, y, top_height, top_height);

  Image image(Image::ARGB, 1, 1, false);
  Graphics g(image);
  paintOpenGlChildrenBackgrounds(g);
}

void PopupBrowser::visibilityChanged() {
  checkNoContent();
  checkStoreButton();
  SynthSection::visibilityChanged();
  if (search_box_)
    search_box_->setText("");

  File selected = folder_list_->selected();
  if (selected.exists())
    newSelection(selected);
}

void PopupBrowser::newSelection(File selection) {
  if (selection.exists() && selection.isDirectory()) {
    Array<File> files;
    selection.findChildFiles(files, File::findFiles, true, extensions_);
    selection_list_->setSelections(files);
    selection_list_->resetScrollPosition();
  }
  else {
    if (owner_) {
      owner_->loadFile(selection);
      checkStoreButton();
    }
  }
}

void PopupBrowser::allSelected() {
  Array<File> files;
  Array<File> folders = folder_list_->getSelections();
  folders.addArray(folder_list_->getAdditionalFolders());
  for (const File& folder : folders) {
    if (folder.exists() && folder.isDirectory())
      folder.findChildFiles(files, File::findFiles, true, extensions_);
  }

  selection_list_->setSelections(files);
  selection_list_->resetScrollPosition();
}

void PopupBrowser::favoritesSelected() {
  Array<File> files;
  Array<File> folders = folder_list_->getSelections();
  folders.addArray(folder_list_->getAdditionalFolders());
  for (const File& folder : folders) {
    if (folder.exists() && folder.isDirectory())
      folder.findChildFiles(files, File::findFiles, true, extensions_);
  }
  Array<File> favorites;
  std::set<std::string> favorite_lookup = LoadSave::getFavorites();
  for (const File& file : files) {
    if (favorite_lookup.count(file.getFullPathName().toStdString()))
      favorites.add(file);
  }
  selection_list_->setSelections(favorites);
  selection_list_->resetScrollPosition();
}

void PopupBrowser::doubleClickedSelected(File selection) {
  if (selection.exists() && !selection.isDirectory())
    setVisible(false);
}

bool PopupBrowser::keyPressed(const KeyPress &key, Component *origin) {
  if (!isVisible())
    return search_box_->hasKeyboardFocus(true);

  if (key.getKeyCode() == KeyPress::escapeKey) {
    setVisible(false);
    return true;
  }
  if (key.getKeyCode() == KeyPress::upKey || key.getKeyCode() == KeyPress::leftKey) {
    selection_list_->selectPrev();
    return true;
  }
  if (key.getKeyCode() == KeyPress::downKey || key.getKeyCode() == KeyPress::rightKey) {
    selection_list_->selectNext();
    return true;
  }
  return search_box_->hasKeyboardFocus(true);
}

bool PopupBrowser::keyStateChanged(bool is_key_down, Component *origin) {
  if (is_key_down)
    return search_box_->hasKeyboardFocus(true);
  return false;
}

void PopupBrowser::closingAreaClicked(PopupClosingArea* closing_area, const MouseEvent& e) {
  if (!browser_bounds_.contains(e.getPosition() + closing_area->getPosition()))
    setVisible(false);
}

void PopupBrowser::checkNoContent() {
  bool has_content = LoadSave::hasDataDirectory();
  if (search_box_)
    search_box_->setVisible(has_content);
  download_button_->setVisible(!has_content);
}

void PopupBrowser::checkStoreButton() {
  if (owner_) {
    std::string author = owner_->getFileAuthor();
    String type = folder_list_->getPassthroughFolderName();
    store_button_->setText("Get more " + type.toLowerCase().toStdString() + " by " + author);
    String cleaned = String(author).removeCharacters(" _.").toLowerCase();
    store_button_->setVisible(more_author_presets_.count(cleaned.toStdString()));
  }
}

void PopupBrowser::loadPresets(std::vector<File> directories, const String& extensions,
                               const std::string& passthrough_name, const std::string& additional_folders_name) {
  extensions_ = extensions;
  if (search_box_)
    search_box_->setText("");

  Array<File> folders;
  for (const File& directory : directories)
    folders.add(directory);

  folder_list_->setPassthroughFolderName(passthrough_name);
  folder_list_->setAdditionalRootsName(additional_folders_name);
  folder_list_->setSelections(folders);

  if (!additional_folders_name.empty()) {
    std::vector<std::string> additional = LoadSave::getAdditionalFolders(additional_folders_name);
    for (const std::string& path : additional)
      directories.emplace_back(File(path));
  }

  Array<File> presets;
  selection_list_->setSelected(File());
  folder_list_->filter("");
  if (!folder_list_->selected().exists()) {
    LoadSave::getAllFilesOfTypeInDirectories(presets, extensions_, directories);
    selection_list_->setSelections(presets);
  }
  selection_list_->filter("");
  if (owner_)
    selection_list_->setSelected(owner_->getCurrentFile());

  more_author_presets_.clear();
  try {
    json available = LoadSave::getAvailablePacks();
    json available_packs = available["packs"];
    for (auto& pack : available_packs) {
      if (pack.count(folder_list_->getPassthroughFolderName()) == 0)
        continue;

      bool contains_files = pack[folder_list_->getPassthroughFolderName()];
      if (!contains_files)
        continue;

      bool purchased = false;
      if (pack.count("Purchased"))
        purchased = pack["Purchased"];
      if (purchased)
        continue;

      std::string author_data = pack["Author"];
      StringArray authors;
      authors.addTokens(author_data, ",", "");
      for (const String& author : authors)
        more_author_presets_.insert(author.removeCharacters(" ._").toLowerCase().toStdString());
    }
  }
  catch (const json::exception& e) {
  }
  checkNoContent();
  checkStoreButton();
}

void PopupBrowser::filterPresets() {
  selection_list_->filter(search_box_->getText());
  selection_list_->redoCache();
}

void PopupBrowser::textEditorTextChanged(TextEditor& editor) {
  filterPresets();
}

void PopupBrowser::textEditorEscapeKeyPressed(TextEditor& editor) {
  editor.setText("");
}

void PopupBrowser::buttonClicked(Button* clicked_button) {
  if (clicked_button == exit_button_.get())
    setVisible(false);
  else if (clicked_button == download_button_.get()) {
    FullInterface* parent = findParentComponentOfClass<FullInterface>();
    if (parent) {
      setVisible(false);
      parent->startDownload();
    }
  }
  else if (clicked_button == store_button_.get() && owner_) {
    String encoded_author = URL::addEscapeChars(owner_->getFileAuthor(), true);
    encoded_author = encoded_author.replace("+", "%2B");
    
    URL url(String(kStoreUrl) + encoded_author);
    url.launchInDefaultBrowser();
  }
}
