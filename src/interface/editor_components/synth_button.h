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

#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"
#include "synth_gui_interface.h"

class OpenGlShapeButtonComponent : public OpenGlComponent {
  public:
    static constexpr float kHoverInc = 0.2f;

    OpenGlShapeButtonComponent(Button* button) : button_(button), down_(false), hover_(false), hover_amount_(0.0f),
                                                 use_on_colors_(false), shape_("shape") {
      shape_.setComponent(button);
      shape_.setScissor(true);
    }

    void parentHierarchyChanged() override {
      if (findParentComponentOfClass<SynthGuiInterface>())
        setColors();
    }

    void setColors() {
      off_normal_color_ = button_->findColour(Skin::kIconButtonOff, true);
      off_hover_color_ = button_->findColour(Skin::kIconButtonOffHover, true);
      off_down_color_ = button_->findColour(Skin::kIconButtonOffPressed, true);
      on_normal_color_ = button_->findColour(Skin::kIconButtonOn, true);
      on_hover_color_ = button_->findColour(Skin::kIconButtonOnHover, true);
      on_down_color_ = button_->findColour(Skin::kIconButtonOnPressed, true);
    }

    void incrementHover();

    virtual void init(OpenGlWrapper& open_gl) override {
      OpenGlComponent::init(open_gl);
      shape_.init(open_gl);
    };

    virtual void render(OpenGlWrapper& open_gl, bool animate) override;

    virtual void destroy(OpenGlWrapper& open_gl) override {
      OpenGlComponent::destroy(open_gl);
      shape_.destroy(open_gl);
    };

    void redoImage() { shape_.redrawImage(true); setColors(); }

    void setShape(const Path& shape) { shape_.setShape(shape); }

    void useOnColors(bool use) { use_on_colors_ = use; }
  
    void setDown(bool down) { down_ = down; }
    void setHover(bool hover) { hover_ = hover; }

  private:
    Button* button_;

    bool down_;
    bool hover_;
    float hover_amount_;
    bool use_on_colors_;
    PlainShapeComponent shape_;
    Colour off_normal_color_;
    Colour off_hover_color_;
    Colour off_down_color_;
    Colour on_normal_color_;
    Colour on_hover_color_;
    Colour on_down_color_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlShapeButtonComponent)
};

class OpenGlShapeButton : public ToggleButton {
  public:
    OpenGlShapeButton(String name) : gl_component_(this) {
      setName(name);
    }

    OpenGlComponent* getGlComponent() { return &gl_component_; }

    void setShape(const Path& shape) { gl_component_.setShape(shape); }
    void useOnColors(bool use) { gl_component_.useOnColors(use); }

    void resized() override {
      ToggleButton::resized();
      gl_component_.redoImage();
    }
  
    void mouseEnter(const MouseEvent& e) override {
      ToggleButton::mouseEnter(e);
      gl_component_.setHover(true);
    }
  
    void mouseExit(const MouseEvent& e) override {
      ToggleButton::mouseExit(e);
      gl_component_.setHover(false);
    }
    
    void mouseDown(const MouseEvent& e) override {
      ToggleButton::mouseDown(e);
      gl_component_.setDown(true);
    }
    
    void mouseUp(const MouseEvent& e) override {
      ToggleButton::mouseUp(e);
      gl_component_.setDown(false);
    }

  private:
    OpenGlShapeButtonComponent gl_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlShapeButton)
};

class OpenGlButtonComponent : public OpenGlComponent {
  public:
    static constexpr float kHoverInc = 0.2f;

    enum ButtonStyle {
      kTextButton,
      kJustText,
      kPowerButton,
      kUiButton,
      kLightenButton,
      kNumButtonStyles
    };

    OpenGlButtonComponent(Button* button) : style_(kTextButton), button_(button),
                                            show_on_colors_(true), primary_ui_button_(false),
                                            down_(false), hover_(false), hover_amount_(0.0f),
                                            background_(Shaders::kRoundedRectangleFragment), text_("text", "") {
      background_.setTargetComponent(button);
      background_.setColor(Colours::orange);
      background_.setQuad(0, -1.0f, -1.0f, 2.0f, 2.0f);

      addChildComponent(text_);
      text_.setActive(false);
      text_.setScissor(true);
      text_.setComponent(button);
      text_.setFontType(PlainTextComponent::kMono);
    }

    virtual void init(OpenGlWrapper& open_gl) override {
      if (style_ == kPowerButton)
        background_.setFragmentShader(Shaders::kCircleFragment);

      background_.init(open_gl);
      text_.init(open_gl);

      setColors();
    }
  
    void setColors();

    void renderTextButton(OpenGlWrapper& open_gl, bool animate);
    void renderPowerButton(OpenGlWrapper& open_gl, bool animate);
    void renderUiButton(OpenGlWrapper& open_gl, bool animate);
    void renderLightenButton(OpenGlWrapper& open_gl, bool animate);
    void incrementHover();

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      if (style_ == kTextButton || style_ == kJustText)
        renderTextButton(open_gl, animate);
      else if (style_ == kPowerButton)
        renderPowerButton(open_gl, animate);
      else if (style_ == kUiButton)
        renderUiButton(open_gl, animate);
      else if (style_ == kLightenButton)
        renderLightenButton(open_gl, animate);
    }

    void setText() {
      String text = button_->getButtonText();
      if (!text.isEmpty()) {
        text_.setActive(true);
        text_.setText(text);
      }
    }
  
    void setDown(bool down) { down_ = down; }
    void setHover(bool hover) { hover_ = hover; }

    virtual void destroy(OpenGlWrapper& open_gl) override {
      background_.destroy(open_gl);
      text_.destroy(open_gl);
    }

    void setJustification(Justification justification) { text_.setJustification(justification); }
    void setStyle(ButtonStyle style) { style_ = style; }
    void setShowOnColors(bool show) { show_on_colors_ = show; }
    void setPrimaryUiButton(bool primary) { primary_ui_button_ = primary; }

    void paintBackground(Graphics& g) override { }

    OpenGlQuad& background() { return background_; }
    PlainTextComponent& text() { return text_; }
    ButtonStyle style() { return style_; }

  protected:
    ButtonStyle style_;
    Button* button_;
    bool show_on_colors_;
    bool primary_ui_button_;
    bool down_;
    bool hover_;
    float hover_amount_;
    OpenGlQuad background_;
    PlainTextComponent text_;

    Colour on_color_;
    Colour on_pressed_color_;
    Colour on_hover_color_;
    Colour off_color_;
    Colour off_pressed_color_;
    Colour off_hover_color_;
    Colour background_color_;
    Colour body_color_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlButtonComponent)
};

class OpenGlToggleButton : public ToggleButton {
  public:
    OpenGlToggleButton(String name) : ToggleButton(name), active_(true), button_component_(this) { }

    OpenGlButtonComponent* getGlComponent() { return &button_component_; }

    void setActive(bool active = true) { active_ = active; }
    bool isActive() const { return active_; }

    void resized() override;

    void setText(String text) {
      setButtonText(text);
      button_component_.setText();
    }

    void setPowerButton() {
      button_component_.setStyle(OpenGlButtonComponent::kPowerButton);
    }

    void setNoBackground() {
      button_component_.setStyle(OpenGlButtonComponent::kJustText);
    }

    void setJustification(Justification justification) {
      button_component_.setJustification(justification);
    }

    void setLightenButton() {
      button_component_.setStyle(OpenGlButtonComponent::kLightenButton);
    }

    void setShowOnColors(bool show) {
      button_component_.setShowOnColors(show);
    }

    void setUiButton(bool primary) {
      button_component_.setStyle(OpenGlButtonComponent::kUiButton);
      button_component_.setPrimaryUiButton(primary);
    }
  
    virtual void enablementChanged() override {
      ToggleButton::enablementChanged();
      button_component_.setColors();
    }
  
    void mouseEnter(const MouseEvent& e) override {
      ToggleButton::mouseEnter(e);
      button_component_.setHover(true);
    }
  
    void mouseExit(const MouseEvent& e) override {
      ToggleButton::mouseExit(e);
      button_component_.setHover(false);
    }
    
    void mouseDown(const MouseEvent& e) override {
      ToggleButton::mouseDown(e);
      button_component_.setDown(true);
    }
    
    void mouseUp(const MouseEvent& e) override {
      ToggleButton::mouseUp(e);
      button_component_.setDown(false);
    }

  private:
    bool active_;
    OpenGlButtonComponent button_component_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlToggleButton)
};

class SynthButton : public OpenGlToggleButton {
  public:
    enum MenuId {
      kCancel = 0,
      kArmMidiLearn,
      kClearMidiLearn
    };

    class ButtonListener {
      public:
        virtual ~ButtonListener() { }
        virtual void guiChanged(SynthButton* button) { }
    };

    SynthButton(String name);

    void setStringLookup(const std::string* lookup) {
      string_lookup_ = lookup;
    }
    const std::string* getStringLookup() const { return string_lookup_; }
    String getTextFromValue(bool value);

    void handlePopupResult(int result);

    virtual void mouseDown(const MouseEvent& e) override;
    virtual void mouseUp(const MouseEvent& e) override;

    void addButtonListener(ButtonListener* listener);

    virtual void clicked() override {
      OpenGlToggleButton::clicked();
      if (string_lookup_)
        setText(string_lookup_[getToggleState() ? 1 : 0]);
    }

  private:
    void clicked(const ModifierKeys& modifiers) override;

    const std::string* string_lookup_;

    std::vector<ButtonListener*> button_listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthButton)
};
