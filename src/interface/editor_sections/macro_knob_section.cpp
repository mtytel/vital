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

#include "macro_knob_section.h"

#include "fonts.h"
#include "modulation_button.h"
#include "synth_slider.h"
#include "synth_gui_interface.h"

class MacroLabel : public OpenGlImageComponent {
  public:
    MacroLabel(String name, String text) : OpenGlImageComponent(name), text_(std::move(text)), text_size_(1.0f) {
      setInterceptsMouseClicks(false, false);
    }

    void setText(String text) { text_ = text; redrawImage(true); }
    void setTextSize(float size) { text_size_ = size; redrawImage(true); }
    String getText() { return text_; }

    void paint(Graphics& g) override {
      g.setColour(findColour(Skin::kBodyText, true));
      g.setFont(Fonts::instance()->proportional_regular().withPointHeight(text_size_));
      g.drawText(text_, 0, 0, getWidth(), getHeight(), Justification::centred, false);
    }

  private:
    String text_;
    float text_size_;
};

class SingleMacroSection : public SynthSection, public TextEditor::Listener {
  public:
    SingleMacroSection(String name, int index) : SynthSection(name), index_(index) {
      std::string number = std::to_string(index_ + 1);
      std::string control_name = "macro_control_" + number;

      macro_knob_ = std::make_unique<SynthSlider>(control_name);
      addSlider(macro_knob_.get());
      macro_knob_->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
      macro_knob_->setPopupPlacement(BubbleComponent::right);

      macro_source_ = std::make_unique<ModulationButton>(control_name);
      addModulationButton(macro_source_.get());
      macro_source_->overrideText("");

      macro_label_ = std::make_unique<MacroLabel>("Macro Label " + number, "MACRO " + number);
      addOpenGlComponent(macro_label_.get());

      edit_label_ = std::make_unique<OpenGlShapeButton>("Edit " + number);
      addAndMakeVisible(edit_label_.get());
      addOpenGlComponent(edit_label_->getGlComponent());
      edit_label_->addListener(this);
      edit_label_->setShape(Paths::pencil());
      edit_label_->setTriggeredOnMouseDown(true);

      setSkinOverride(Skin::kMacro);

#if !defined(NO_TEXT_ENTRY)
      macro_label_editor_ = std::make_unique<OpenGlTextEditor>("Search");
      macro_label_editor_->addListener(this);
      macro_label_editor_->setSelectAllWhenFocused(true);
      macro_label_editor_->setMultiLine(false, false);
      macro_label_editor_->setJustification(Justification::centred);
      addChildComponent(macro_label_editor_.get());
      addOpenGlComponent(macro_label_editor_->getImageComponent());
#endif
    }

    void resized() override {
      int knob_height = getHeight() / 2;
      int button_height = getHeight() - knob_height;
      int width = getWidth();

      macro_knob_->setBounds(0, 0, width, knob_height);
      placeRotaryOption(edit_label_.get(), macro_knob_.get());

      macro_source_->setBounds(0, knob_height, width, button_height);
      macro_source_->setFontSize(0);

      macro_label_->setBounds(getLabelBackgroundBounds(macro_knob_.get()));
      macro_label_->setTextSize(findValue(Skin::kLabelHeight));
    }

    void paintBackground(Graphics& g) override {
      paintBody(g);
      paintMacroSourceBackground(g);
      setLabelFont(g);

      drawLabelBackgroundForComponent(g, macro_knob_.get());
      paintKnobShadows(g);
      paintChildrenBackgrounds(g);
      paintBorder(g);
    }

    void paintBackgroundShadow(Graphics& g) override { paintTabShadow(g); }

    void paintMacroSourceBackground(Graphics& g) {
      g.saveState();
      Rectangle<int> bounds = getLocalArea(macro_source_.get(), macro_source_->getLocalBounds());
      g.reduceClipRegion(bounds);
      g.setOrigin(bounds.getTopLeft());
      macro_source_->paintBackground(g);
      g.restoreState();
    }

    void buttonClicked(Button* clicked_button) override {
      if (macro_label_editor_) {
        if (macro_label_editor_->isVisible()) {
          saveMacroLabel();
          return;
        }

        Rectangle<int> bounds = macro_label_->getBounds();
        float text_height = findValue(Skin::kLabelHeight);
        macro_label_editor_->setFont(Fonts::instance()->proportional_regular().withPointHeight(text_height));
        macro_label_editor_->setText(macro_label_->getText());
        macro_label_editor_->setBounds(bounds.translated(0, -1));
        macro_label_editor_->setVisible(true);
        macro_label_editor_->grabKeyboardFocus();
      }
    }

    void saveMacroLabel() {
      macro_label_editor_->setVisible(false);
      String text = macro_label_editor_->getText().trim().toUpperCase();
      if (text.isEmpty())
        return;

      macro_label_->setText(text);

      SynthGuiInterface* synth_gui_interface = findParentComponentOfClass<SynthGuiInterface>();
      if (synth_gui_interface)
        synth_gui_interface->getSynth()->setMacroName(index_, text);
    }

    void textEditorReturnKeyPressed(TextEditor& text_editor) override {
      saveMacroLabel();
    }

    void textEditorFocusLost(TextEditor& text_editor) override {
      saveMacroLabel();
    }

    void textEditorEscapeKeyPressed(TextEditor& editor) override {
      macro_label_editor_->setVisible(false);
    }

    void reset() override {
      SynthGuiInterface* synth_gui_interface = findParentComponentOfClass<SynthGuiInterface>();
      if (synth_gui_interface == nullptr)
        return;

      macro_label_->setText(synth_gui_interface->getSynth()->getMacroName(index_));
    }

  private:
    int index_;
    std::unique_ptr<SynthSlider> macro_knob_;
    std::unique_ptr<ModulationButton> macro_source_;
    std::unique_ptr<MacroLabel> macro_label_;
    std::unique_ptr<OpenGlTextEditor> macro_label_editor_;
    std::unique_ptr<OpenGlShapeButton> edit_label_;
};

MacroKnobSection::MacroKnobSection(String name) : SynthSection(name) {
  setWantsKeyboardFocus(true);
  for (int i = 0; i < vital::kNumMacros; ++i) {
    macros_[i] = std::make_unique<SingleMacroSection>(name + String(i), i);
    addSubSection(macros_[i].get());
  }

  setSkinOverride(Skin::kMacro);
}

MacroKnobSection::~MacroKnobSection() { }

void MacroKnobSection::paintBackground(Graphics& g) {
  paintChildrenBackgrounds(g);
}

void MacroKnobSection::resized() {
  int padding = getPadding();
  int knob_section_height = getKnobSectionHeight();
  int widget_margin = getWidgetMargin();
  int width = getWidth();
  int y = 0;
  for (int i = 0; i < vital::kNumMacros; ++i) {
    float next_y = (i + 1) * (2 * knob_section_height - widget_margin + padding);
    macros_[i]->setBounds(0, y, width, next_y - y - padding);
    y = next_y;
  }

  int last_y = macros_[vital::kNumMacros - 2]->getBottom() + padding;
  macros_[vital::kNumMacros - 1]->setBounds(0, last_y, width, getHeight() - last_y);

  reset();
  SynthSection::resized();
}
