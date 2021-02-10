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

#include "line_generator.h"
#include "open_gl_image_component.h"
#include "open_gl_multi_image.h"
#include "open_gl_multi_quad.h"
#include "open_gl_line_renderer.h"
#include "synth_lfo.h"
#include "synth_module.h"

class LineEditor : public OpenGlLineRenderer, public TextEditor::Listener {
  public:
    static constexpr float kPositionWidth = 9.0f;
    static constexpr float kPowerWidth = 7.0f;
    static constexpr float kRingThickness = 0.45f;
    static constexpr float kGrabRadius = 12.0f;
    static constexpr float kDragRadius = 20.0f;
    static constexpr int kResolution = 64;
    static constexpr int kNumWrapPoints = 8;
    static constexpr int kDrawPoints = kResolution + LineGenerator::kMaxPoints;
    static constexpr int kTotalPoints = kDrawPoints + 2 * kNumWrapPoints;
    static constexpr int kMaxGridSizeX = 32;
    static constexpr int kMaxGridSizeY = 24;
    static constexpr float kPaddingY = 6.0f;
    static constexpr float kPaddingX = 0.0f;
    static constexpr float kPowerMouseMultiplier = 9.0f;
    static constexpr float kMinPointDistanceForPower = 3.0f;

    enum MenuOptions {
      kCancel,
      kCopy,
      kPaste,
      kSave,
      kEnterPhase,
      kEnterValue,
      kResetPower,
      kRemovePoint,
      kInit,
      kFlipHorizontal,
      kFlipVertical,
      kNumMenuOptions
    };

    class Listener {
      public:
        virtual ~Listener() { }
        virtual void setPhase(float phase) = 0;
        virtual void lineEditorScrolled(const MouseEvent& e, const MouseWheelDetails& wheel) = 0;
        virtual void togglePaintMode(bool enabled, bool temporary_switch) = 0;
        virtual void fileLoaded() = 0;
        virtual void importLfo() = 0;
        virtual void exportLfo() = 0;
        virtual void pointChanged(int index, Point<float> position, bool mouse_up) { }
        virtual void powersChanged(bool mouse_up) { }
        virtual void pointAdded(int index, Point<float> position) { }
        virtual void pointRemoved(int index) { }
        virtual void pointsAdded(int index, int num_points_added) { }
        virtual void pointsRemoved(int index, int num_points_removed) { }
    };

    LineEditor(LineGenerator* line_source);
    virtual ~LineEditor();

    void resetWavePath();
    void resized() override {
      OpenGlLineRenderer::resized();
      drag_circle_.setBounds(getLocalBounds());
      hover_circle_.setBounds(getLocalBounds());
      grid_lines_.setBounds(getLocalBounds());
      position_circle_.setBounds(getLocalBounds());
      point_circles_.setBounds(getLocalBounds());
      power_circles_.setBounds(getLocalBounds());
      resetPositions();
    }

    float padY(float y);
    float unpadY(float y);

    float padX(float x);
    float unpadX(float x);

    virtual void mouseDown(const MouseEvent& e) override;
    virtual void mouseDoubleClick(const MouseEvent& e) override;
    virtual void mouseMove(const MouseEvent& e) override;
    virtual void mouseDrag(const MouseEvent& e) override;
    virtual void mouseUp(const MouseEvent& e) override;

    virtual void respondToCallback(int point, int power, int option);
    bool hasMatchingSystemClipboard();
    void paintLine(const MouseEvent& e);

    void drawDown(const MouseEvent& e);
    void drawDrag(const MouseEvent& e);
    void drawUp(const MouseEvent& e);

    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override;
    void mouseExit(const MouseEvent& e) override;
    void clearActiveMouseActions();

    void renderGrid(OpenGlWrapper& open_gl, bool animate);
    void renderPoints(OpenGlWrapper& open_gl, bool animate);
    void init(OpenGlWrapper& open_gl) override;
    void render(OpenGlWrapper& open_gl, bool animate) override;
    void destroy(OpenGlWrapper& open_gl) override;
    void setSizeRatio(float ratio) { size_ratio_ = ratio; }
    float sizeRatio() const { return size_ratio_; }

    void setLoop(bool loop) { loop_ = loop; }
    void setSmooth(bool smooth) { model_->setSmooth(smooth); resetPositions(); }
    bool getSmooth() const { return model_->smooth(); }
    void setPaint(bool paint);
    void setPaintPattern(std::vector<std::pair<float, float>> pattern) { paint_pattern_ = pattern; }

    virtual void setGridSizeX(int size) { grid_size_x_ = size; setGridPositions(); }
    virtual void setGridSizeY(int size) { grid_size_y_ = size; setGridPositions(); }
    int getGridSizeX() { return grid_size_x_; }
    int getGridSizeY() { return grid_size_y_; }

    void setModel(LineGenerator* model) { model_ = model; resetPositions(); }
    LineGenerator* getModel() { return model_; }
    void showTextEntry();
    void hideTextEntry();
    void textEditorReturnKeyPressed(TextEditor& editor) override;
    void textEditorFocusLost(TextEditor& editor) override;
    void textEditorEscapeKeyPressed(TextEditor& editor) override;
    void setSliderPositionFromText();
    void setAllowFileLoading(bool allow) { allow_file_loading_ = allow; }

    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setActive(bool active) { active_ = active; }
    force_inline void resetPositions() { reset_positions_ = true; }
    OpenGlComponent* getTextEditorComponent() {
      if (value_entry_)
        return value_entry_->getImageComponent();
      return nullptr;
    }

  protected:
    void drawPosition(OpenGlWrapper& open_gl, Colour color, float fraction_x);
    void setEditingCircleBounds();
    void setGridPositions();
    void setPointPositions();
    void setGlPositions();
    int getActivePoint() { return active_point_; }
    int getActivePower() { return active_power_; }
    int getActiveGridSection() { return active_grid_section_; }
    bool isPainting() { return paint_ != temporary_paint_toggle_; }
    bool isPaintEnabled() { return paint_; }
    vital::poly_float adjustBoostPhase(vital::poly_float phase);
    virtual void enableTemporaryPaintToggle(bool toggle);

    bool active_;
    std::vector<Listener*> listeners_;

  private:
    float adjustBoostPhase(float phase);

    static std::pair<vital::Output*, vital::Output*> getOutputs(const vital::output_map& mono_modulations,
                                                                const vital::output_map& poly_modulations,
                                                                String name) {
      return {
        mono_modulations.at(name.toStdString()),
        poly_modulations.at(name.toStdString())
      };
    }

    int getHoverPoint(Point<float> position);
    int getHoverPower(Point<float> position);
    float getSnapRadiusX();
    float getSnapRadiusY();
    float getSnappedX(float x);
    float getSnappedY(float y);
    void addPointAt(Point<float> position);
    void movePoint(int index, Point<float> position, bool snap);
    void movePower(int index, Point<float> position, bool all, bool alternate);
    void removePoint(int index);
    float getMinX(int index);
    float getMaxX(int index);

    Point<float> valuesToOpenGlPosition(float x, float y);
    Point<float> getPowerPosition(int index);
    bool powerActive(int index);

    LineGenerator* model_;
    int active_point_;
    int active_power_;
    int active_grid_section_;
    bool dragging_;
    bool reset_positions_;
    bool allow_file_loading_;
    Point<float> last_mouse_position_;
    int last_model_render_;
    bool loop_;
    int grid_size_x_;
    int grid_size_y_;
    bool paint_;

    bool temporary_paint_toggle_;
    std::vector<std::pair<float, float>> paint_pattern_;

    vital::poly_float last_phase_;
    vital::poly_float last_voice_;
    vital::poly_float last_last_voice_;
    float size_ratio_;

    OpenGlQuad drag_circle_;
    OpenGlQuad hover_circle_;
    OpenGlMultiQuad grid_lines_;
    OpenGlQuad position_circle_;
    OpenGlMultiQuad point_circles_;
    OpenGlMultiQuad power_circles_;
    std::unique_ptr<OpenGlTextEditor> value_entry_;
    bool entering_phase_;
    int entering_index_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LineEditor)
};

