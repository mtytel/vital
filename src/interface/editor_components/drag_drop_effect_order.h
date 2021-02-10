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
#include "synth_section.h"
#include "synth_constants.h"
#include "synth_button.h"
#include "synth_slider.h"
#include "open_gl_image_component.h"

class DraggableEffect : public SynthSection {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void effectEnabledChanged(DraggableEffect* effect, bool enabled) = 0;
    };

    DraggableEffect(const String& name, int order);
    void paint(Graphics& g) override;
    void paintBackground(Graphics& g) override { }

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override {
      SynthSection::renderOpenGlComponents(open_gl, animate);
    }

    void resized() override;
    void buttonClicked(Button* clicked_button) override;
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    bool enabled() const { return enable_->getToggleState(); }
    int order() const { return order_; }

    void hover(bool hover);

  private:
    Path icon_;
    int order_;
    bool hover_;
    std::unique_ptr<SynthButton> enable_;
    std::unique_ptr<OpenGlImageComponent> background_;
    std::vector<Listener*> listeners_;
};

class DragDropEffectOrder : public SynthSection, public DraggableEffect::Listener {
  public:
    static constexpr int kEffectPadding = 6;

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void orderChanged(DragDropEffectOrder* order) = 0;
        virtual void effectEnabledChanged(int order_index, bool enabled) = 0;
    };

    DragDropEffectOrder(String name);
    virtual ~DragDropEffectOrder();

    void resized() override;
    void paintBackground(Graphics& g) override;

    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;

    void mouseMove(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    void effectEnabledChanged(DraggableEffect* effect, bool enabled) override;
    void setAllValues(vital::control_map& controls) override;

    void moveEffect(int start_index, int end_index);
    void setStationaryEffectPosition(int index);
    void addListener(Listener* listener) { listeners_.push_back(listener); }

    int getEffectIndex(int index) const;
    Component* getEffect(int index) const;
    bool effectEnabled(int index) const;
    int getEffectIndexFromY(float y) const;

  private:
    int getEffectY(int index) const;

    std::vector<Listener*> listeners_;
    DraggableEffect* currently_dragged_;
    DraggableEffect* currently_hovered_;

    int last_dragged_index_;
    int mouse_down_y_;
    int dragged_starting_y_;
    std::vector<std::unique_ptr<DraggableEffect>> effect_list_;
    int effect_order_[vital::constants::kNumEffects];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DragDropEffectOrder)
};

