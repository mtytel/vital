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

#include "open_gl_line_renderer.h"
#include "common.h"
#include "wave_window_modifier.h"

class WaveWindowEditor : public OpenGlLineRenderer {
  public:
    static constexpr float kGrabRadius = 0.05f;
    static constexpr int kPointsPerSection = 50;
    static constexpr int kTotalPoints = 4 * kPointsPerSection;
  
    enum ActiveSide {
      kNone,
      kLeft,
      kRight
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void windowChanged(bool left, bool mouse_up) = 0;
    };

    WaveWindowEditor();
    virtual ~WaveWindowEditor();

    void paintBackground(Graphics& g) override { }
    void resized() override;

    virtual void init(OpenGlWrapper& open_gl) override {
      OpenGlLineRenderer::init(open_gl);
      edit_bars_.init(open_gl);
    }

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      OpenGlLineRenderer::render(open_gl, animate);
      edit_bars_.render(open_gl, animate);
    }

    virtual void destroy(OpenGlWrapper& open_gl) override {
      OpenGlLineRenderer::destroy(open_gl);
      edit_bars_.destroy(open_gl);
    }

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;

    void addListener(Listener* listener) { listeners_.push_back(listener); }

    ActiveSide getHover(Point<int> position);

    float getLeftPosition() { return left_position_; }
    float getRightPosition() { return right_position_; }
    void setPositions(float left, float right) {
      left_position_ = left;
      right_position_ = right;
      setPoints();
    }

    void setWindowShape(WaveWindowModifier::WindowShape window_shape) {
      window_shape_ = window_shape;
      setPoints();
    }

  private:
    void notifyWindowChanged(bool mouse_up);
    void changeValues(const MouseEvent& e);
    void setEditingQuads();
    void setPoints();

    std::vector<Listener*> listeners_;
    Point<int> last_edit_position_;

    OpenGlMultiQuad edit_bars_;

    WaveWindowModifier::WindowShape window_shape_;
    ActiveSide hovering_;
    ActiveSide editing_;
    float left_position_;
    float right_position_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveWindowEditor)
};

