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

#include "open_gl_component.h"
#include "fonts.h"
#include "open_gl_image.h"

class SynthSection;

class OpenGlImageComponent : public OpenGlComponent {
  public:
    OpenGlImageComponent(String name = "");
    virtual ~OpenGlImageComponent() = default;

    virtual void paintBackground(Graphics& g) override {
      redrawImage(false);
    }

    virtual void paintToImage(Graphics& g) {
      Component* component = component_ ? component_ : this;
      if (paint_entire_component_)
        component->paintEntireComponent(g, false);
      else
        component->paint(g);
    }

    virtual void init(OpenGlWrapper& open_gl) override;
    virtual void render(OpenGlWrapper& open_gl, bool animate) override;
    virtual void destroy(OpenGlWrapper& open_gl) override;

    virtual void redrawImage(bool force);
    void setComponent(Component* component) { component_ = component; }
    void setScissor(bool scissor) { image_.setScissor(scissor); }
    void setUseAlpha(bool use_alpha) { image_.setUseAlpha(use_alpha); }
    void setColor(Colour color) { image_.setColor(color); }
    OpenGlImage& image() { return image_; }
    void setActive(bool active) { active_ = active; }
    void setStatic(bool static_image) { static_image_ = static_image; }
    void paintEntireComponent(bool paint_entire_component) { paint_entire_component_ = paint_entire_component; }
    bool isActive() const { return active_; }

  protected:
    Component* component_;
    bool active_;
    bool static_image_;
    bool paint_entire_component_;
    std::unique_ptr<Image> draw_image_;
    OpenGlImage image_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlImageComponent)
};


template <class ComponentType>
class OpenGlAutoImageComponent : public ComponentType {
  public:
    using ComponentType::ComponentType;

    virtual void mouseDown(const MouseEvent& e) override {
      ComponentType::mouseDown(e);
      redoImage();
    }

    virtual void mouseUp(const MouseEvent& e) override {
      ComponentType::mouseUp(e);
      redoImage();
    }

    virtual void mouseDoubleClick(const MouseEvent& e) override {
      ComponentType::mouseDoubleClick(e);
      redoImage();
    }

    virtual void mouseEnter(const MouseEvent& e) override {
      ComponentType::mouseEnter(e);
      redoImage();
    }

    virtual void mouseExit(const MouseEvent& e) override {
      ComponentType::mouseExit(e);
      redoImage();
    }

    virtual void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override {
      ComponentType::mouseWheelMove(e, wheel);
      redoImage();
    }

    OpenGlImageComponent* getImageComponent() { return &image_component_; }
    virtual void redoImage() { image_component_.redrawImage(true); }

  protected:
    OpenGlImageComponent image_component_;
};

class OpenGlTextEditor : public OpenGlAutoImageComponent<TextEditor>, public TextEditor::Listener {
  public:
    OpenGlTextEditor(String name) : OpenGlAutoImageComponent(name) {
      monospace_ = false;
      image_component_.setComponent(this);
      addListener(this);
    }
  
    OpenGlTextEditor(String name, wchar_t password_char) : OpenGlAutoImageComponent(name, password_char) {
      monospace_ = false;
      image_component_.setComponent(this);
      addListener(this);
    }

    bool keyPressed(const KeyPress& key) override {
      bool result = TextEditor::keyPressed(key);
      redoImage();
      return result;
    }
  
    void textEditorTextChanged(TextEditor&) override { redoImage(); }
    void textEditorFocusLost(TextEditor&) override { redoImage(); }

    virtual void mouseDrag(const MouseEvent& e) override {
      TextEditor::mouseDrag(e);
      redoImage();
    }

    void applyFont() {
      Font font;
      if (monospace_)
        font = Fonts::instance()->monospace().withPointHeight(getHeight() / 2.0f);
      else
        font = Fonts::instance()->proportional_light().withPointHeight(getHeight() / 2.0f);

      applyFontToAllText(font);
      redoImage();
    }

    void visibilityChanged() override {
      TextEditor::visibilityChanged();

      if (isVisible() && !isMultiLine())
        applyFont();
    }

    void resized() override {
      TextEditor::resized();
      if (isMultiLine()) {
        float indent = image_component_.findValue(Skin::kLabelBackgroundRounding);
        setIndents(indent, indent);
        return;
      }

      if (monospace_)
        setIndents(getHeight() * 0.2, getHeight() * 0.17);
      else
        setIndents(getHeight() * 0.2, getHeight() * 0.15);
      if (isVisible())
        applyFont();
    }

    void setMonospace() {
      monospace_ = true;
    }

  private:
    bool monospace_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGlTextEditor)
};

class PlainTextComponent : public OpenGlImageComponent {
  public:
    enum FontType {
      kTitle,
      kLight,
      kRegular,
      kMono,
      kNumFontTypes
    };

    PlainTextComponent(String name, String text) : OpenGlImageComponent(name), text_(std::move(text)),
                                                   text_size_(1.0f), font_type_(kRegular),
                                                   justification_(Justification::centred),
                                                   buffer_(0) {
      setInterceptsMouseClicks(false, false);
    }

    void resized() override {
      OpenGlImageComponent::resized();
      redrawImage(true);
    }

    void setText(String text) {
      if (text_ == text)
        return;

      text_ = text;
      redrawImage(true);
    }

    String getText() const { return text_; }

    void paintToImage(Graphics& g) override {
      g.setColour(Colours::white);

      if (font_type_ == kTitle)
        g.setFont(Fonts::instance()->proportional_title().withPointHeight(text_size_));
      else if (font_type_ == kLight)
        g.setFont(Fonts::instance()->proportional_light().withPointHeight(text_size_));
      else if (font_type_ == kRegular)
        g.setFont(Fonts::instance()->proportional_regular().withPointHeight(text_size_));
      else
        g.setFont(Fonts::instance()->monospace().withPointHeight(text_size_));

      Component* component = component_ ? component_ : this;

      g.drawFittedText(text_, buffer_, 0, component->getWidth() - 2 * buffer_,
                       component->getHeight(), justification_, false);
    }

    void setTextSize(float size) {
      text_size_ = size;
      redrawImage(true);
    }

    void setFontType(FontType font_type) {
      font_type_ = font_type;
    }

    void setJustification(Justification justification) {
      justification_ = justification;
    }

    void setBuffer(int buffer) { buffer_ = buffer; }

  private:
    String text_;
    float text_size_;
    FontType font_type_;
    Justification justification_;
    int buffer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlainTextComponent)
};

class PlainShapeComponent : public OpenGlImageComponent {
  public:
    PlainShapeComponent(String name) : OpenGlImageComponent(name), justification_(Justification::centred) {
      setInterceptsMouseClicks(false, false);
    }

    void paintToImage(Graphics& g) override {
      Component* component = component_ ? component_ : this;
      Rectangle<float> bounds = component->getLocalBounds().toFloat();
      Path shape = shape_;
      shape.applyTransform(shape.getTransformToScaleToFit(bounds, true, justification_));

      g.setColour(Colours::white);
      g.fillPath(shape);
    }

    void setShape(Path shape) {
      shape_ = shape;
      redrawImage(true);
    }

    void setJustification(Justification justification) { justification_ = justification; }

  private:
    Path shape_;
    Justification justification_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlainShapeComponent)
};
