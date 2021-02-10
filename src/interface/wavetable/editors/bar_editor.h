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

#include "bar_renderer.h"
#include "common.h"
#include "open_gl_multi_quad.h"
#include "skin.h"
#include "utils.h"

class BarEditor : public BarRenderer {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void barsChanged(int start, int end, bool mouse_up) = 0;
    };

    enum BarEditorMenu {
      kCancel = 0,
      kClear,
      kClearRight,
      kClearLeft,
      kClearEven,
      kClearOdd,
      kRandomize
    };

    BarEditor(int num_points) : BarRenderer(num_points), editing_quad_(Shaders::kColorFragment), 
                                random_generator_(-1.0f, 1.0f), editing_(false), clear_value_(-1.0f) {
      current_mouse_position_ = Point<int>(-10, -10);
      editing_quad_.setTargetComponent(this);
    }
    virtual ~BarEditor() { }

    virtual void init(OpenGlWrapper& open_gl) override {
      BarRenderer::init(open_gl);
      editing_quad_.init(open_gl);
    }

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      BarRenderer::render(open_gl, animate);
      int hovered_index = getHoveredIndex(current_mouse_position_);
      if (current_mouse_position_.x < 0)
        hovered_index = -1;
      float bar_width = 2.0f * scale_ / num_points_;
      editing_quad_.setQuad(0, bar_width * hovered_index - 1.0f, -1.0f, bar_width, 2.0f);
      editing_quad_.render(open_gl, animate);
    }

    virtual void destroy(OpenGlWrapper& open_gl) override {
      BarRenderer::destroy(open_gl);
      editing_quad_.destroy(open_gl);
    }

    virtual void resized() override {
      BarRenderer::resized();
      editing_quad_.setColor(findColour(Skin::kLightenScreen, true));
    }

    void mouseMove(const MouseEvent& e) override;
    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }

    void setClearValue(float value) { clear_value_ = value; }
    void randomize();
    void clear();
    void clearRight();
    void clearLeft();
    void clearEven();
    void clearOdd();

  protected:
    void changeValues(const MouseEvent& e);
    int getHoveredIndex(Point<int> position);

    OpenGlQuad editing_quad_;
    vital::utils::RandomGenerator random_generator_;
    std::vector<Listener*> listeners_;
    Point<int> current_mouse_position_;
    Point<int> last_edit_position_;
    bool editing_;
    float clear_value_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarEditor)
};

