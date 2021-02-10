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
#include "open_gl_multi_image.h"
#include "synth_section.h"
#include "wavetable_component_factory.h"

class WavetableComponent;
class WavetableCreator;
class WavetableGroup;

class WavetableComponentViewport : public Viewport {
  public:
    class Listener {
      public:
      virtual ~Listener() { }
      virtual void componentsScrolled() = 0;
    };

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void visibleAreaChanged(const Rectangle<int>& newVisibleArea) override {
      for (Listener* listener : listeners_)
        listener->componentsScrolled();

      Viewport::visibleAreaChanged(newVisibleArea);
    }

  private:
    std::vector<Listener*> listeners_;
};

class WavetableComponentList : public SynthSection, ScrollBar::Listener,
                               public WavetableComponentViewport::Listener {
  public:
    static constexpr int kMaxRows = 128;
    static constexpr int kMaxSources = 16;

    enum ComponentRowMenu {
      kRowCancel = 0,
      kReset,
      kMoveUp,
      kMoveDown,
      kRemove
    };

    class Listener {
      public:
        virtual ~Listener() { }
      
        virtual void componentAdded(WavetableComponent* component) = 0;
        virtual void componentRemoved(WavetableComponent* component) = 0;
        virtual void componentsReordered() = 0;
        virtual void componentsChanged() = 0;
        virtual void componentsScrolled(int offset) { }
    };

    WavetableComponentList(WavetableCreator* wavetable_creator);
    void clear();
    void init();
    void resized() override;
    void paintBackground(Graphics& g) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setRowHeight(int row_height) {
      row_height_ = row_height;
      resetGroups();
    }

    std::pair<int, int> getIndicesForRow(int row_index);
    void groupMenuClicked(int row_index);
    void modifierMenuClicked(int row_index);
    void menuClicked(int row_index);
    void addModifierClicked(int group_index);
    void addSourceClicked();
    void buttonClicked(Button* button) override;
    void componentsScrolled() override;

    void addSource(int index);
    void removeGroup(int index);

    void addComponent(int type);
    void removeComponent();
    void resetComponent();
    void removeGroup();
    void moveGroupUp();
    void moveGroupDown();
    void moveModifierUp();
    void moveModifierDown();
    int numGroups() { return wavetable_creator_->numGroups(); }

    void notifyComponentAdded(WavetableComponent* component);
    void notifyComponentRemoved(WavetableComponent* component);
    void notifyComponentsReordered();
    void notifyComponentsChanged();

    void scrollBarMoved(ScrollBar* scroll_bar, double range_start) override;
    void scroll(const MouseEvent& e, const MouseWheelDetails& wheel) {
      viewport_.mouseWheelMove(e, wheel);
    }

  protected:
    void resetGroups();
    void positionGroups();
    void setScrollBarRange();

    WavetableComponentViewport viewport_;
    Component component_container_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;

    WavetableCreator* wavetable_creator_;
    int current_group_index_;
    int current_component_index_;
    std::vector<Listener*> listeners_;
    OpenGlMultiQuad component_backgrounds_;
    std::unique_ptr<PlainTextComponent> names_[kMaxRows];
    std::unique_ptr<OpenGlShapeButton> menu_buttons_[kMaxRows];
    std::unique_ptr<OpenGlToggleButton> create_component_button_;
    std::unique_ptr<OpenGlToggleButton> add_modifier_buttons_[kMaxSources];
    std::unique_ptr<PlainShapeComponent> plus_icons_[kMaxSources + 1];
    int row_height_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableComponentList)
};
