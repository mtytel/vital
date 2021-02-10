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

#include "bank_exporter.h"

#include "skin.h"
#include "fonts.h"
#include "load_save.h"
#include "paths.h"
#include "open_gl_component.h"
#include "open_gl_image.h"
#include "synth_section.h"

namespace {
  template<class Comparator>
  void sortFiles(Array<File>& file_array) {
    Comparator comparator;
    file_array.sort(comparator, true);
  }

  String getRelativePath(const File& file, const String& folder) {
    File parent = file;
    while (parent.exists() && !parent.isRoot()) {
      parent = parent.getParentDirectory();
      if (parent.getFileName() == folder)
        return file.getRelativePathFrom(parent).replaceCharacter(File::getSeparatorChar(), '/');
    }

    return file.getFileName();
  }
}

ContentList::ContentList(const std::string& name) :
    SynthSection(name), num_contents_(0), last_selected_index_(-1), hover_index_(-1),
    cache_position_(0), view_position_(0), sort_column_(kDate), sort_ascending_(true), selected_(),
    highlight_(kNumCachedRows, Shaders::kColorFragment), hover_(Shaders::kColorFragment) {
  addAndMakeVisible(browse_area_);
  browse_area_.setInterceptsMouseClicks(false, false);
  highlight_.setTargetComponent(&browse_area_);
  hover_.setTargetComponent(&browse_area_);
  highlight_.setAdditive(true);
  hover_.setAdditive(true);

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);
}

void ContentList::paintBackground(Graphics& g) {
  int title_width = getTitleWidth();
  g.setColour(findColour(Skin::kWidgetBackground, true));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));

  int selected_width = kAddWidthRatio * getWidth();
  int name_width = kNameWidthRatio * getWidth();
  int date_width = getWidth() - name_width;
  int row_height = getRowHeight();
  int text_padding = row_height / 2;

  g.saveState();
  g.setColour(findColour(Skin::kBody, true));
  g.reduceClipRegion(getLocalBounds().removeFromTop(title_width));
  Rectangle<float> top = getLocalBounds().toFloat().removeFromTop(title_width * 2.0f);
  g.fillRoundedRectangle(top, findValue(Skin::kBodyRounding));
  g.restoreState();

  Colour lighten = findColour(Skin::kLightenScreen, true);
  scroll_bar_->setColor(lighten);

  g.setColour(lighten);
  g.fillRect(selected_width, 0, 1, title_width);
  g.fillRect(selected_width + name_width, 0, 1, title_width);

  g.setColour(findColour(Skin::kTextComponentText, true));
  g.setFont(Fonts::instance()->proportional_regular().withPointHeight(title_width * 0.5f));

  String name = getName() + " Name";
  g.drawText(name, selected_width + text_padding, 0, name_width, title_width, Justification::centredLeft);
  g.drawText("Date", getWidth() - date_width, 0,
             date_width - text_padding, title_width, Justification::centredRight);

  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
}

void ContentList::resized() {
  static constexpr float kScrollBarWidth = 15.0f;

  int scroll_bar_width = kScrollBarWidth * getSizeRatio();
  int title_width = getTitleWidth();
  int scroll_bar_height = getHeight() - title_width;
  scroll_bar_->setBounds(getWidth() - scroll_bar_width, title_width, scroll_bar_width, scroll_bar_height);
  setScrollBarRange();

  browse_area_.setBounds(0, title_width, getWidth(), getHeight() - title_width);
  loadBrowserCache(cache_position_, cache_position_ + kNumCachedRows);
}

void ContentList::sort() {
  if (sort_column_ == kName && sort_ascending_)
    sortFiles<FileNameAscendingComparator>(contents_);
  else if (sort_column_ == kName && !sort_ascending_)
    sortFiles<FileNameDescendingComparator>(contents_);
  else if (sort_column_ == kDate && sort_ascending_)
    sortFiles<FileDateAscendingComparator>(contents_);
  else if (sort_column_ == kDate && !sort_ascending_)
    sortFiles<FileDateDescendingComparator>(contents_);
  else if (sort_column_ == kAdded) {
    SelectedComparator comparator(selected_files_, sort_ascending_);
    contents_.sort(comparator, true);
  }
}

void ContentList::setContent(Array<File> contents) {
  contents_ = std::move(contents);
  num_contents_ = contents_.size();
  redoCache();
  setScrollBarRange();
}

void ContentList::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) {
  view_position_ -= wheel.deltaY * kScrollSensitivity;
  view_position_ = std::max(0.0f, view_position_);
  int title_width = getTitleWidth();
  float scaled_height = getHeight() - title_width;
  int scrollable_range = getScrollableRange();
  view_position_ = std::min(view_position_, 1.0f * scrollable_range - scaled_height);
  viewPositionChanged();
  setScrollBarRange();
}

int ContentList::getRowFromPosition(float mouse_position) {
  int title_width = getTitleWidth();

  return floorf((mouse_position + getViewPosition() - title_width) / getRowHeight());
}

void ContentList::mouseMove(const MouseEvent& e) {
  hover_index_ = getRowFromPosition(e.position.y);
  if (hover_index_ >= contents_.size())
    hover_index_ = -1;
}

void ContentList::mouseExit(const MouseEvent& e) {
  hover_index_ = -1;
}

void ContentList::mouseDown(const MouseEvent& e) {
  int title_width = getTitleWidth();
  float click_y_position = e.position.y;
  float click_x_position = e.position.x;
  int row = getRowFromPosition(click_y_position);

  if (click_y_position <= title_width) {
    int selected_right = kAddWidthRatio * getWidth();
    int name_right = selected_right + kNameWidthRatio * getWidth();
    Column clicked_column;

    if (click_x_position < selected_right)
      clicked_column = kAdded;
    else if (click_x_position < name_right)
      clicked_column = kName;
    else
      clicked_column = kDate;

    if (clicked_column == sort_column_)
      sort_ascending_ = !sort_ascending_;
    else
      sort_ascending_ = true;
    sort_column_ = clicked_column;
    sort();
    repaint();
    redoCache();
  }
  else if (row < contents_.size() && row >= 0) {
    if (click_x_position < kAddWidthRatio * getWidth()) {
      if (highlighted_files_.count(contents_[row].getFullPathName().toStdString()) == 0)
        highlightClick(e, row);
      selectHighlighted(row);
    }
    else
      highlightClick(e, row);

    redoCache();
    repaint();
  }
}

void ContentList::scrollBarMoved(ScrollBar* scroll_bar, double range_start) {
  view_position_ = range_start;
  viewPositionChanged();
}

void ContentList::setScrollBarRange() {
  static constexpr float kScrollStepRatio = 0.05f;

  int title_width = getTitleWidth();
  float scaled_height = getHeight() - title_width;
  scroll_bar_->setRangeLimits(0.0f, getScrollableRange());
  scroll_bar_->setCurrentRange(view_position_, scaled_height, dontSendNotification);
  scroll_bar_->setSingleStepSize(scroll_bar_->getHeight() * kScrollStepRatio);
  scroll_bar_->cancelPendingUpdate();
}

void ContentList::selectHighlighted(int clicked_index) {
  std::string path = contents_[clicked_index].getFullPathName().toStdString();
  if (selected_files_.count(path)) {
    for (const std::string& highlighted : highlighted_files_)
      selected_files_.erase(highlighted);
  }
  else {
    for (const std::string& highlighted : highlighted_files_)
      selected_files_.insert(highlighted);
  }
}

void ContentList::highlightClick(const MouseEvent& e, int clicked_index) {
  int cache_index = clicked_index - cache_position_;
  if (e.mods.isShiftDown())
    selectRange(clicked_index);
  else if (e.mods.isCommandDown()) {
    std::string path = contents_[clicked_index].getFullPathName().toStdString();
    bool selected = highlighted_files_.count(path);
    if (selected)
      highlighted_files_.erase(path);
    else
      highlighted_files_.insert(path);
    if (cache_index >= 0 && cache_index < kNumCachedRows)
      selected_[cache_index] = !selected;
  }
  else {
    highlighted_files_.clear();
    for (int i = 0; i < kNumCachedRows; ++i)
      selected_[i] = i == cache_index;
    highlighted_files_.insert(contents_[clicked_index].getFullPathName().toStdString());
  }

  last_selected_index_ = clicked_index;
}

void ContentList::selectRange(int clicked_index) {
  if (last_selected_index_ < 0)
    last_selected_index_ = clicked_index;

  int start = std::min(std::min(clicked_index, last_selected_index_), contents_.size());
  int end = std::min(std::max(clicked_index, last_selected_index_), contents_.size());

  for (int i = start; i <= end; ++i) {
    int cache_index = i - cache_position_;
    if (cache_index >= 0 && cache_index < kNumCachedRows)
      selected_[cache_index] = true;
    highlighted_files_.insert(contents_[i].getFullPathName().toStdString());
  }
}

void ContentList::redoCache() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int max = static_cast<int>(contents_.size()) - kNumCachedRows;
  int position = std::max(0, std::min<int>(cache_position_, max));
  loadBrowserCache(position, position + kNumCachedRows);
}

int ContentList::getScrollableRange() {
  int row_height = getRowHeight();
  int title_width = getTitleWidth();
  int presets_height = row_height * static_cast<int>(contents_.size());
  return std::max(presets_height, getHeight() - title_width);
}

void ContentList::initOpenGlComponents(OpenGlWrapper& open_gl) {
  for (int i = 0; i < kNumCachedRows; ++i) {
    rows_[i].setScissor(true);
    rows_[i].init(open_gl);
    rows_[i].setColor(Colours::white);
  }

  highlight_.init(open_gl);
  hover_.init(open_gl);
  SynthSection::initOpenGlComponents(open_gl);
}

void ContentList::viewPositionChanged() {
  int row_height = getRowHeight();

  int last_cache_position = cache_position_;
  cache_position_ = getViewPosition() / row_height;
  int max = static_cast<int>(contents_.size() - kNumCachedRows);
  cache_position_ = std::max(0, std::min<int>(cache_position_, max));

  if (std::abs(cache_position_ - last_cache_position) >= kNumCachedRows)
    redoCache();
  else if (last_cache_position < cache_position_)
    loadBrowserCache(last_cache_position + kNumCachedRows, cache_position_ + kNumCachedRows);
  else if (last_cache_position > cache_position_)
    loadBrowserCache(cache_position_, last_cache_position);
}

void ContentList::loadBrowserCache(int start_index, int end_index) {
  int mult = getPixelMultiple();
  int row_height = getRowHeight() * mult;
  int image_width = getWidth() * mult;

  int text_padding = row_height / 2.0f;
  int add_x = text_padding;
  int add_width = kAddWidthRatio * image_width;
  int name_x = add_x + add_width;
  int name_width = kNameWidthRatio * image_width;
  int date_width = kDateWidthRatio * image_width;
  int date_x = image_width - date_width + text_padding;

  end_index = std::min(static_cast<int>(contents_.size()), end_index);
  Font font = Fonts::instance()->proportional_light().withPointHeight(row_height * 0.5f);

  Path icon;
  icon.addRoundedRectangle(0.0f, 0.0f, 1.0f, 1.0f, 0.1f, 0.1f);
  icon.addPath(Paths::plusOutline());
  float add_draw_width = row_height * 0.8f;
  float add_y = (row_height - add_draw_width) / 2.0f;
  Rectangle<float> add_bounds((add_width - add_draw_width) / 2.0f, add_y, add_draw_width, add_draw_width);
  icon.applyTransform(icon.getTransformToScaleToFit(add_bounds, true));
  PathStrokeType add_stroke(1.0f, PathStrokeType::curved);

  Colour text_color = findColour(Skin::kTextComponentText, true);
  Colour add_unselected = findColour(Skin::kLightenScreen, true);
  Colour add_selected = findColour(Skin::kWidgetPrimary1, true);

  for (int i = start_index; i < end_index; ++i) {
    Image row_image(Image::ARGB, image_width, row_height, true);
    Graphics g(row_image);

    File content = contents_[i];
    String name = content.getFileNameWithoutExtension();
    String date = content.getCreationTime().toString(true, false, false);

    if (selected_files_.count(content.getFullPathName().toStdString()))
      g.setColour(add_selected);
    else
      g.setColour(add_unselected);

    g.fillPath(icon);

    g.setColour(text_color);
    g.setFont(font);
    g.drawText(name, name_x, 0, name_width - 2 * text_padding, row_height, Justification::centredLeft, true);
    g.drawText(date, date_x, 0, date_width - 2 * text_padding, row_height, Justification::centredRight, true);

    rows_[i % kNumCachedRows].setOwnImage(row_image);
    selected_[i % kNumCachedRows] = highlighted_files_.count(content.getFullPathName().toStdString());
  }
}

void ContentList::moveQuadToRow(OpenGlMultiQuad* quad, int index, int row, float y_offset) {
  int row_height = getRowHeight();
  float view_height = getHeight() - (int)getTitleWidth();
  float open_gl_row_height = 2.0f * row_height / view_height;
  float offset = row * open_gl_row_height;

  float y = 1.0f + y_offset - offset;
  quad->setQuad(index, -1.0f, y - open_gl_row_height, 2.0f, open_gl_row_height);
}

void ContentList::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  int title_width = getTitleWidth();
  float view_height = getHeight() - title_width;
  int row_height = getRowHeight();
  int num_contents = num_contents_;

  int view_position = getViewPosition();
  float y_offset = 2.0f * view_position / view_height;

  Rectangle<int> view_bounds(0, title_width, getWidth(), getHeight() - title_width);
  OpenGlComponent::setViewPort(this, view_bounds, open_gl);

  float image_width = vital::utils::nextPowerOfTwo(getWidth());
  float image_height = vital::utils::nextPowerOfTwo(row_height);
  float width_ratio = image_width / getWidth();
  float height_ratio = image_height / row_height;

  float open_gl_row_height = 2.0f * row_height / view_height;
  float open_gl_image_height = height_ratio * open_gl_row_height;
  int cache_position = std::max(0, std::min(cache_position_, num_contents - kNumCachedRows));
  int num_selected = 0;
  for (int i = 0; i < kNumCachedRows && i < num_contents; ++i) {
    int row = cache_position + i;
    int cache_index = row % kNumCachedRows;
    float offset = (2.0f * row_height * row) / view_height;
    float y = 1.0f + y_offset - offset;

    Rectangle<int> row_bounds(0, row_height * row - view_position + title_width, getWidth(), row_height);
    OpenGlComponent::setScissorBounds(this, row_bounds, open_gl);

    rows_[cache_index].setTopLeft(-1.0f, y);
    rows_[cache_index].setTopRight(-1.0f + 2.0f * width_ratio, y);
    rows_[cache_index].setBottomLeft(-1.0f, y - open_gl_image_height);
    rows_[cache_index].setBottomRight(-1.0f + 2.0f * width_ratio, y - open_gl_image_height);
    rows_[cache_index].drawImage(open_gl);

    if (selected_[cache_index]) {
      highlight_.setQuad(num_selected, -1.0f, y - open_gl_row_height, 2.0f, open_gl_row_height);
      num_selected++;
    }
  }

  highlight_.setNumQuads(num_selected);
  highlight_.setColor(findColour(Skin::kWidgetSecondary1, true).darker(0.8f));
  highlight_.render(open_gl, animate);

  if (hover_index_ >= 0) {
    moveQuadToRow(&hover_, 0, hover_index_, y_offset);
    hover_.setColor(findColour(Skin::kLightenScreen, true));
    hover_.render(open_gl, animate);
  }

  SynthSection::renderOpenGlComponents(open_gl, animate);
}

void ContentList::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  for (int i = 0; i < kNumCachedRows; ++i)
    rows_[i].destroy(open_gl);

  highlight_.destroy(open_gl);
  hover_.destroy(open_gl);
  SynthSection::destroyOpenGlComponents(open_gl);
}

BankExporter::BankExporter() : SynthSection("bank_exporter") {
  export_bank_button_ = std::make_unique<OpenGlToggleButton>("Export Bank");
  export_bank_button_->setEnabled(false);
  export_bank_button_->addListener(this);
  export_bank_button_->setUiButton(true);
  addAndMakeVisible(export_bank_button_.get());
  addOpenGlComponent(export_bank_button_->getGlComponent());

  addKeyListener(this);

  preset_list_ = std::make_unique<ContentList>("Preset");
  addSubSection(preset_list_.get());
  wavetable_list_ = std::make_unique<ContentList>("Wavetable");
  addSubSection(wavetable_list_.get());
  lfo_list_ = std::make_unique<ContentList>("LFO");
  addSubSection(lfo_list_.get());
  sample_list_ = std::make_unique<ContentList>("Sample");
  addSubSection(sample_list_.get());

#if !defined(NO_TEXT_ENTRY)
  bank_name_box_ = std::make_unique<OpenGlTextEditor>("Bank Name");
  bank_name_box_->addListener(this);
  bank_name_box_->setSelectAllWhenFocused(true);
  bank_name_box_->setMultiLine(false, false);
  bank_name_box_->setJustification(Justification::centredLeft);
  addAndMakeVisible(bank_name_box_.get());
  addOpenGlComponent(bank_name_box_->getImageComponent());
#endif

  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  setSkinOverride(Skin::kPresetBrowser);
}

BankExporter::~BankExporter() = default;

void BankExporter::paintBackground(Graphics& g) {
  paintChildrenBackgrounds(g);
  Rectangle<int> export_bounds = wavetable_list_->getBounds();
  export_bounds.setY(0);
  export_bounds.setHeight(wavetable_list_->getY() - findValue(Skin::kLargePadding));
  paintBody(g, export_bounds);

  Colour empty_color = findColour(Skin::kBodyText, true);
  empty_color = empty_color.withAlpha(0.5f * empty_color.getFloatAlpha());
  setButtonColors();

  if (bank_name_box_) {
    bank_name_box_->setTextToShowWhenEmpty(TRANS("Bank Name"), empty_color);
    bank_name_box_->setColour(CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
    bank_name_box_->setColour(TextEditor::textColourId, findColour(Skin::kBodyText, true));
    bank_name_box_->setColour(TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
    bank_name_box_->setColour(TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
    bank_name_box_->redoImage();
  }
}

void BankExporter::paintBackgroundShadow(Graphics& g) {
  paintTabShadow(g, preset_list_->getBounds());
  paintTabShadow(g, wavetable_list_->getBounds());
  paintTabShadow(g, lfo_list_->getBounds());
  paintTabShadow(g, sample_list_->getBounds());

  Rectangle<int> export_bounds = wavetable_list_->getBounds();
  export_bounds.setY(0);
  export_bounds.setHeight(wavetable_list_->getY() - findValue(Skin::kLargePadding));
  paintTabShadow(g, export_bounds);
}

void BankExporter::resized() {
  static constexpr float kOptionsHeight = 0.08f;

  int padding_width = findValue(Skin::kLargePadding);
  int browse_width = getWidth() / 2 - padding_width;
  preset_list_->setBounds(0, 0, browse_width, getHeight());

  int options_height = kOptionsHeight * getHeight();
  int other_browse_x = getWidth() - browse_width - padding_width;
  int other_browse_height = (getHeight() - options_height - 2.0f * padding_width) / 3.0f;

  wavetable_list_->setBounds(other_browse_x, options_height, browse_width, other_browse_height);

  Rectangle<int> export_bounds = wavetable_list_->getBounds();
  export_bounds.setY(0);
  export_bounds.setHeight(wavetable_list_->getY() - findValue(Skin::kLargePadding));

  int lfo_y = wavetable_list_->getBottom() + padding_width;
  lfo_list_->setBounds(other_browse_x, lfo_y, browse_width, other_browse_height);

  int sample_y = getHeight() - other_browse_height;
  sample_list_->setBounds(other_browse_x, sample_y, browse_width, other_browse_height);

  int widget_margin = findValue(Skin::kWidgetMargin);
  int active_export_width = export_bounds.getWidth() - 3 * widget_margin;
  int bank_name_width = active_export_width / 2;
  int export_button_width = active_export_width - bank_name_width;
  int options_y = export_bounds.getY() + widget_margin;
  int option_component_height = export_bounds.getHeight() - 2 * widget_margin;

  int name_x = export_bounds.getX() + widget_margin;
  int export_button_x = name_x + bank_name_width + widget_margin;
  export_bank_button_->setBounds(export_button_x, options_y, export_button_width, option_component_height);

  if (bank_name_box_) {
    bank_name_box_->setBounds(name_x, options_y, bank_name_width, option_component_height);
    bank_name_box_->resized();
  }
}

void BankExporter::visibilityChanged() {
  SynthSection::visibilityChanged();
  if (isShowing())
    loadFiles();
}

void BankExporter::textEditorTextChanged(TextEditor& editor) {
  bool enabled = bank_name_box_ && bank_name_box_->getText().trim() != "";
  if (enabled == export_bank_button_->isEnabled())
    return;

  export_bank_button_->setEnabled(enabled);
  setButtonColors();
}

void BankExporter::setButtonColors() {
  if (export_bank_button_->isEnabled())
    export_bank_button_->setColour(Skin::kUiButton, findColour(Skin::kUiActionButton, true));
  else
    export_bank_button_->setColour(Skin::kUiButton, findColour(Skin::kUiButtonPressed, true));

  export_bank_button_->setColour(Skin::kUiButtonHover, findColour(Skin::kUiActionButtonHover, true));
  export_bank_button_->setColour(Skin::kUiButtonPressed, findColour(Skin::kUiActionButtonPressed, true));
}

void BankExporter::exportBank() {
  ZipFile::Builder bank_zip;
  String bank_name = bank_name_box_->getText().trim();
  if (bank_name.isEmpty())
    return;

  std::set<std::string> presets = preset_list_->selectedFiles();
  std::set<std::string> wavetables = wavetable_list_->selectedFiles();
  std::set<std::string> lfos = lfo_list_->selectedFiles();
  std::set<std::string> samples = sample_list_->selectedFiles();

  if (presets.empty() && wavetables.empty() && lfos.empty() && samples.empty())
    return;

  String preset_path = bank_name + "/" + LoadSave::kPresetFolderName + "/";
  for (const std::string& path : presets) {
    File preset(path);
    if (preset.exists())
      bank_zip.addFile(preset, 9, preset_path + getRelativePath(preset, LoadSave::kPresetFolderName));
  }

  String wavetable_path = bank_name + "/" + LoadSave::kWavetableFolderName + "/";
  for (const std::string& path : wavetables) {
    File wavetable(path);
    if (wavetable.exists())
      bank_zip.addFile(wavetable, 9, wavetable_path + getRelativePath(wavetable, LoadSave::kWavetableFolderName));
  }

  String lfo_path = bank_name + "/" + LoadSave::kLfoFolderName + "/";
  for (const std::string& path : lfos) {
    File lfo(path);
    if (lfo.exists())
      bank_zip.addFile(lfo, 9, lfo_path + getRelativePath(lfo, LoadSave::kLfoFolderName));
  }

  String sample_path = bank_name + "/" + LoadSave::kSampleFolderName + "/";
  for (const std::string& path : samples) {
    File sample(path);
    if (sample.exists())
      bank_zip.addFile(sample, 9, sample_path + getRelativePath(sample, LoadSave::kSampleFolderName));
  }

  File file = File::getCurrentWorkingDirectory().getChildFile(bank_name + "." + vital::kBankExtension);
  FileChooser export_box("Export Bank", file, String("*.") + vital::kBankExtension);
  if (export_box.browseForFileToSave(true)) {
    File destination = export_box.getResult().withFileExtension(vital::kBankExtension);
    if (destination.hasWriteAccess()) {
      FileOutputStream output_stream(destination);
      if (output_stream.openedOk())
        bank_zip.writeToStream(output_stream, nullptr);
    }
  }
}

void BankExporter::loadFiles() {
  Array<File> presets;
  LoadSave::getAllUserPresets(presets);
  preset_list_->setContent(presets);

  Array<File> wavetables;
  LoadSave::getAllUserWavetables(wavetables);
  wavetable_list_->setContent(wavetables);

  Array<File> lfos;
  LoadSave::getAllUserLfos(lfos);
  lfo_list_->setContent(lfos);

  Array<File> samples;
  LoadSave::getAllUserSamples(samples);
  sample_list_->setContent(samples);
}

void BankExporter::buttonClicked(Button* clicked_button) {
  if (clicked_button == export_bank_button_.get())
    exportBank();
}

bool BankExporter::keyPressed(const KeyPress &key, Component *origin) {
  if (key.getKeyCode() == KeyPress::escapeKey && isVisible()) {
    setVisible(false);
    return true;
  }
  return bank_name_box_->hasKeyboardFocus(true);
}

bool BankExporter::keyStateChanged(bool is_key_down, Component *origin) {
  if (is_key_down)
    return bank_name_box_->hasKeyboardFocus(true);
  return false;
}
