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
#include "popup_browser.h"
#include "save_section.h"
#include "synth_section.h"

class PresetInfoCache {
  public:
    std::string getAuthor(const File& preset) {
      std::string path = preset.getFullPathName().toStdString();
      if (author_cache_.count(path) == 0)
        author_cache_[path] = LoadSave::getAuthorFromFile(preset).toStdString();

      return author_cache_[path];
    }

    std::string getStyle(const File& preset) {
      std::string path = preset.getFullPathName().toStdString();
      if (style_cache_.count(path) == 0)
        style_cache_[path] = LoadSave::getStyleFromFile(preset).toLowerCase().toStdString();

      return style_cache_[path];
    }

  private:
    std::map<std::string, std::string> author_cache_;
    std::map<std::string, std::string> style_cache_;
};

class PresetList : public SynthSection, public TextEditor::Listener, ScrollBar::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void newPresetSelected(File preset) = 0;
        virtual void deleteRequested(File preset) = 0;
    };

    enum Column {
      kNone,
      kStar,
      kName,
      kStyle,
      kAuthor,
      kDate,
      kNumColumns
    };

    enum MenuOptions {
      kCancel,
      kOpenFileLocation,
      kRename,
      kDelete,
      kNumMenuOptions
    };

    static constexpr int kNumCachedRows = 50;
    static constexpr float kRowSizeHeightPercent = 0.04f;
    static constexpr float kStarWidthPercent = 0.04f;
    static constexpr float kNameWidthPercent = 0.35f;
    static constexpr float kStyleWidthPercent = 0.18f;
    static constexpr float kAuthorWidthPercent = 0.25f;
    static constexpr float kDateWidthPercent = 0.18f;
    static constexpr float kScrollSensitivity = 200.0f;

    class FileNameAscendingComparator {
      public:
        static int compareElements(File first, File second) {
          String first_name = first.getFileNameWithoutExtension().toLowerCase();
          String second_name = second.getFileNameWithoutExtension().toLowerCase();
          return first_name.compareNatural(second_name);
        }
    };

    class FileNameDescendingComparator {
      public:
        static int compareElements(File first, File second) {
          return FileNameAscendingComparator::compareElements(second, first);
        }
    };

    class AuthorAscendingComparator {
      public:
        AuthorAscendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }
        
        int compareElements(File first, File second) {
          String first_author = cache_->getAuthor(first);
          String second_author = cache_->getAuthor(second);
          return first_author.compareNatural(second_author);
        }

      private:
        PresetInfoCache* cache_;
    };

    class AuthorDescendingComparator {
      public:
        AuthorDescendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }

        int compareElements(File first, File second) {
          String first_author = cache_->getAuthor(first);
          String second_author = cache_->getAuthor(second);
          return -first_author.compareNatural(second_author);
        }

      private:
        PresetInfoCache* cache_;
    };

    class StyleAscendingComparator {
      public:
        StyleAscendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }

        int compareElements(File first, File second) {
          String first_style = cache_->getStyle(first);
          String second_style = cache_->getStyle(second);
          return first_style.compareNatural(second_style);
        }

      private:
        PresetInfoCache* cache_;
    };

    class StyleDescendingComparator {
      public:
        StyleDescendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }
        
        int compareElements(File first, File second) {
          String first_style = cache_->getStyle(first);
          String second_style = cache_->getStyle(second);
          return -first_style.compareNatural(second_style);
        }

      private:
        PresetInfoCache* cache_;
    };

    class FileDateAscendingComparator {
      public:
        static int compareElements(File first, File second) {
          RelativeTime relative_time = first.getCreationTime() - second.getCreationTime();
          double days = relative_time.inDays();
          return days < 0.0 ? 1 : (days > 0.0f ? -1 : 0);
        }
    };

    class FileDateDescendingComparator {
      public:
        static int compareElements(File first, File second) {
          return FileDateAscendingComparator::compareElements(second, first);
        }
    };

    class FavoriteComparator {
      public:
        FavoriteComparator() {
          favorites_ = LoadSave::getFavorites();
        }

        bool isFavorite(const File& file) {
          return favorites_.count(file.getFullPathName().toStdString());
        }

        int compare(File first, File second) {
          if (isFavorite(first)) {
            if (isFavorite(second))
              return 0;
            return -1;
          }
          if (isFavorite(second))
            return 1;
          return 0;
        }

      private:
        std::set<std::string> favorites_;
    };

    class FavoriteAscendingComparator : public FavoriteComparator {
      public:
        int compareElements(File first, File second) {
          return compare(first, second);
        }
    };

    class FavoriteDescendingComparator : public FavoriteComparator {
      public:
        int compareElements(File first, File second) {
          return compare(second, first);
        }
    };

    PresetList();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void resized() override;

    void setPresets(Array<File> presets);
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    int getRowFromPosition(float mouse_position);
    int getRowHeight() { return getHeight() * kRowSizeHeightPercent; }
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void respondToMenuCallback(int result);
    void menuClick(const MouseEvent& e);
    void leftClick(const MouseEvent& e);

    void mouseDown(const MouseEvent& e) override;

    void textEditorReturnKeyPressed(TextEditor& text_editor) override;
    void textEditorFocusLost(TextEditor& text_editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void finishRename();
    void reloadPresets();
    void shiftSelectedPreset(int indices);

    void redoCache();
    void filter(String filter_string, const std::set<std::string>& styles);
    int getSelectedIndex();
    int getScrollableRange();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }
    void setCurrentFolder(const File& folder) {
      current_folder_ = folder;
      reloadPresets();
    }

  private:
    void viewPositionChanged();
    int getViewPosition() {
      int view_height = getHeight() - getTitleWidth();
      return std::max(0, std::min<int>(num_view_presets_ * getRowHeight() - view_height, view_position_));
    }
    void loadBrowserCache(int start_index, int end_index);
    void moveQuadToRow(OpenGlQuad& quad, int row, float y_offset);
    void sort();

    std::vector<Listener*> listeners_;
    Array<File> presets_;
    int num_view_presets_;
    std::vector<File> filtered_presets_;
    std::set<std::string> favorites_;
    std::unique_ptr<OpenGlTextEditor> rename_editor_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    String filter_string_;
    std::set<std::string> filter_styles_;
    File selected_preset_;
    File renaming_preset_;
    File current_folder_;
    int hover_preset_;
    int click_preset_;

    PresetInfoCache preset_info_cache_;

    Component browse_area_;
    int cache_position_;
    OpenGlImage rows_[kNumCachedRows];
    OpenGlQuad highlight_;
    OpenGlQuad hover_;
    float view_position_;
    Column sort_column_;
    bool sort_ascending_;
};

class PresetBrowser : public SynthSection,
                      public PresetList::Listener,
                      public TextEditor::Listener,
                      public KeyListener,
                      public SaveSection::Listener,
                      public DeleteSection::Listener,
                      public SelectionList::Listener {
  public:
    static constexpr int kLeftPadding = 24;
    static constexpr int kTopPadding = 24;
    static constexpr int kMiddlePadding = 15;
    static constexpr int kNameFontHeight = 26;
    static constexpr int kAuthorFontHeight = 19;
    static constexpr int kStoreHeight = 33;
    static constexpr int kCommentsFontHeight = 15;

    class Listener {
      public:
        virtual ~Listener() { }

        virtual void newPresetSelected(File preset) = 0;
        virtual void deleteRequested(File preset) = 0;
        virtual void hidePresetBrowser() = 0;
    };

    PresetBrowser();
    ~PresetBrowser();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override;
    void resized() override;
    void buttonClicked(Button* clicked_button) override;
    bool keyPressed(const KeyPress &key, Component *origin) override;
    bool keyStateChanged(bool is_key_down, Component *origin) override;
    void visibilityChanged() override;

    Rectangle<int> getSearchRect();
    Rectangle<int> getInfoRect();

    void filterPresets();
    void textEditorTextChanged(TextEditor& editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;

    void newPresetSelected(File preset) override {
      for (Listener* listener : listeners_)
        listener->newPresetSelected(preset);
      loadPresetInfo();

      String author = author_text_->getText();
      store_button_->setText("Get more presets by " + author);
      bool visible = more_author_presets_.count(author.removeCharacters(" _.").toLowerCase().toStdString());
      bool was_visible = store_button_->isVisible();
      store_button_->setVisible(visible);
      if (was_visible != visible)
        setCommentsBounds();
    }

    void deleteRequested(File preset) override {
      for (Listener* listener : listeners_)
        listener->deleteRequested(preset);
    }

    void loadPresets();
    void save(File preset) override;
    void fileDeleted(File deleted_file) override;

    void jumpToPreset(int indices);
    void loadNextPreset();
    void loadPrevPreset();
    void externalPresetLoaded(File file);
    void clearExternalPreset() { external_preset_ = File(); }

    void addListener(Listener* listener);
    void setSaveSection(SaveSection* save_section);
    void setDeleteSection(DeleteSection* delete_section);

    void newSelection(File selection) override;
    void allSelected() override;
    void favoritesSelected() override;
    void doubleClickedSelected(File selection) override { }

  private:
    bool loadFromFile(File& preset);
    void loadPresetInfo();
    void setCommentsBounds();
    void setPresetInfo(File& preset);

    std::vector<Listener*> listeners_;
    std::unique_ptr<PresetList> preset_list_;
    std::unique_ptr<OpenGlTextEditor> search_box_;
    std::unique_ptr<SelectionList> folder_list_;
    std::unique_ptr<PlainTextComponent> preset_text_;
    std::unique_ptr<PlainTextComponent> author_text_;
    std::unique_ptr<OpenGlToggleButton> style_buttons_[LoadSave::kNumPresetStyles];
    std::unique_ptr<OpenGlToggleButton> store_button_;

    SaveSection* save_section_;
    DeleteSection* delete_section_;

    std::unique_ptr<OpenGlTextEditor> comments_;
    File external_preset_;
    String author_;
    String license_;
    std::set<std::string> more_author_presets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
