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

#pragma once

#include "JuceHeader.h"
#include "delete_section.h"
#include "load_save.h"
#include "open_gl_image.h"
#include "open_gl_image_component.h"
#include "open_gl_multi_image.h"
#include "open_gl_multi_quad.h"
#include "overlay.h"
#include "save_section.h"
#include "synth_section.h"

class PopupDisplay : public SynthSection {
  public:
    PopupDisplay();

    void resized() override;

    void setContent(const std::string& text, Rectangle<int> bounds, BubbleComponent::BubblePlacement placement);

  private:
    PlainTextComponent text_;
    OpenGlQuad body_;
    OpenGlQuad border_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupDisplay)
};

class PopupList : public SynthSection, ScrollBar::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void newSelection(PopupList* list, int id, int index) = 0;
        virtual void doubleClickedSelected(PopupList* list, int id, int index) { }
    };

    static constexpr float kRowHeight = 24.0f;
    static constexpr float kScrollSensitivity = 200.0f;
    static constexpr float kScrollBarWidth = 15.0f;

    PopupList();

    void paintBackground(Graphics& g) override { }
    void paintBackgroundShadow(Graphics& g) override { }
    void resized() override;

    void setSelections(PopupItems selections);
    PopupItems getSelectionItems(int index) const { return selections_.items[index]; }
    int getRowFromPosition(float mouse_position);
    int getRowHeight() { return size_ratio_ * kRowHeight; }
    int getTextPadding() { return getRowHeight() / 4; }
    int getBrowseWidth();
    int getBrowseHeight() { return getRowHeight() * selections_.size(); }
    Font getFont() {
      return Fonts::instance()->proportional_light().withPointHeight(getRowHeight() * 0.55f * getPixelMultiple());
    }
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    int getSelection(const MouseEvent& e);

    void mouseUp(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;

    void setSelected(int selection) { selected_ = selection; }
    int getSelected() const { return selected_; }
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void resetScrollPosition();
    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();
    int getScrollableRange();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }
    void showSelected(bool show) { show_selected_ = show; }
    void select(int select);

  private:
    int getViewPosition() {
      int view_height = getHeight();
      return std::max(0, std::min<int>(selections_.size() * getRowHeight() - view_height, view_position_));
    }
    void redoImage();
    void moveQuadToRow(OpenGlQuad& quad, int row);

    std::vector<Listener*> listeners_;
    PopupItems selections_;
    int selected_;
    int hovered_;
    bool show_selected_;

    float view_position_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    OpenGlImage rows_;
    OpenGlQuad highlight_;
    OpenGlQuad hover_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupList)
};

class SelectionList : public SynthSection, ScrollBar::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void newSelection(File selection) = 0;
        virtual void allSelected() { }
        virtual void favoritesSelected() { }
        virtual void doubleClickedSelected(File selection) = 0;
    };

    static constexpr int kNumCachedRows = 50;
    static constexpr float kRowHeight = 24.0f;
    static constexpr float kStarWidth = 38.0f;
    static constexpr float kScrollSensitivity = 200.0f;
    static constexpr float kScrollBarWidth = 15.0f;

  static File getAllFile() { return File::getSpecialLocation(File::tempDirectory).getChildFile("All"); }
  static File getFavoritesFile() { return File::getSpecialLocation(File::tempDirectory).getChildFile("Favorites"); }

    class FileNameAscendingComparator {
      public:
        static int compareElements(const File& first, const File& second) {
          String first_name = first.getFullPathName().toLowerCase();
          String second_name = second.getFullPathName().toLowerCase();
          return first_name.compareNatural(second_name);
        }
    };

    SelectionList();

    void paintBackground(Graphics& g) override { }
    void paintBackgroundShadow(Graphics& g) override { }
    void resized() override;

    void addFavoritesOption() { favorites_option_ = true; selected_ = getAllFile(); }
    void setSelections(Array<File> presets);
    Array<File> getSelections() { return selections_; }
    Array<File> getAdditionalFolders() { return additional_roots_; }
    void resetScrollPosition();
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    int getRowFromPosition(float mouse_position);
    int getRowHeight() { return size_ratio_ * kRowHeight; }
    int getIconPadding() { return getRowHeight() * 0.25f; }
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void respondToMenuCallback(int result);
    void menuClick(const MouseEvent& e);
    void leftClick(const MouseEvent& e);
    File getSelection(const MouseEvent& e);
    void mouseDown(const MouseEvent& e) override;
    void mouseDoubleClick(const MouseEvent& e) override;
    void addAdditionalFolder();
    void removeAdditionalFolder(const File& folder);

    void select(const File& selection);
    File selected() const { return selected_; }
    void setSelected(const File& selection) { selected_ = selection; }
    void selectIcon(const File& selection);

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void redoCache();
    int getFolderDepth(const File& file);
    void addSubfolderSelections(const File& selection, std::vector<File>& selections);
    void setAdditionalRootsName(const std::string& name);
    void filter(const String& filter_string);
    int getSelectedIndex();
    int getScrollableRange();
    void selectNext();
    void selectPrev();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }

    void setPassthroughFolderName(const std::string& name) { passthrough_name_ = name; }
    std::string getPassthroughFolderName() const { return passthrough_name_; }
    bool hasValidPath() {
      for (File& selection : selections_) {
        if (selection.exists())
          return true;
      }
      return false;
    }

  private:
    void viewPositionChanged();
    void toggleFavorite(const File& selection);
    void toggleOpenFolder(const File& selection);
    int getViewPosition() {
      int view_height = getHeight();
      return std::max(0, std::min<int>(num_view_selections_ * getRowHeight() - view_height, view_position_));
    }
    void loadBrowserCache(int start_index, int end_index);
    void moveQuadToRow(OpenGlQuad& quad, int row, float y_offset);
    void sort();

    bool favorites_option_;
    std::vector<Listener*> listeners_;
    Array<File> selections_;
    std::string additional_roots_name_;
    Array<File> additional_roots_;
    int num_view_selections_;
    std::vector<File> filtered_selections_;
    std::set<std::string> favorites_;
    std::map<std::string, int> open_folders_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    String filter_string_;
    File selected_;
    int hovered_;
    bool x_area_;

    Component browse_area_;
    int cache_position_;
    OpenGlImage rows_[kNumCachedRows];
    bool is_additional_[kNumCachedRows];
    OpenGlQuad highlight_;
    OpenGlQuad hover_;
    PlainShapeComponent remove_additional_x_;
    float view_position_;
    std::string passthrough_name_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectionList)
};

class SinglePopupSelector : public SynthSection, public PopupList::Listener {
  public:
    SinglePopupSelector();

    void paintBackground(Graphics& g) override { }
    void paintBackgroundShadow(Graphics& g) override { }
    void resized() override;

    void visibilityChanged() override {
      if (isShowing() && isVisible())
        grabKeyboardFocus();
    }

    void setPosition(Point<int> position, Rectangle<int> bounds);

    void newSelection(PopupList* list, int id, int index) override {
      if (id >= 0) {
        cancel_ = nullptr;
        callback_(id);
        setVisible(false);
      }
      else
        cancel_();
    }

    void focusLost(FocusChangeType cause) override {
      setVisible(false);
      if (cancel_)
        cancel_();
    }

    void setCallback(std::function<void(int)> callback) { callback_ = std::move(callback); }
    void setCancelCallback(std::function<void()> cancel) { cancel_ = std::move(cancel); }

    void showSelections(const PopupItems& selections) {
      popup_list_->setSelections(selections);
    }

  private:
    OpenGlQuad body_;
    OpenGlQuad border_;

    std::function<void(int)> callback_;
    std::function<void()> cancel_;
    std::unique_ptr<PopupList> popup_list_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SinglePopupSelector)
};

class DualPopupSelector : public SynthSection, public PopupList::Listener {
  public:
    DualPopupSelector();

    void paintBackground(Graphics& g) override { }
    void paintBackgroundShadow(Graphics& g) override { }
    void resized() override;
    void visibilityChanged() override {
      if (isShowing() && isVisible())
        grabKeyboardFocus();
    }

    void setPosition(Point<int> position, int width, Rectangle<int> bounds);

    void newSelection(PopupList* list, int id, int index) override;
    void doubleClickedSelected(PopupList* list, int id, int index) override { setVisible(false); }

    void focusLost(FocusChangeType cause) override { setVisible(false); }

    void setCallback(std::function<void(int)> callback) { callback_ = std::move(callback); }

    void showSelections(const PopupItems& selections) {
      left_list_->setSelections(selections);

      for (int i = 0; i < selections.size(); ++i) {
        if (selections.items[i].selected)
          right_list_->setSelections(selections.items[i]);
      }
    }

  private:
    OpenGlQuad body_;
    OpenGlQuad border_;
    OpenGlQuad divider_;

    std::function<void(int)> callback_;
    std::unique_ptr<PopupList> left_list_;
    std::unique_ptr<PopupList> right_list_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DualPopupSelector)
};

class PopupClosingArea : public Component {
  public:
    PopupClosingArea() : Component("Ignore Area") { }
  
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void closingAreaClicked(PopupClosingArea* closing_area, const MouseEvent& e) = 0;
    };
  
    void mouseDown(const MouseEvent& e) override {
      for (Listener* listener : listeners_)
        listener->closingAreaClicked(this, e);
    }

    void addListener(Listener* listener) { listeners_.push_back(listener); }
  
  private:
    std::vector<Listener*> listeners_;
};

class PopupBrowser : public SynthSection,
                     public SelectionList::Listener,
                     public TextEditor::Listener,
                     public KeyListener,
                     public PopupClosingArea::Listener {
  public:
    PopupBrowser();
    ~PopupBrowser();

    void paintBackground(Graphics& g) override { }
    void paintBackgroundShadow(Graphics& g) override { }
    void resized() override;
    void buttonClicked(Button* clicked_button) override;
    void visibilityChanged() override;

    void filterPresets();
    void textEditorTextChanged(TextEditor& editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;

    void newSelection(File selection) override;
    void allSelected() override;
    void favoritesSelected() override;
    void doubleClickedSelected(File selection) override;
    void closingAreaClicked(PopupClosingArea* closing_area, const MouseEvent& e) override;
                       
    bool keyPressed(const KeyPress &key, Component *origin) override;
    bool keyStateChanged(bool is_key_down, Component *origin) override;

    void checkNoContent();
    void checkStoreButton();
    void loadPresets(std::vector<File> directories, const String& extensions,
                     const std::string& passthrough_name, const std::string& additional_folders_name);

    void setOwner(SynthSection* owner) {
      owner_ = owner;
      if (owner_)
        selection_list_->setSelected(owner_->getCurrentFile());
      checkStoreButton();
    }
    void setIgnoreBounds(Rectangle<int> bounds) { passthrough_bounds_ = bounds; resized(); }
    void setBrowserBounds(Rectangle<int> bounds) { browser_bounds_ = bounds; resized(); }

  private:
    OpenGlQuad body_;
    OpenGlQuad border_;
    OpenGlQuad horizontal_divider_;
    OpenGlQuad vertical_divider_;

    std::unique_ptr<SelectionList> folder_list_;
    std::unique_ptr<SelectionList> selection_list_;
    std::unique_ptr<OpenGlTextEditor> search_box_;
    std::unique_ptr<OpenGlShapeButton> exit_button_;
    std::unique_ptr<OpenGlToggleButton> store_button_;
    std::unique_ptr<OpenGlToggleButton> download_button_;
    Rectangle<int> passthrough_bounds_;
    Rectangle<int> browser_bounds_;
    PopupClosingArea closing_areas_[4];

    SynthSection* owner_;
    String extensions_;
    String author_;
    std::set<std::string> more_author_presets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupBrowser)
};
