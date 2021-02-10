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
#include "fonts.h"
#include "paths.h"
#include "open_gl_image_component.h"
#include "open_gl_multi_quad.h"
#include "shaders.h"
#include "skin.h"
#include "synth_types.h"
#include "synth_button.h"

#include <functional>
#include <map>
#include <set>

class ModulationButton;
class OpenGlComponent;
class PresetSelector;
class SynthSlider;

struct PopupItems {
  int id;
  std::string name;
  bool selected;
  std::vector<PopupItems> items;

  PopupItems() : id(0), selected(false) { }
  PopupItems(std::string name) : id(0), name(std::move(name)), selected(false) { }

  PopupItems(int id, std::string name, bool selected, std::vector<PopupItems> items) {
    this->id = id;
    this->selected = selected;
    this->name = std::move(name);
    this->items = std::move(items);
  }

  void addItem(int sub_id, const std::string& sub_name, bool sub_selected = false) {
    items.emplace_back(sub_id, sub_name, sub_selected, std::vector<PopupItems>());
  }
  void addItem(const PopupItems& item) { items.push_back(item); }
  int size() const { return static_cast<int>(items.size()); }
};

class LoadingWheel : public OpenGlQuad {
  public:
    LoadingWheel() : OpenGlQuad(Shaders::kRotaryModulationFragment), tick_(0), complete_(false), complete_ticks_(0) {
      setAlpha(1.0f);
    }

    void resized() override {
      OpenGlQuad::resized();

      Colour color = findColour(Skin::kWidgetAccent1, true);
      setColor(color);
      setModColor(color);
      setAltColor(color);
    }

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      static constexpr float kRotationMult = 0.05f;
      static constexpr float kWidthFrequency = 0.025f;
      static constexpr float kMinRads = 0.6f;
      static constexpr float kMaxRads = 4.0f;
      static constexpr float kRadRange = kMaxRads - kMinRads;
      static constexpr float kCompleteSpeed = 0.15f;
      static constexpr float kStartRads = -vital::kPi - 0.05f;

      tick_++;
      setStartPos(-tick_ * kRotationMult);

      float width = (sinf(tick_ * kWidthFrequency) * 0.5f + 0.5f) * kRadRange + kMinRads;
      if (complete_) {
        complete_ticks_++;
        width += kCompleteSpeed * complete_ticks_;
      }

      setShaderValue(0, kStartRads, 0);
      setShaderValue(0, kStartRads + width, 1);
      setShaderValue(0, kStartRads, 2);
      setShaderValue(0, kStartRads + width, 3);

      OpenGlQuad::render(open_gl, animate);
    }

    void completeRing() {
      complete_ = true;
    }

  private:
    int tick_;
    bool complete_;
    int complete_ticks_;
};

class AppLogo : public OpenGlImageComponent {
  public:
    AppLogo(String name) : OpenGlImageComponent(std::move(name)) {
      logo_letter_ = Paths::vitalV();
      logo_ring_ = Paths::vitalRing();
    }

    void paint(Graphics& g) override {
      const DropShadow shadow(findColour(Skin::kShadow, true), 10.0f, Point<int>(0, 0));

      logo_letter_.applyTransform(logo_letter_.getTransformToScaleToFit(getLocalBounds().toFloat(), true));
      logo_ring_.applyTransform(logo_ring_.getTransformToScaleToFit(getLocalBounds().toFloat(), true));

      shadow.drawForPath(g, logo_letter_);
      shadow.drawForPath(g, logo_ring_);

      Colour letter_top_color = findColour(Skin::kWidgetSecondary1, true);
      Colour letter_bottom_color = findColour(Skin::kWidgetSecondary2, true);
      Colour ring_top_color = findColour(Skin::kWidgetPrimary1, true);
      Colour ring_bottom_color = findColour(Skin::kWidgetPrimary2, true);
      ColourGradient letter_gradient(letter_top_color, 0.0f, 12.0f, letter_bottom_color, 0.0f, 96.0f, false);
      ColourGradient ring_gradient(ring_top_color, 0.0f, 12.0f, ring_bottom_color, 0.0f, 96.0f, false);
      g.setGradientFill(letter_gradient);
      g.fillPath(logo_letter_);

      g.setGradientFill(ring_gradient);
      g.fillPath(logo_ring_);
    }

  private:
    Path logo_letter_;
    Path logo_ring_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppLogo)
};

class SynthSection : public Component, public Slider::Listener,
                     public Button::Listener, public SynthButton::ButtonListener {
  public:
    static constexpr int kDefaultPowerButtonOffset = 0;
    static constexpr float kPowerButtonPaddingPercent = 0.29f;
    static constexpr float kTransposeHeightPercent = 0.5f;
    static constexpr float kTuneHeightPercent = 0.4f;
    static constexpr float kJointModulationRadiusPercent = 0.1f;
    static constexpr float kJointModulationExtensionPercent = 0.6666f;
    static constexpr float kPitchLabelPercent = 0.33f;
    static constexpr float kJointLabelHeightPercent = 0.4f;
    static constexpr double kTransposeMouseSensitivity = 0.2;
    static constexpr float kJointLabelBorderRatioX = 0.05f;

    static constexpr int kDefaultBodyRounding = 4;
    static constexpr int kDefaultLabelHeight = 10;
    static constexpr int kDefaultLabelBackgroundHeight = 16;
    static constexpr int kDefaultLabelBackgroundWidth = 56;
    static constexpr int kDefaultLabelBackgroundRounding = 4;
    static constexpr int kDefaultPadding = 2;
    static constexpr int kDefaultPopupMenuWidth = 150;
    static constexpr int kDefaultDualPopupMenuWidth = 340;
    static constexpr int kDefaultStandardKnobSize = 32;
    static constexpr int kDefaultKnobThickness = 2;
    static constexpr float kDefaultKnobModulationAmountThickness = 2.0f;
    static constexpr int kDefaultKnobModulationMeterSize = 43;
    static constexpr int kDefaultKnobModulationMeterThickness = 4;
    static constexpr int kDefaultModulationButtonWidth = 64;
    static constexpr int kDefaultModFontSize = 10;
    static constexpr int kDefaultKnobSectionHeight = 64;
    static constexpr int kDefaultSliderWidth = 24;
    static constexpr int kDefaultTextWidth = 80;
    static constexpr int kDefaultTextHeight = 24;
    static constexpr int kDefaultWidgetMargin = 6;
    static constexpr float kDefaultWidgetFillFade = 0.3f;
    static constexpr float kDefaultWidgetLineWidth = 4.0f;
    static constexpr float kDefaultWidgetFillCenter = 0.0f;

    class OffOverlay : public OpenGlQuad {
      public:
        OffOverlay() : OpenGlQuad(Shaders::kColorFragment) { }

        void paintBackground(Graphics& g) override { }
    };

    SynthSection(const String& name);
    virtual ~SynthSection() = default;

    void setParent(const SynthSection* parent) { parent_ = parent; }
    float findValue(Skin::ValueId value_id) const;

    virtual void reset();
    virtual void resized() override;
    virtual void paint(Graphics& g) override;
    virtual void paintSidewaysHeadingText(Graphics& g);
    virtual void paintHeadingText(Graphics& g);
    virtual void paintBackground(Graphics& g);
    virtual void setSkinValues(const Skin& skin, bool top_level);
    void setSkinOverride(Skin::SectionOverride skin_override) { skin_override_ = skin_override; }
    virtual void repaintBackground();
    void showPopupBrowser(SynthSection* owner, Rectangle<int> bounds, std::vector<File> directories,
                          String extensions, std::string passthrough_name, std::string additional_folders_name);
    void updatePopupBrowser(SynthSection* owner);

    void showPopupSelector(Component* source, Point<int> position, const PopupItems& options,
                           std::function<void(int)> callback, std::function<void()> cancel = { });
    void showDualPopupSelector(Component* source, Point<int> position, int width,
                               const PopupItems& options, std::function<void(int)> callback);
    void showPopupDisplay(Component* source, const std::string& text,
                          BubbleComponent::BubblePlacement placement, bool primary);
    void hidePopupDisplay(bool primary);

    virtual void loadFile(const File& file) { }
    virtual File getCurrentFile() { return File(); }
    virtual std::string getFileName() { return ""; }
    virtual std::string getFileAuthor() { return ""; }
    virtual void paintContainer(Graphics& g);
    virtual void paintBody(Graphics& g, Rectangle<int> bounds);
    virtual void paintBorder(Graphics& g, Rectangle<int> bounds);
    virtual void paintBody(Graphics& g);
    virtual void paintBorder(Graphics& g);
    int getComponentShadowWidth();
    virtual void paintTabShadow(Graphics& g);
    void paintTabShadow(Graphics& g, Rectangle<int> bounds);
    virtual void paintBackgroundShadow(Graphics& g) { }
    virtual void setSizeRatio(float ratio);
    void paintKnobShadows(Graphics& g);
    Font getLabelFont();
    void setLabelFont(Graphics& g);
    void drawLabelConnectionForComponents(Graphics& g, Component* left, Component* right);
    void drawLabelBackground(Graphics& g, Rectangle<int> bounds, bool text_component = false);
    void drawLabelBackgroundForComponent(Graphics& g, Component* component);
    Rectangle<int> getDividedAreaBuffered(Rectangle<int> full_area, int num_sections, int section, int buffer);
    Rectangle<int> getDividedAreaUnbuffered(Rectangle<int> full_area, int num_sections, int section, int buffer);
    Rectangle<int> getLabelBackgroundBounds(Rectangle<int> bounds, bool text_component = false);
    Rectangle<int> getLabelBackgroundBounds(Component* component, bool text_component = false) {
      return getLabelBackgroundBounds(component->getBounds(), text_component);
    }
    void drawLabel(Graphics& g, String text, Rectangle<int> component_bounds, bool text_component = false);
    void drawLabelForComponent(Graphics& g, String text, Component* component, bool text_component = false) {
      drawLabel(g, std::move(text), component->getBounds(), text_component);
    }
    void drawTextBelowComponent(Graphics& g, String text, Component* component, int space, int padding = 0);

    virtual void paintChildrenShadows(Graphics& g);
    void paintChildrenBackgrounds(Graphics& g);
    void paintOpenGlChildrenBackgrounds(Graphics& g);
    void paintChildBackground(Graphics& g, SynthSection* child);
    void paintChildShadow(Graphics& g, SynthSection* child);
    void paintOpenGlBackground(Graphics& g, OpenGlComponent* child);
    void drawTextComponentBackground(Graphics& g, Rectangle<int> bounds, bool extend_to_label);
    void drawTempoDivider(Graphics& g, Component* sync);
    virtual void initOpenGlComponents(OpenGlWrapper& open_gl);
    virtual void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate);
    virtual void destroyOpenGlComponents(OpenGlWrapper& open_gl);

    virtual void sliderValueChanged(Slider* moved_slider) override;
    virtual void buttonClicked(Button* clicked_button) override;
    virtual void guiChanged(SynthButton* button) override;

    std::map<std::string, SynthSlider*> getAllSliders() { return all_sliders_; }
    std::map<std::string, ToggleButton*> getAllButtons() { return all_buttons_; }
    std::map<std::string, ModulationButton*> getAllModulationButtons() {
      return all_modulation_buttons_;
    }

    virtual void setActive(bool active);
    bool isActive() const { return active_; }
    virtual void animate(bool animate);
    virtual void setAllValues(vital::control_map& controls);
    virtual void setValue(const std::string& name, vital::mono_float value, NotificationType notification);

    void addModulationButton(ModulationButton* button, bool show = true);
    void addSubSection(SynthSection* section, bool show = true);
    void removeSubSection(SynthSection* section);
    virtual void setScrollWheelEnabled(bool enabled);
    ToggleButton* activator() const { return activator_; }

    void setSkinValues(std::map<Skin::ValueId, float> values) { value_lookup_ = std::move(values); }
    void setSkinValue(Skin::ValueId id, float value) { value_lookup_[id] = value; }

    float getTitleWidth();
    float getPadding();
    float getPowerButtonOffset() const { return size_ratio_ * kDefaultPowerButtonOffset; }
    float getKnobSectionHeight();
    float getSliderWidth();
    float getSliderOverlap();
    float getSliderOverlapWithSpace();
    float getTextComponentHeight();
    float getStandardKnobSize();
    float getTotalKnobHeight() { return getStandardKnobSize(); }
    float getTextSectionYOffset();
    float getModButtonWidth();
    float getModFontSize();
    float getWidgetMargin();
    float getWidgetRounding();
    float getSizeRatio() const { return size_ratio_; }
    int getPopupWidth() const { return kDefaultPopupMenuWidth * size_ratio_; }
    int getDualPopupWidth() const { return kDefaultDualPopupMenuWidth * size_ratio_; }

  protected:
    void setSliderHasHzAlternateDisplay(SynthSlider* slider);
    void setSidewaysHeading(bool sideways) { sideways_heading_ = sideways; }
    void addToggleButton(ToggleButton* button, bool show);
    void addButton(OpenGlToggleButton* button, bool show = true);
    void addButton(OpenGlShapeButton* button, bool show = true);
    void addSlider(SynthSlider* slider, bool show = true, bool listen = true);
    void addOpenGlComponent(OpenGlComponent* open_gl_component, bool to_beginning = false);
    void setActivator(SynthButton* activator);
    void createOffOverlay();
    void setPresetSelector(PresetSelector* preset_selector, bool half = false) {
      preset_selector_ = preset_selector;
      preset_selector_half_width_ = half;
    }
    void paintJointControlSliderBackground(Graphics& g, int x, int y, int width, int height);
    void paintJointControlBackground(Graphics& g, int x, int y, int width, int height);
    void paintJointControl(Graphics& g, int x, int y, int width, int height, const std::string& name);
    void placeJointControls(int x, int y, int width, int height,
                            SynthSlider* left, SynthSlider* right, Component* widget = nullptr);
    void placeTempoControls(int x, int y, int width, int height, SynthSlider* tempo, SynthSlider* sync);
    void placeRotaryOption(Component* option, SynthSlider* rotary);
    void placeKnobsInArea(Rectangle<int> area, std::vector<Component*> knobs);

    void lockCriticalSection();
    void unlockCriticalSection();
    Rectangle<int> getPresetBrowserBounds();
    int getTitleTextRight();
    Rectangle<int> getPowerButtonBounds();
    Rectangle<int> getTitleBounds();
    float getDisplayScale() const;
    virtual int getPixelMultiple() const;

    std::map<Skin::ValueId, float> value_lookup_;

    std::vector<SynthSection*> sub_sections_;
    std::vector<OpenGlComponent*> open_gl_components_;

    std::map<std::string, SynthSlider*> slider_lookup_;
    std::map<std::string, Button*> button_lookup_;
    std::map<std::string, ModulationButton*> modulation_buttons_;

    std::map<std::string, SynthSlider*> all_sliders_;
    std::map<std::string, ToggleButton*> all_buttons_;
    std::map<std::string, ModulationButton*> all_modulation_buttons_;

    const SynthSection* parent_;
    SynthButton* activator_;
    PresetSelector* preset_selector_;
    bool preset_selector_half_width_;
    std::unique_ptr<OffOverlay> off_overlay_;

    Skin::SectionOverride skin_override_;
    float size_ratio_;
    bool active_;
    bool sideways_heading_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthSection)
};

