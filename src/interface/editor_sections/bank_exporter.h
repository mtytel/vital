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
#include "overlay.h"
#include "save_section.h"
#include "synth_section.h"

class ContentList : public SynthSection, ScrollBar::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void selectedPresetsChanged() = 0;
    };

    enum Column {
      kNone,
      kAdded,
      kName,
      kDate,
      kNumColumns
    };

    static constexpr int kNumCachedRows = 40;
    static constexpr float kRowHeight = 26.0f;
    static constexpr float kAddWidthRatio = 0.04f;
    static constexpr float kNameWidthRatio = 0.76f;
    static constexpr float kDateWidthRatio = 0.2f;
    static constexpr float kScrollSensitivity = 200.0f;

    class FileNameAscendingComparator {
      public:
        static int compareElements(const File& first, const File& second) {
          String first_name = first.getFileNameWithoutExtension().toLowerCase();
          String second_name = second.getFileNameWithoutExtension().toLowerCase();
          return first_name.compareNatural(second_name);
        }
    };

    class FileNameDescendingComparator {
      public:
        static int compareElements(const File& first, const File& second) {
          return -FileNameAscendingComparator::compareElements(first, second);
        }
    };

    class FileDateAscendingComparator {
      public:
        static int compareElements(const File& first, const File& second) {
          RelativeTime relative_time = first.getCreationTime() - second.getCreationTime();
          double days = relative_time.inDays();
          return days < 0.0 ? 1 : (days > 0.0f ? -1 : 0);
        }
    };

    class FileDateDescendingComparator {
      public:
        static int compareElements(const File& first, const File& second) {
          return -FileDateAscendingComparator::compareElements(first, second);
        }
    };

    class SelectedComparator {
      public:
        SelectedComparator(std::set<std::string> selected, bool ascending) :
            selected_(std::move(selected)), ascending_(ascending) { }

        inline bool isSelected(const File& file) {
          return selected_.count(file.getFullPathName().toStdString());
        }

        int compareElements(const File& first, const File& second) {
          int order_value = ascending_ ? 1 : -1;
          if (isSelected(first)) {
            if (isSelected(second))
              return 0;
            return -order_value;
          }
          if (isSelected(second))
            return order_value;
          return 0;
        }

      private:
        std::set<std::string> selected_;
        bool ascending_;
    };

    ContentList(const std::string& name);

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }
    void resized() override;

    void setContent(Array<File> presets);
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    int getRowFromPosition(float mouse_position);
    int getRowHeight() { return kRowHeight * size_ratio_; }
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void redoCache();
    int getScrollableRange();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }
    std::set<std::string> selectedFiles() { return selected_files_; };

  private:
    void viewPositionChanged();
    int getViewPosition() {
      int view_height = getHeight() - getTitleWidth();
      return std::max(0, std::min<int>(contents_.size() * getRowHeight() - view_height, view_position_));
    }
    void loadBrowserCache(int start_index, int end_index);
    void moveQuadToRow(OpenGlMultiQuad* quad, int index, int row, float y_offset);
    void sort();

    void selectHighlighted(int clicked_index); 
    void highlightClick(const MouseEvent& e, int clicked_index);
    void selectRange(int clicked_index);

    std::vector<Listener*> listeners_;
    Array<File> contents_;
    int num_contents_;
    std::set<std::string> selected_files_;
    std::set<std::string> highlighted_files_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    int last_selected_index_;
    int hover_index_;

    Component browse_area_;
    int cache_position_;
    float view_position_;
    Column sort_column_;
    bool sort_ascending_; 
    OpenGlImage rows_[kNumCachedRows];
    bool selected_[kNumCachedRows];
    OpenGlMultiQuad highlight_;
    OpenGlQuad hover_;
};

class BankExporter : public SynthSection, public TextEditor::Listener, public KeyListener {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;

        virtual void hideBankExporter() = 0;
    };

    BankExporter();
    ~BankExporter();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override;
    void resized() override;
    bool keyPressed(const KeyPress &key, Component *origin) override;
    bool keyStateChanged(bool is_key_down, Component *origin) override;
    void visibilityChanged() override;

    void buttonClicked(Button* clicked_button) override;
    void textEditorTextChanged(TextEditor& editor) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }

  private:
    void setButtonColors();
    void exportBank();
    void loadFiles();

    std::unique_ptr<ContentList> preset_list_;
    std::unique_ptr<ContentList> wavetable_list_;
    std::unique_ptr<ContentList> lfo_list_;
    std::unique_ptr<ContentList> sample_list_;

    std::unique_ptr<OpenGlTextEditor> bank_name_box_;
    std::unique_ptr<OpenGlToggleButton> export_bank_button_;

    std::vector<Listener*> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BankExporter)
};