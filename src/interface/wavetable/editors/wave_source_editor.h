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

#include "audio_file_drop_source.h"
#include "open_gl_line_renderer.h"
#include "open_gl_multi_quad.h"
#include "wavetable_component_overlay.h"
#include "common.h"

class WaveSourceEditor : public OpenGlLineRenderer, public AudioFileDropSource {
  public:
    class Listener {
      public:
        virtual ~Listener() { }
        virtual void valuesChanged(int start, int end, bool mouse_up) = 0;
    };

    static constexpr int kMaxGridParts = WavetableComponentOverlay::kMaxGrid + 1;
    static constexpr int kNumCircles = kMaxGridParts * kMaxGridParts;
  
    enum WaveSourceMenu {
      kCancel = 0,
      kFlipHorizontal,
      kFlipVertical,
      kClear,
      kInitSaw,
    };

    WaveSourceEditor(int size);
    virtual ~WaveSourceEditor();

    void paintBackground(Graphics& g) override;
    void resized() override;

    virtual void init(OpenGlWrapper& open_gl) override {
      grid_lines_.init(open_gl);
      grid_circles_.init(open_gl);
      hover_circle_.init(open_gl);
      editing_line_.init(open_gl);
      OpenGlLineRenderer::init(open_gl);
    }

    virtual void render(OpenGlWrapper& open_gl, bool animate) override {
      grid_lines_.render(open_gl, animate);
      grid_circles_.render(open_gl, animate);
      hover_circle_.render(open_gl, animate);
      if (editing_)
        editing_line_.render(open_gl, animate);
      OpenGlLineRenderer::render(open_gl, animate);
    }

    virtual void destroy(OpenGlWrapper& open_gl) override {
      grid_lines_.destroy(open_gl);
      grid_circles_.destroy(open_gl);
      hover_circle_.destroy(open_gl);
      editing_line_.destroy(open_gl);
      OpenGlLineRenderer::destroy(open_gl);
    }

    void mouseDown(const MouseEvent& e) override;
    void mouseUp(const MouseEvent& e) override;
    void mouseMove(const MouseEvent& e) override;
    void mouseDrag(const MouseEvent& e) override;
    void mouseExit(const MouseEvent& e) override;
    float valueAt(int index) { return values_[index]; }

    void loadWaveform(float* waveform);
    void addListener(Listener* listener) { listeners_.push_back(listener); }
    void setEditable(bool editable);

    void setGrid(int horizontal, int vertical);

    void audioFileLoaded(const File& file) override;
    void fileDragEnter(const StringArray& files, int x, int y) override;
    void fileDragExit(const StringArray& files) override;
    void clear();
    void flipVertical();
    void flipHorizontal();

  private:
    void setGridPositions();
    void setHoverPosition();
    void changeValues(const MouseEvent& e);
    Point<int> getSnappedPoint(Point<int> input);
    Point<int> snapToGrid(Point<int> input);
    int getHoveredIndex(Point<int> position);
    float getSnapRadius();
    void setLineValues();

    std::vector<Listener*> listeners_;
    Point<int> last_edit_position_;
    Point<int> current_mouse_position_;

    OpenGlMultiQuad grid_lines_;
    OpenGlMultiQuad grid_circles_;
    OpenGlQuad hover_circle_;
    OpenGlLineRenderer editing_line_;

    std::unique_ptr<AudioFileDropSource> audio_file_drop_source_;
    std::unique_ptr<float[]> values_;
    bool editing_;
    bool dragging_audio_file_;
    bool editable_;
    int horizontal_grid_;
    int vertical_grid_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveSourceEditor)
};

