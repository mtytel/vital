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
#include "line_map_editor.h"
#include "open_gl_image.h"
#include "overlay.h"
#include "preset_selector.h"
#include "synth_section.h"
#include "synth_constants.h"

class ModulationMatrixRow;
class ModulationMeterReadouts;
class SynthButton;
class SynthGuiInterface;
class PaintPatternSelector;

class ModulationSelector : public OpenGlSlider {
  public:
    static void modulationSelectionCallback(int result, ModulationSelector* selector) {
      if (selector != nullptr && result != 0)
        selector->setValue(result - 1);
    }
  
    ModulationSelector(String name, const std::vector<String>* selections, PopupItems* popup_items, bool dual_menu) :
        OpenGlSlider(std::move(name)), selections_(selections), popup_items_(popup_items), dual_menu_(dual_menu) {
      setRange(0.0, selections_->size() - 1.0, 1.0);
      setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    }
  
    String getTextFromValue(double value) override;
  
    String getSelection() {
      int index = std::round(getValue());
      return selections_->at(index);
    }
  
    bool connected() const {
      return getValue() != 0.0f;
    }
  
    void setValueFromName(const String& name, NotificationType notification_type);
  
    void mouseDown(const juce::MouseEvent &e) override;
  
  private:
    const std::vector<String>* selections_;
    PopupItems* popup_items_;
    bool dual_menu_;
};

class ModulationViewport : public Viewport {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void modulationScrolled(int position) = 0;
        virtual void startScroll() = 0;
        virtual void endScroll() = 0;
    };

    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override {
      for (Listener* listener : listeners_)
        listener->startScroll();

      Viewport::mouseWheelMove(e, wheel);

      for (Listener* listener : listeners_)
        listener->endScroll();
    }

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void visibleAreaChanged(const Rectangle<int>& visible_area) override {
      for (Listener* listener : listeners_)
        listener->modulationScrolled(visible_area.getY());

      Viewport::visibleAreaChanged(visible_area);
    }

  private:
    std::vector<Listener*> listeners_;
};

class ModulationMatrixRow : public SynthSection {
  public:
    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void rowSelected(ModulationMatrixRow* selected_row) = 0;
    };

    ModulationMatrixRow(int index, PopupItems* source_items, PopupItems* destination_items,
                        const std::vector<String>* sources, const std::vector<String>* destinations);

    void resized() override;
    void repaintBackground() override { }

    void setGuiParent(SynthGuiInterface* parent) { parent_ = parent; }
    void setConnection(vital::ModulationConnection* connection) { connection_ = connection; }

    void paintBackground(Graphics& g) override;
    void sliderValueChanged(Slider* changed_slider) override;
    void buttonClicked(Button* button) override;

    void updateDisplay();
    void updateDisplayValue();
    bool connected() const;

    bool matchesSourceAndDestination(const std::string& source, const std::string& destination) const;
    Rectangle<int> getMeterBounds();
    void select() {
      for (Listener* listener : listeners_)
        listener->rowSelected(this);
    }

    void mouseDown(const MouseEvent& e) override { select(); }

    void select(bool select);
    bool selected() const { return selected_; }
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    force_inline int index() const { return index_; }
    force_inline int source() const { return source_->getValue(); }
    force_inline int destination() const { return destination_->getValue(); }
    force_inline int stereo() const { return stereo_->getToggleState(); }
    force_inline int bipolar() const { return bipolar_->getToggleState(); }
    force_inline float morph() const { return power_slider_->getValue(); }
    force_inline float amount() const { return amount_slider_->getValue(); }

  protected:
    std::vector<Listener*> listeners_;

    int index_;
    vital::ModulationConnection* connection_;
    SynthGuiInterface* parent_;

    std::unique_ptr<ModulationSelector> source_;
    std::unique_ptr<ModulationSelector> destination_;
    double last_source_value_;
    double last_destination_value_;
    double last_amount_value_;
    std::unique_ptr<SynthSlider> amount_slider_;
    std::unique_ptr<SynthSlider> power_slider_;
    std::unique_ptr<OpenGlShapeButton> bipolar_;
    std::unique_ptr<SynthButton> stereo_;
    std::unique_ptr<SynthButton> bypass_;
    OverlayBackgroundRenderer highlight_;

    bool updating_;
    bool selected_;
};

class ModulationMatrix : public SynthSection, public ModulationViewport::Listener,
                         public ModulationMatrixRow::Listener, public ScrollBar::Listener,
                         public PresetSelector::Listener, public LineEditor::Listener {
  public:
    static constexpr int kRowPadding = 1;
    static constexpr int kDefaultGridSizeX = 8;
    static constexpr int kDefaultGridSizeY = 1;
    static String getMenuSourceDisplayName(const String& original);
    static String getUiSourceDisplayName(const String& original);
                           
    enum SortColumn {
      kNumber,
      kSource,
      kBipolar,
      kStereo,
      kMorph,
      kAmount,
      kDestination,
      kNumColumns
    };

    class Listener {
      public:
        virtual ~Listener() = default;
        virtual void modulationsScrolled() = 0;
    };

    ModulationMatrix(const vital::output_map& sources, const vital::output_map& destinations);
    virtual ~ModulationMatrix();

    void paintBackground(Graphics& g) override;
    void paintBackgroundShadow(Graphics& g) override;
    void paintScrollableBackground();
    void resized() override;
    void setMeterBounds();
    void setVisible(bool should_be_visible) override {
      SynthSection::setVisible(should_be_visible);
      updateModulations();
    }
    void setRowPositions();
    void parentHierarchyChanged() override;
    void sliderValueChanged(Slider* changed_slider) override;
    void buttonClicked(Button* button) override;
    void setAllValues(vital::control_map& controls) override;

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;

    void prevClicked() override;
    void nextClicked() override;
    void textMouseDown(const MouseEvent& e) override;

    void setPhase(float phase) override { }
    void lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void togglePaintMode(bool enabled, bool temporary_switch) override;
    void importLfo() override;
    void exportLfo() override;
    void fileLoaded() override;

    void loadFile(const File& file) override;
    File getCurrentFile() override { return current_file_; }

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void updateModulations();
    void updateModulationValue(int index);
    void checkNumModulationsShown();

    void addListener(Listener* listener) { listeners_.push_back(listener); }

    void startScroll() override {
      open_gl_critical_section_.enter();
    }

    void endScroll() override {
      open_gl_critical_section_.exit();
    }

    void modulationScrolled(int position) override {
      setScrollBarRange();
      scroll_bar_->setCurrentRange(position, viewport_.getHeight());
      for (Listener* listener : listeners_)
        listener->modulationsScrolled();
    }

    void rowSelected(ModulationMatrixRow* selected_row) override;
    void mouseDown(const MouseEvent& e) override;

  private:
    void sort();
    int getRowHeight() { return getSizeRatio() * 34.0f; }

    std::vector<Listener*> listeners_;

    PopupItems source_popup_items_;
    PopupItems destination_popup_items_;

    File current_file_;
    SortColumn sort_column_;
    bool sort_ascending_;
    int selected_index_;
    int num_shown_;
    std::vector<ModulationMatrixRow*> row_order_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;

    CriticalSection open_gl_critical_section_;
    std::unique_ptr<ModulationMatrixRow> rows_[vital::kMaxModulationConnections];
    std::unique_ptr<LineMapEditor> map_editors_[vital::kMaxModulationConnections];
    std::vector<String> source_strings_;
    std::vector<String> destination_strings_;
    std::unique_ptr<ModulationMeterReadouts> readouts_;

    ModulationViewport viewport_;
    Component container_;
  
    OpenGlImage background_;

    std::unique_ptr<PlainTextComponent> remap_name_;
    std::unique_ptr<PresetSelector> preset_selector_;
    std::unique_ptr<PaintPatternSelector> paint_pattern_;

    std::unique_ptr<SynthSlider> grid_size_x_;
    std::unique_ptr<SynthSlider> grid_size_y_;
    std::unique_ptr<OpenGlShapeButton> paint_;
    std::unique_ptr<OpenGlShapeButton> smooth_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrix)
};

