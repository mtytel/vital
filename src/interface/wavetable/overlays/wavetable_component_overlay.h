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

#include "open_gl_component.h"
#include "open_gl_multi_quad.h"
#include "synth_section.h"
#include "wavetable_playhead.h"
#include "wavetable_organizer.h"

class WavetableComponentOverlay : public SynthSection,
                                  public WavetablePlayhead::Listener,
                                  public WavetableOrganizer::Listener {
  public:
    static constexpr int kMaxGrid = 16;
    static constexpr float kTitleHeightForWidth = 0.1f;
    static constexpr float kWidgetHeightForWidth = 0.08f;
    static constexpr float kShadowPercent = 0.1f;
    static constexpr float kDividerPoint = 0.44f;
    static constexpr float kTitleHeightRatio = 0.4f;

    class ControlsBackground : public SynthSection {
      public:
        static constexpr int kMaxLines = 16;

        ControlsBackground() : SynthSection("background"), background_(Shaders::kRoundedRectangleFragment),
                               border_(Shaders::kRoundedRectangleBorderFragment),
                               lines_(kMaxLines, Shaders::kColorFragment),
                               title_backgrounds_(kMaxLines + 1, Shaders::kColorFragment) {
          addOpenGlComponent(&background_);
          addOpenGlComponent(&border_);
          addOpenGlComponent(&lines_);
          addOpenGlComponent(&title_backgrounds_);

          background_.setTargetComponent(this);
          border_.setTargetComponent(this);
          lines_.setTargetComponent(this);
          title_backgrounds_.setTargetComponent(this);

          for (int i = 0; i <= kMaxLines; ++i) {
            title_texts_[i] = std::make_unique<PlainTextComponent>("text", "");
            addOpenGlComponent(title_texts_[i].get());
            title_texts_[i]->setActive(false);
            title_texts_[i]->setFontType(PlainTextComponent::kLight);
          }
        }
        void clearLines() { line_positions_.clear(); setPositions(); }
        void clearTitles() { titles_.clear(); setPositions(); }
        void addLine(int position) { line_positions_.push_back(position); setPositions(); }
        void addTitle(const std::string& title) { titles_.push_back(title); setPositions(); }

        void setPositions();

      private:
        OpenGlQuad background_;
        OpenGlQuad border_;
        OpenGlMultiQuad lines_;
        OpenGlMultiQuad title_backgrounds_;
        std::unique_ptr<PlainTextComponent> title_texts_[kMaxLines + 1];
        std::vector<int> line_positions_;
        std::vector<std::string> titles_;
    };

    class Listener {
      public:
        virtual ~Listener() { }
      
        virtual void frameDoneEditing() = 0;
        virtual void frameChanged() = 0;
    };

    WavetableComponentOverlay(String name) : SynthSection(name), current_component_(nullptr),
                                             controls_width_(0), initialized_(false), padding_(0) {
      setInterceptsMouseClicks(false, true);
      addSubSection(&controls_background_);
      controls_background_.setAlwaysOnTop(true);
    }

    WavetableComponentOverlay() = delete;

    void paintBackground(Graphics& g) override { paintChildrenBackgrounds(g); }

    void playheadMoved(int position) override { }

    virtual void setEditBounds(Rectangle<int> bounds);
    virtual bool setTimeDomainBounds(Rectangle<int> bounds) { return false; }
    virtual bool setFrequencyAmplitudeBounds(Rectangle<int> bounds) { return false; }
    virtual bool setPhaseBounds(Rectangle<int> bounds) { return false; }
    void resetOverlay();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override {
      SynthSection::initOpenGlComponents(open_gl);
      initialized_ = true;
    }
    bool initialized() { return initialized_; }

    void addFrameListener(Listener* listener) {
      listeners_.push_back(listener);
    }

    void removeListener(Listener* listener) {
      listeners_.erase(std::find(listeners_.begin(), listeners_.end(), listener));
    }

    virtual void setPowerScale(bool scale) { }
    virtual void setFrequencyZoom(float zoom) { }
    void setPadding(int padding) { padding_ = padding; repaint(); }
    int getPadding() { return padding_; }
    void setComponent(WavetableComponent* component);
    WavetableComponent* getComponent() { return current_component_; }

  protected:
    void setControlsWidth(int width) { controls_width_ = width; repaint(); }
    void notifyChanged(bool mouse_up);
    float getTitleHeight();
    int getDividerX();
    int getWidgetHeight();
    int getWidgetPadding();

    WavetableComponent* current_component_;
    ControlsBackground controls_background_;
    std::vector<Listener*> listeners_;
    Rectangle<int> edit_bounds_;
    int controls_width_;
    bool initialized_;
    int padding_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableComponentOverlay)
};

